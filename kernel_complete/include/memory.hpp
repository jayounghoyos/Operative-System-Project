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
    LRU
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

    // Cambiar política en caliente (opcional)
    void set_policy(ReplacementPolicy p);
    ReplacementPolicy get_policy() const { return policy_; }

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
};

#endif // MEMORY_HPP