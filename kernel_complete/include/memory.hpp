#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>
#include <map>
#include <queue>
#include <string>

// Estructura de un frame físico
struct Frame {
    int frame_id;
    int page_number;   // Qué página está cargada
    int process_id;    // A qué proceso pertenece
    bool occupied;
    int load_time;     // Para FIFO
};

// Estructura de entrada en page table
struct PageTableEntry {
    int frame_id;      // Marco físico asignado
    bool valid;        // Está en memoria física?
};

class MemoryManager {
public:
    MemoryManager(int num_frames);
    
    // Operaciones principales
    bool access_page(int process_id, int page_number);
    void display_frames() const;
    void display_stats() const;
    void display_page_table(int process_id) const;
    
    double get_hit_ratio() const;
    double get_fault_rate() const;
    
    // Reset stats
    void reset_stats();

private:
    int num_frames_;                                    // Cantidad de frames
    std::vector<Frame> frames_;                         // Tabla de frames
    std::map<int, std::map<int, PageTableEntry>> page_tables_; // Page table por proceso
    std::queue<int> fifo_queue_;                        // Cola FIFO para reemplazo
    
    // Estadísticas
    int total_accesses_;
    int page_faults_;
    int page_hits_;
    int current_time_;
    
    // Helpers internos
    int find_free_frame();
    int select_victim_fifo();
    void load_page(int process_id, int page_number, int frame_id);
    void evict_page(int frame_id);
};

#endif // MEMORY_HPP
