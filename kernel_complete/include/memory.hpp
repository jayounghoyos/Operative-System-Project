#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>
#include <map>
#include <queue>
#include <string>
#include <list>
#include <unordered_map>

// Estructura de un frame físico
struct Frame {
    int frame_id;
    int page_number;   // Qué página está cargada
    int process_id;    // A qué proceso pertenece
    bool occupied;
    int load_time;     // Para FIFO
    int last_used;     // Para LRU
};

// Estructura de entrada en page table
struct PageTableEntry {
    int frame_id;      // Marco físico asignado
    bool valid;        // Está en memoria física?
};

// Algoritmos de reemplazo soportados
enum class ReplacementPolicy {
    FIFO,
    LRU,
    PFF  // Page Fault Frequency - gestión avanzada
};

class MemoryManager {
public:
    explicit MemoryManager(int num_frames, ReplacementPolicy policy = ReplacementPolicy::FIFO);

    // Operaciones principales
    bool access_page(int process_id, int page_number);
    void display_frames() const;
    void display_stats() const;
    void display_page_table(int process_id) const;

    double get_hit_ratio() const;
    double get_fault_rate() const;

    // Reset stats
    void reset_stats();

    // Cambiar política en caliente
    void set_policy(ReplacementPolicy p);
    ReplacementPolicy get_policy() const { return policy_; }

    // PFF-specific operations
    void display_pff_stats() const;
    int get_process_frame_count(int process_id) const;

private:
    int num_frames_;                                    // Cantidad de frames
    std::vector<Frame> frames_;                         // Tabla de frames
    std::map<int, std::map<int, PageTableEntry>> page_tables_; // Page table por proceso

    // Estructuras para reemplazo
    std::queue<int> fifo_queue_;                        // Cola FIFO para reemplazo

    // LRU: lista doblemente enlazada con el frame más reciente al frente
    mutable std::list<int> lru_list_;                   // guarda frame_id
    // acceso O(1) a la posición del frame en la lista
    mutable std::unordered_map<int, std::list<int>::iterator> lru_pos_;

    // Configuración
    ReplacementPolicy policy_;

    // Estadísticas
    int total_accesses_;
    int page_faults_;
    int page_hits_;
    int current_time_;

    // Helpers internos
    int find_free_frame();
    int select_victim_fifo();
    int select_victim_lru();
    void load_page(int process_id, int page_number, int frame_id);
    void evict_page(int frame_id);

    // LRU helpers
    void lru_touch(int frame_id) const; // mover frame a frente (más reciente)
    void lru_erase(int frame_id) const; // quitar frame de estructuras LRU
    void lru_add(int frame_id) const;   // insertar frame como más reciente

    // PFF (Page Fault Frequency) - Gestión avanzada
    struct PFFProcessInfo {
        int allocated_frames;      // Frames asignados actualmente
        int min_frames;            // Mínimo de frames (working set mínimo)
        int max_frames;            // Máximo de frames permitido
        int recent_faults;         // Fallos en ventana actual
        int recent_accesses;       // Accesos en ventana actual
        int window_start_time;     // Inicio de ventana de medición
        double fault_rate;         // Tasa de fallos calculada
    };

    std::map<int, PFFProcessInfo> pff_process_info_;  // Info PFF por proceso
    int pff_window_size_;          // Tamaño de ventana de medición (accesos)
    double pff_upper_threshold_;   // Umbral superior (e.g., 0.6 = 60% fallos)
    double pff_lower_threshold_;   // Umbral inferior (e.g., 0.1 = 10% fallos)
    int pff_min_frames_per_process_; // Mínimo frames por proceso
    int pff_max_frames_per_process_; // Máximo frames por proceso

    // PFF helpers
    void pff_initialize_process(int process_id);
    bool pff_update_stats(int process_id, bool was_fault);
    void pff_adjust_allocation(int process_id);
    int select_victim_pff(int process_id);
    bool pff_can_allocate_frame(int process_id) const;
    int pff_reclaim_frame_from_process(int receiver_pid);
};

#endif // MEMORY_HPP
