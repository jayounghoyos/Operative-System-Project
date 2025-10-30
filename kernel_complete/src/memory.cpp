#include "../include/memory.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

MemoryManager::MemoryManager(int num_frames)
    : num_frames_(num_frames),
      total_accesses_(0),
      page_faults_(0),
      page_hits_(0),
      current_time_(0) {
    
    // Inicializar frames
    frames_.resize(num_frames);
    for (int i = 0; i < num_frames; i++) {
        frames_[i].frame_id = i;
        frames_[i].page_number = -1;
        frames_[i].process_id = -1;
        frames_[i].occupied = false;
        frames_[i].load_time = -1;
    }
    
    std::cout << Color::GREEN << "[MEMORY] Inicializada con " 
              << num_frames << " frames" << Color::RESET << std::endl;
}

bool MemoryManager::access_page(int process_id, int page_number) {
    total_accesses_++;
    current_time_++;
    
    // Verificar si la página ya está en memoria (HIT)
    if (page_tables_[process_id].count(page_number) > 0 && 
        page_tables_[process_id][page_number].valid) {
        page_hits_++;
        int frame_id = page_tables_[process_id][page_number].frame_id;
        
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
    
    // Si no hay frame libre, usar FIFO para seleccionar víctima
    if (frame_id == -1) {
        frame_id = select_victim_fifo();
        std::cout << Color::RED << "  └─ Evictando frame " << frame_id 
                  << " (FIFO)" << Color::RESET << std::endl;
        evict_page(frame_id);
    }
    
    // Cargar la página en el frame
    load_page(process_id, page_number, frame_id);
    
    std::cout << Color::CYAN << "  └─ Página cargada en frame " << frame_id
              << Color::RESET << std::endl;
    
    return false;
}

int MemoryManager::find_free_frame() {
    for (int i = 0; i < num_frames_; i++) {
        if (!frames_[i].occupied) {
            return i;
        }
    }
    return -1; // No hay frames libres
}

int MemoryManager::select_victim_fifo() {
    // FIFO: seleccionar el frame más antiguo (primero en la cola)
    if (fifo_queue_.empty()) {
        return 0; // Fallback (no debería pasar)
    }
    
    int victim = fifo_queue_.front();
    fifo_queue_.pop();
    return victim;
}

void MemoryManager::load_page(int process_id, int page_number, int frame_id) {
    // Actualizar frame
    frames_[frame_id].occupied = true;
    frames_[frame_id].page_number = page_number;
    frames_[frame_id].process_id = process_id;
    frames_[frame_id].load_time = current_time_;
    
    // Agregar a cola FIFO
    fifo_queue_.push(frame_id);
    
    // Actualizar page table
    page_tables_[process_id][page_number].frame_id = frame_id;
    page_tables_[process_id][page_number].valid = true;
}

void MemoryManager::evict_page(int frame_id) {
    if (frames_[frame_id].occupied) {
        int old_process = frames_[frame_id].process_id;
        int old_page = frames_[frame_id].page_number;
        
        // Invalidar entrada en page table
        page_tables_[old_process][old_page].valid = false;
        
        // Marcar frame como libre
        frames_[frame_id].occupied = false;
        frames_[frame_id].process_id = -1;
        frames_[frame_id].page_number = -1;
    }
}

void MemoryManager::display_frames() const {
    print_header("ESTADO DE FRAMES");
    
    std::cout << std::left
              << std::setw(10) << "Frame ID"
              << std::setw(12) << "Estado"
              << std::setw(12) << "Proceso"
              << std::setw(12) << "Página"
              << std::setw(15) << "Load Time"
              << std::endl;
    print_separator(60);
    
    for (const auto& frame : frames_) {
        std::string status_color = frame.occupied ? Color::GREEN : Color::WHITE;
        std::string status = frame.occupied ? "OCUPADO" : "LIBRE";
        
        std::cout << status_color
                  << std::setw(10) << frame.frame_id
                  << std::setw(12) << status;
        
        if (frame.occupied) {
            std::cout << std::setw(12) << ("P" + std::to_string(frame.process_id))
                      << std::setw(12) << frame.page_number
                      << std::setw(15) << frame.load_time;
        } else {
            std::cout << std::setw(12) << "-"
                      << std::setw(12) << "-"
                      << std::setw(15) << "-";
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
    
    std::cout << "\nAlgoritmo:             FIFO (First-In-First-Out)" << std::endl;
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
    return (double)page_hits_ / total_accesses_;
}

double MemoryManager::get_fault_rate() const {
    if (total_accesses_ == 0) return 0.0;
    return (double)page_faults_ / total_accesses_;
}

void MemoryManager::reset_stats() {
    total_accesses_ = 0;
    page_faults_ = 0;
    page_hits_ = 0;
    current_time_ = 0;
    std::cout << Color::CYAN << "[MEMORY] Estadísticas reiniciadas" 
              << Color::RESET << std::endl;
}
