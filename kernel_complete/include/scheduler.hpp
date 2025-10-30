#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "process.hpp"
#include <vector>
#include <queue>
#include <memory>

class RoundRobinScheduler {
public:
    RoundRobinScheduler(int quantum);
    
    // Gestión de procesos
    void create_process(int burst_time);
    void kill_process(int pid);
    
    // Ejecución
    void tick();           // Ejecutar 1 unidad de tiempo
    void run(int n);       // Ejecutar N unidades
    
    // Visualización
    void list_processes() const;
    void show_stats() const;
    
    // Getters
    int get_current_time() const { return current_time_; }
    int get_quantum() const { return quantum_; }

private:
    int quantum_;                                    // Quantum de Round Robin
    int current_quantum_;                            // Quantum usado por proceso actual
    std::vector<std::shared_ptr<Process>> processes_; // Todos los procesos
    std::queue<std::shared_ptr<Process>> ready_queue_; // Cola de listos
    std::shared_ptr<Process> current_process_;       // Proceso en CPU
    int next_pid_;                                   // Siguiente PID a asignar
    int current_time_;                               // Reloj del sistema
    
    // Helpers internos
    void update_wait_times();
    void dispatch_next();
    void preempt_current();
};

#endif // SCHEDULER_HPP
