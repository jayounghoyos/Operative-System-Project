#ifndef SYNC_HPP
#define SYNC_HPP

#include <mutex>
#include <condition_variable>
#include <vector>

class ProducerConsumer {
public:
    ProducerConsumer(int buffer_size);
    
    // Operaciones principales
    bool produce(int item);     // Retorna true si tuvo éxito
    bool consume(int& item);    // Retorna true si tuvo éxito, item por referencia
    
    // Visualización
    void display_buffer() const;
    void display_stats() const;
    
    // Control
    void reset();
    
private:
    int buffer_size_;                  // Tamaño del buffer
    std::vector<int> buffer_;          // Buffer circular
    int count_;                        // Elementos actuales en buffer
    int in_;                           // Índice de inserción
    int out_;                          // Índice de extracción
    
    // Sincronización
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    
    // Estadísticas
    int total_produced_;
    int total_consumed_;
    int producer_blocks_;
    int consumer_blocks_;
};

#endif // SYNC_HPP
