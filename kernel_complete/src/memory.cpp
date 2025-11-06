#include "../include/memory.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <list>
#include <unordered_map>

MemoryManager::MemoryManager(int num_frames, ReplacementPolicy policy)
    : num_frames_(num_frames),
      policy_(policy),
      total_accesses_(0),
      page_faults_(0),
      page_hits_(0),
      current_time_(0),
      pff_window_size_(10),           // Ventana de 10 accesos
      pff_upper_threshold_(0.6),      // 60% de fallos → aumentar frames
      pff_lower_threshold_(0.1),      // 10% de fallos → reducir frames
      pff_min_frames_per_process_(2), // Mínimo 2 frames por proceso
      pff_max_frames_per_process_(num_frames / 2) // Máximo 50% de frames
{
    frames_.resize(num_frames_);
    for (int i = 0; i < num_frames_; i++) {
        frames_[i].frame_id   = i;
        frames_[i].page_number= -1;
        frames_[i].process_id = -1;
        frames_[i].occupied   = false;
        frames_[i].load_time  = -1;
        frames_[i].last_used  = -1;
    }

    std::string pol = (policy_ == ReplacementPolicy::FIFO) ? "FIFO" :
                      (policy_ == ReplacementPolicy::LRU) ? "LRU" : "PFF";
    std::cout << Color::GREEN << "[MEMORY] Inicializada con "
              << num_frames_ << " frames (política=" << pol << ")"
              << Color::RESET << std::endl;

    if (policy_ == ReplacementPolicy::PFF) {
        std::cout << Color::CYAN << "[PFF] Configuración:"
                  << "\n  ├─ Ventana de medición: " << pff_window_size_ << " accesos"
                  << "\n  ├─ Umbral superior:     " << (pff_upper_threshold_ * 100) << "%"
                  << "\n  ├─ Umbral inferior:     " << (pff_lower_threshold_ * 100) << "%"
                  << "\n  ├─ Frames mín/proceso:  " << pff_min_frames_per_process_
                  << "\n  └─ Frames máx/proceso:  " << pff_max_frames_per_process_
                  << Color::RESET << std::endl;
    }
}

void MemoryManager::set_policy(ReplacementPolicy p) {
    policy_ = p;

    // Re-construir estructuras de reemplazo a partir del estado actual
    // FIFO: re-ordenamos por load_time (más viejo sale primero)
    // LRU: re-llenamos lista y mapa; ponemos más recientes adelante
    std::queue<int> empty;
    std::swap(fifo_queue_, empty);
    lru_list_.clear();
    lru_pos_.clear();

    // recopilar frames ocupados
    std::vector<int> occ;
    occ.reserve(num_frames_);
    for (auto& f : frames_) if (f.occupied) occ.push_back(f.frame_id);

    if (policy_ == ReplacementPolicy::FIFO) {
        std::sort(occ.begin(), occ.end(),
                  [&](int a, int b){ return frames_[a].load_time < frames_[b].load_time; });
        for (int id : occ) fifo_queue_.push(id);
    } else { // LRU
        // ordenamos por last_used descendente para agregar recientes al frente
        std::sort(occ.begin(), occ.end(),
                  [&](int a, int b){ return frames_[a].last_used > frames_[b].last_used; });
        for (int id : occ) lru_add(id);
    }

    std::string pol = (policy_ == ReplacementPolicy::FIFO) ? "FIFO" :
                      (policy_ == ReplacementPolicy::LRU) ? "LRU" : "PFF";
    std::cout << Color::CYAN << "[MEMORY] Política cambiada a " << pol
              << Color::RESET << std::endl;

    // Si cambiamos a PFF, mostrar configuración
    if (policy_ == ReplacementPolicy::PFF) {
        std::cout << Color::CYAN << "[PFF] Ventana=" << pff_window_size_
                  << " accesos, Umbrales=[" << (pff_lower_threshold_ * 100)
                  << "%, " << (pff_upper_threshold_ * 100) << "%]"
                  << Color::RESET << std::endl;
    }
}

bool MemoryManager::access_page(int process_id, int page_number) {
    total_accesses_++;
    current_time_++;

    bool window_complete = false;

    // Inicializar proceso en PFF si es necesario
    if (policy_ == ReplacementPolicy::PFF &&
        pff_process_info_.find(process_id) == pff_process_info_.end()) {
        pff_initialize_process(process_id);
    }

    // HIT
    if (page_tables_[process_id].count(page_number) > 0 &&
        page_tables_[process_id][page_number].valid) {

        page_hits_++;
        int frame_id = page_tables_[process_id][page_number].frame_id;

        // Tocar LRU
        frames_[frame_id].last_used = current_time_;
        if (policy_ == ReplacementPolicy::LRU) {
            lru_touch(frame_id);
        }

        // Actualizar estadísticas PFF
        if (policy_ == ReplacementPolicy::PFF) {
            window_complete = pff_update_stats(process_id, false); // false = no fue fault
        }

        std::cout << Color::GREEN << "[HIT] "
                  << "P" << process_id << " página " << page_number
                  << " → frame " << frame_id
                  << " (hits=" << page_hits_ << ")"
                  << Color::RESET << std::endl;

        if (policy_ == ReplacementPolicy::PFF && window_complete) {
            pff_adjust_allocation(process_id);
        }
        return true;
    }

    // PAGE FAULT
    page_faults_++;
    std::cout << Color::YELLOW << "[PAGE FAULT #" << page_faults_ << "] "
              << "P" << process_id << " página " << page_number
              << Color::RESET << std::endl;

    // Actualizar estadísticas PFF
    if (policy_ == ReplacementPolicy::PFF) {
        window_complete = pff_update_stats(process_id, true); // true = fue fault
    }

    // Buscar frame libre
    int frame_id = find_free_frame();
    bool frame_from_reclaim = false;

    if (policy_ == ReplacementPolicy::PFF) {
        bool can_grow = pff_can_allocate_frame(process_id);
        if (!can_grow) {
            frame_id = -1; // fuerza selección dentro del proceso
        } else if (frame_id == -1) {
            frame_id = pff_reclaim_frame_from_process(process_id);
            frame_from_reclaim = (frame_id != -1);
        }
    }

    // Si no hay, seleccionar víctima según política
    if (frame_id == -1) {
        if (policy_ == ReplacementPolicy::FIFO) {
            frame_id = select_victim_fifo();
            std::cout << Color::RED << "  └─ Evictando frame " << frame_id
                      << " (FIFO)" << Color::RESET << std::endl;
        } else if (policy_ == ReplacementPolicy::LRU) {
            frame_id = select_victim_lru();
            std::cout << Color::RED << "  └─ Evictando frame " << frame_id
                      << " (LRU)" << Color::RESET << std::endl;
        } else { // PFF
            frame_id = select_victim_pff(process_id);
            if (frame_id == -1) {
                frame_id = select_victim_lru();
                std::cout << Color::YELLOW << "[PFF] Sin frames propios; usando LRU global"
                          << Color::RESET << std::endl;
            } else {
                std::cout << Color::RED << "  └─ Evictando frame " << frame_id
                          << " (PFF)" << Color::RESET << std::endl;
            }
        }
        evict_page(frame_id);
    } else if (!frame_from_reclaim && frames_[frame_id].occupied) {
        evict_page(frame_id);
    }

    // Cargar
    load_page(process_id, page_number, frame_id);

    std::cout << Color::CYAN << "  └─ Página cargada en frame " << frame_id
              << Color::RESET << std::endl;

    // PFF: Ajustar asignación de frames si es necesario
    if (policy_ == ReplacementPolicy::PFF && window_complete) {
        pff_adjust_allocation(process_id);
    }

    return false;
}

int MemoryManager::find_free_frame() {
    for (int i = 0; i < num_frames_; i++) {
        if (!frames_[i].occupied) return i;
    }
    return -1;
}

int MemoryManager::select_victim_fifo() {
    if (fifo_queue_.empty()) return 0; // fallback
    int victim = fifo_queue_.front();
    fifo_queue_.pop();
    return victim;
}

int MemoryManager::select_victim_lru() {
    // El menos recientemente usado está al FINAL de la lista
    if (lru_list_.empty()) return 0; // fallback
    int victim = lru_list_.back();
    // lo quitamos de estructuras LRU aquí; evict_page también lo quitará si siguiera marcado
    lru_erase(victim);
    return victim;
}

void MemoryManager::load_page(int process_id, int page_number, int frame_id) {
    // Actualizar frame
    frames_[frame_id].occupied   = true;
    frames_[frame_id].page_number= page_number;
    frames_[frame_id].process_id = process_id;
    frames_[frame_id].load_time  = current_time_;
    frames_[frame_id].last_used  = current_time_;

    // Siempre mantener FIFO en orden de carga
    fifo_queue_.push(frame_id);

    // Mantener LRU
    lru_add(frame_id);

    // Actualizar page table
    page_tables_[process_id][page_number].frame_id = frame_id;
    page_tables_[process_id][page_number].valid    = true;
}

void MemoryManager::evict_page(int frame_id) {
    if (frames_[frame_id].occupied) {
        int old_process = frames_[frame_id].process_id;
        int old_page    = frames_[frame_id].page_number;

        // Invalidar page table
        page_tables_[old_process][old_page].valid = false;

        // Quitar de LRU si estuviera
        lru_erase(frame_id);

        // Marcar libre
        frames_[frame_id].occupied   = false;
        frames_[frame_id].process_id = -1;
        frames_[frame_id].page_number= -1;
        frames_[frame_id].load_time  = -1;
        frames_[frame_id].last_used  = -1;
    }
}

void MemoryManager::display_frames() const {
    print_header("ESTADO DE FRAMES");

    std::cout << std::left
              << std::setw(10) << "Frame ID"
              << std::setw(12) << "Estado"
              << std::setw(12) << "Proceso"
              << std::setw(12) << "Página"
              << std::setw(12) << "Load"
              << std::setw(12) << "LastUsed"
              << std::endl;
    print_separator(70);

    for (const auto& frame : frames_) {
        std::string status_color = frame.occupied ? Color::GREEN : Color::WHITE;
        std::string status = frame.occupied ? "OCUPADO" : "LIBRE";

        std::cout << status_color
                  << std::setw(10) << frame.frame_id
                  << std::setw(12) << status;

        if (frame.occupied) {
            std::cout << std::setw(12) << ("P" + std::to_string(frame.process_id))
                      << std::setw(12) << frame.page_number
                      << std::setw(12) << frame.load_time
                      << std::setw(12) << frame.last_used;
        } else {
            std::cout << std::setw(12) << "-"
                      << std::setw(12) << "-"
                      << std::setw(12) << "-"
                      << std::setw(12) << "-";
        }
        std::cout << Color::RESET << std::endl;
    }
    std::cout << std::endl;
}

void MemoryManager::display_stats() const {
    print_header("ESTADÍSTICAS DE MEMORIA");

    int occupied_frames = std::count_if(frames_.begin(), frames_.end(),
                                       [](const Frame& f) { return f.occupied; });
    int free_frames = num_frames_ - occupied_frames;

    std::cout << "  Frames totales:       " << num_frames_ << std::endl;
    std::cout << "   ├─ Ocupados:           " << occupied_frames << std::endl;
    std::cout << "   └─ Libres:             " << free_frames << std::endl;

    std::cout << "\n Accesos a memoria:     " << total_accesses_ << std::endl;
    std::cout << "    ├─ Page Hits:          " << page_hits_
              << " (" << std::fixed << std::setprecision(1)
              << get_hit_ratio() * 100 << "%)" << std::endl;
    std::cout << "    └─ Page Faults:        " << page_faults_
              << " (" << std::fixed << std::setprecision(1)
              << get_fault_rate() * 100 << "%)" << std::endl;

    std::string algo_name;
    if (policy_ == ReplacementPolicy::FIFO) {
        algo_name = "FIFO (First-In-First-Out)";
    } else if (policy_ == ReplacementPolicy::LRU) {
        algo_name = "LRU (Least Recently Used)";
    } else {
        algo_name = "PFF (Page Fault Frequency) - Gestión Avanzada";
    }

    std::cout << "\nAlgoritmo:             " << algo_name << std::endl;
    std::cout << std::endl;
}

void MemoryManager::display_page_table(int process_id) const {
    if (page_tables_.count(process_id) == 0 || page_tables_.at(process_id).empty()) {
        std::cout << Color::YELLOW << "Proceso P" << process_id
                  << " no tiene páginas cargadas" << Color::RESET << std::endl;
        return;
    }

    print_header("PAGE TABLE - P" + std::to_string(process_id));

    std::cout << std::left
              << std::setw(15) << "Página Virtual"
              << std::setw(15) << "Frame Físico"
              << std::setw(10) << "Válido"
              << std::endl;
    print_separator(40);

    for (const auto& entry : page_tables_.at(process_id)) {
        int page_num = entry.first;
        const PageTableEntry& pte = entry.second;

        std::string color = pte.valid ? Color::GREEN : Color::RED;
        std::string valid_str = pte.valid ? "SÍ" : "NO";

        std::cout << color
                  << std::setw(15) << page_num
                  << std::setw(15) << (pte.valid ? std::to_string(pte.frame_id) : "-")
                  << std::setw(10) << valid_str
                  << Color::RESET << std::endl;
    }
    std::cout << std::endl;
}

double MemoryManager::get_hit_ratio() const {
    if (total_accesses_ == 0) return 0.0;
    return static_cast<double>(page_hits_) / total_accesses_;
}

double MemoryManager::get_fault_rate() const {
    if (total_accesses_ == 0) return 0.0;
    return static_cast<double>(page_faults_) / total_accesses_;
}

void MemoryManager::reset_stats() {
    total_accesses_ = 0;
    page_faults_ = 0;
    page_hits_ = 0;
    current_time_ = 0;
    std::cout << Color::CYAN << "[MEMORY] Estadísticas reiniciadas"
              << Color::RESET << std::endl;
}

/* LRU helpers */

void MemoryManager::lru_touch(int frame_id) const {
    auto it = lru_pos_.find(frame_id);
    if (it == lru_pos_.end()) {
        // si no está, lo añadimos como más reciente
        lru_add(frame_id);
        return;
    }
    // mover a frente (más reciente)
    lru_list_.erase(it->second);
    lru_list_.push_front(frame_id);
    lru_pos_[frame_id] = lru_list_.begin();
}

void MemoryManager::lru_erase(int frame_id) const {
    auto it = lru_pos_.find(frame_id);
    if (it != lru_pos_.end()) {
        lru_list_.erase(it->second);
        lru_pos_.erase(it);
    }
}

void MemoryManager::lru_add(int frame_id) const {
    // insertar como más reciente sólo si el frame está ocupado
    if (!frames_[frame_id].occupied) return;
    // evitar duplicados
    if (lru_pos_.count(frame_id)) {
        lru_touch(frame_id);
        return;
    }
    lru_list_.push_front(frame_id);
    lru_pos_[frame_id] = lru_list_.begin();
}

/* ===================== PFF (Page Fault Frequency) helpers ===================== */

void MemoryManager::pff_initialize_process(int process_id) {
    PFFProcessInfo info;
    info.min_frames = std::max(1, std::min(pff_min_frames_per_process_, num_frames_));
    info.max_frames = std::max(info.min_frames, std::min(pff_max_frames_per_process_, num_frames_));
    info.allocated_frames = info.min_frames;
    info.recent_faults = 0;
    info.recent_accesses = 0;
    info.window_start_time = current_time_;
    info.fault_rate = 0.0;

    pff_process_info_[process_id] = info;

    std::cout << Color::CYAN << "[PFF] Proceso P" << process_id
              << " inicializado con " << info.allocated_frames << " frames"
              << Color::RESET << std::endl;
}

bool MemoryManager::pff_update_stats(int process_id, bool was_fault) {
    auto& info = pff_process_info_[process_id];
    info.recent_accesses++;

    if (was_fault) {
        info.recent_faults++;
    }

    if (info.recent_accesses > 0) {
        info.fault_rate = static_cast<double>(info.recent_faults) / info.recent_accesses;
    }

    return info.recent_accesses >= pff_window_size_;
}

void MemoryManager::pff_adjust_allocation(int process_id) {
    auto& info = pff_process_info_[process_id];

    if (info.recent_accesses < pff_window_size_) {
        return;
    }

    const double window_fault_rate =
        (info.recent_accesses > 0)
            ? static_cast<double>(info.recent_faults) / info.recent_accesses
            : 0.0;
    info.fault_rate = window_fault_rate;

    int actual_frames = get_process_frame_count(process_id);

    if (window_fault_rate > pff_upper_threshold_) {
        if (info.allocated_frames < info.max_frames) {
            const int previous_target = info.allocated_frames;
            info.allocated_frames = std::min(info.allocated_frames + 1, info.max_frames);

            if (info.allocated_frames > previous_target) {
                std::cout << Color::MAGENTA << "[PFF] P" << process_id
                          << " alta tasa de fallos (" << static_cast<int>(window_fault_rate * 100)
                          << "%) → objetivo frames " << info.allocated_frames
                          << Color::RESET << std::endl;

                if (actual_frames < info.allocated_frames) {
                    int free_frames = 0;
                    for (const auto& f : frames_) {
                        if (!f.occupied) {
                            free_frames++;
                        }
                    }
                    if (free_frames == 0) {
                        const int reclaimed = pff_reclaim_frame_from_process(process_id);
                        if (reclaimed == -1) {
                            std::cout << Color::YELLOW << "[PFF] P" << process_id
                                      << " solicita frames pero no hay disponibles"
                                      << Color::RESET << std::endl;
                        }
                    }
                }
            }
        }
    } else if (window_fault_rate < pff_lower_threshold_) {
        if (info.allocated_frames > info.min_frames) {
            const int previous_target = info.allocated_frames;
            info.allocated_frames = std::max(info.allocated_frames - 1, info.min_frames);

            if (info.allocated_frames < previous_target) {
                std::cout << Color::CYAN << "[PFF] P" << process_id
                          << " baja tasa de fallos (" << static_cast<int>(window_fault_rate * 100)
                          << "%) → objetivo frames " << info.allocated_frames
                          << Color::RESET << std::endl;

                while (actual_frames > info.allocated_frames) {
                    int victim = select_victim_pff(process_id);
                    if (victim == -1) {
                        break;
                    }
                    evict_page(victim);
                    actual_frames--;
                    std::cout << Color::CYAN << "  └─ Liberado frame " << victim
                              << " (P" << process_id << ")"
                              << Color::RESET << std::endl;
                }
            }
        }
    }

    info.recent_accesses = 0;
    info.recent_faults = 0;
    info.window_start_time = current_time_;
}

int MemoryManager::select_victim_pff(int process_id) {
    // En PFF, seleccionamos víctima del mismo proceso usando LRU
    // Buscar el frame menos recientemente usado de este proceso
    int victim = -1;
    int oldest_time = current_time_ + 1;

    for (const auto& frame : frames_) {
        if (frame.occupied && frame.process_id == process_id) {
            if (frame.last_used < oldest_time) {
                oldest_time = frame.last_used;
                victim = frame.frame_id;
            }
        }
    }

    return victim;
}

bool MemoryManager::pff_can_allocate_frame(int process_id) const {
    auto it = pff_process_info_.find(process_id);
    if (it == pff_process_info_.end()) {
        return true; // Proceso nuevo, puede allocar
    }

    int current_frames = get_process_frame_count(process_id);
    int max_allowed = it->second.allocated_frames;

    return current_frames < max_allowed;
}

int MemoryManager::pff_reclaim_frame_from_process(int receiver_pid) {
    int donor_pid = -1;
    int donor_frames = -1;

    for (const auto& entry : pff_process_info_) {
        if (entry.first == receiver_pid) {
            continue;
        }

        const auto& donor_info = entry.second;
        if (donor_info.allocated_frames <= donor_info.min_frames) {
            continue;
        }

        int actual = get_process_frame_count(entry.first);
        if (actual <= donor_info.min_frames) {
            continue;
        }

        if (actual > donor_frames) {
            donor_frames = actual;
            donor_pid = entry.first;
        }
    }

    if (donor_pid == -1) {
        return -1;
    }

    int victim = select_victim_pff(donor_pid);
    if (victim == -1) {
        return -1;
    }

    evict_page(victim);

    auto& donor_info = pff_process_info_[donor_pid];
    if (donor_info.allocated_frames > donor_info.min_frames) {
        donor_info.allocated_frames = std::max(donor_info.allocated_frames - 1, donor_info.min_frames);
    }

    std::cout << Color::YELLOW << "[PFF] Frame " << victim
              << " reclamado de P" << donor_pid << " para P" << receiver_pid
              << Color::RESET << std::endl;

    return victim;
}

int MemoryManager::get_process_frame_count(int process_id) const {
    int count = 0;
    for (const auto& frame : frames_) {
        if (frame.occupied && frame.process_id == process_id) {
            count++;
        }
    }
    return count;
}

void MemoryManager::display_pff_stats() const {
    if (policy_ != ReplacementPolicy::PFF) {
        std::cout << Color::YELLOW << "[PFF] No está activo (política actual: "
                  << ((policy_ == ReplacementPolicy::FIFO) ? "FIFO" : "LRU") << ")"
                  << Color::RESET << std::endl;
        return;
    }

    print_header("ESTADÍSTICAS PFF (Page Fault Frequency)");

    std::cout << " Configuración:" << std::endl;
    std::cout << "   ├─ Ventana:           " << pff_window_size_ << " accesos" << std::endl;
    std::cout << "   ├─ Umbral superior:   " << (pff_upper_threshold_ * 100) << "%" << std::endl;
    std::cout << "   ├─ Umbral inferior:   " << (pff_lower_threshold_ * 100) << "%" << std::endl;
    std::cout << "   ├─ Frames mín/proc:   " << pff_min_frames_per_process_ << std::endl;
    std::cout << "   └─ Frames máx/proc:   " << pff_max_frames_per_process_ << std::endl;

    if (pff_process_info_.empty()) {
        std::cout << "\n⚠  No hay procesos con estadísticas PFF\n" << std::endl;
        return;
    }

    std::cout << "\n Procesos activos:" << std::endl;
    print_separator(80);
    std::cout << std::left
              << std::setw(10) << "Proceso"
              << std::setw(12) << "Frames"
              << std::setw(15) << "Tasa Fallos"
              << std::setw(12) << "Accesos"
              << std::setw(12) << "Fallos"
              << std::setw(12) << "Estado"
              << std::endl;
    print_separator(80);

    for (const auto& entry : pff_process_info_) {
        int pid = entry.first;
        const auto& info = entry.second;
        int actual_frames = get_process_frame_count(pid);

        std::string status;
        std::string color;
        if (info.fault_rate > pff_upper_threshold_) {
            status = "ALTA";
            color = Color::RED;
        } else if (info.fault_rate < pff_lower_threshold_) {
            status = "BAJA";
            color = Color::GREEN;
        } else {
            status = "NORMAL";
            color = Color::YELLOW;
        }

        std::cout << color
                  << std::setw(10) << ("P" + std::to_string(pid))
                  << std::setw(12) << (std::to_string(actual_frames) + "/" + std::to_string(info.allocated_frames))
                  << std::setw(15) << (std::to_string((int)(info.fault_rate * 100)) + "%")
                  << std::setw(12) << info.recent_accesses
                  << std::setw(12) << info.recent_faults
                  << std::setw(12) << status
                  << Color::RESET << std::endl;
    }

    std::cout << std::endl;
}
