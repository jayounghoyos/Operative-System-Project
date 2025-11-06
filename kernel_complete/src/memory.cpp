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
      current_time_(0)
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

    std::string pol = (policy_ == ReplacementPolicy::FIFO) ? "FIFO" : "LRU";
    std::cout << Color::GREEN << "[MEMORY] Inicializada con "
              << num_frames_ << " frames (política=" << pol << ")"
              << Color::RESET << std::endl;
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

    std::string pol = (policy_ == ReplacementPolicy::FIFO) ? "FIFO" : "LRU";
    std::cout << Color::CYAN << "[MEMORY] Política cambiada a " << pol
              << Color::RESET << std::endl;
}

bool MemoryManager::access_page(int process_id, int page_number) {
    total_accesses_++;
    current_time_++;

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

        std::cout << Color::GREEN << "[HIT] "
                  << "P" << process_id << " página " << page_number
                  << " → frame " << frame_id
                  << " (hits=" << page_hits_ << ")"
                  << Color::RESET << std::endl;
        return true;
    }

    // PAGE FAULT
    page_faults_++;
    std::cout << Color::YELLOW << "[PAGE FAULT #" << page_faults_ << "] "
              << "P" << process_id << " página " << page_number
              << Color::RESET << std::endl;

    // Buscar frame libre
    int frame_id = find_free_frame();

    // Si no hay, seleccionar víctima según política
    if (frame_id == -1) {
        if (policy_ == ReplacementPolicy::FIFO) {
            frame_id = select_victim_fifo();
            std::cout << Color::RED << "  └─ Evictando frame " << frame_id
                      << " (FIFO)" << Color::RESET << std::endl;
        } else {
            frame_id = select_victim_lru();
            std::cout << Color::RED << "  └─ Evictando frame " << frame_id
                      << " (LRU)" << Color::RESET << std::endl;
        }
        evict_page(frame_id);
    }

    // Cargar
    load_page(process_id, page_number, frame_id);

    std::cout << Color::CYAN << "  └─ Página cargada en frame " << frame_id
              << Color::RESET << std::endl;

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

    std::cout << "\nAlgoritmo:             "
              << ((policy_ == ReplacementPolicy::FIFO) ? "FIFO (First-In-First-Out)"
                                                        : "LRU (Least Recently Used)")
              << std::endl;
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

/* ===================== LRU helpers ===================== */

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