#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "process.hpp"
#include <vector>
#include <queue>
#include <memory>
#include <functional>

// -----------------------------------------------------------------------------
// Interfaz común (opcional) para planificadores.
// Puedes usar punteros a IScheduler en main.cpp para alternar RR/SJF.
// -----------------------------------------------------------------------------
class IScheduler {
public:
    virtual ~IScheduler() = default;

    // Gestión de procesos
    virtual void create_process(int burst_time) = 0;
    virtual void kill_process(int pid) = 0;

    // Ejecución
    virtual void tick() = 0;     // Ejecutar 1 unidad de tiempo
    virtual void run(int n) = 0; // Ejecutar N unidades

    // Visualización
    virtual void list_processes() const = 0;
    virtual void show_stats() const = 0;

    // Getters
    virtual int get_current_time() const = 0;
};

// -----------------------------------------------------------------------------
// ROUND ROBIN (tu implementación original; mantiene la misma API pública)
// -----------------------------------------------------------------------------
class RoundRobinScheduler : public IScheduler {
public:
    explicit RoundRobinScheduler(int quantum);
    
    // Gestión de procesos
    void create_process(int burst_time) override;
    void kill_process(int pid) override;
    
    // Ejecución
    void tick() override;           // Ejecutar 1 unidad de tiempo
    void run(int n) override;       // Ejecutar N unidades
    
    // Visualización
    void list_processes() const override;
    void show_stats() const override;
    
    // Getters
    int get_current_time() const override { return current_time_; }
    int get_quantum() const { return quantum_; }

private:
    int quantum_;                                     // Quantum de Round Robin
    int current_quantum_;                             // Quantum usado por proceso actual
    std::vector<std::shared_ptr<Process>> processes_; // Todos los procesos
    std::queue<std::shared_ptr<Process>> ready_queue_; // Cola de listos
    std::shared_ptr<Process> current_process_;        // Proceso en CPU
    int next_pid_;                                    // Siguiente PID a asignar
    int current_time_;                                // Reloj del sistema
    
    // Helpers internos
    void update_wait_times();
    void dispatch_next();
    void preempt_current();
};

// -----------------------------------------------------------------------------
// SJF NO EXPROPIATIVO
//  - Selecciona siempre el proceso con MENOR remaining_time cuando la CPU queda libre.
//  - No expulsa al proceso actual hasta que termine.
// -----------------------------------------------------------------------------
class SJFScheduler : public IScheduler {
public:
    SJFScheduler();

    // Gestión de procesos
    void create_process(int burst_time) override;
    void kill_process(int pid) override;

    // Ejecución
    void tick() override;     // Ejecuta 1 unidad (sin expulsión)
    void run(int n) override; // Ejecuta n unidades

    // Visualización
    void list_processes() const override;
    void show_stats() const override;

    // Getters
    int get_current_time() const override { return current_time_; }

private:
    // Comparador para la priority_queue: menor remaining_time primero.
    struct Cmp {
        bool operator()(const std::shared_ptr<Process>& a,
                        const std::shared_ptr<Process>& b) const {
            if (a->get_remaining_time() == b->get_remaining_time())
                return a->get_pid() > b->get_pid(); // desempate por PID
            return a->get_remaining_time() > b->get_remaining_time();
        }
    };

    std::priority_queue<
        std::shared_ptr<Process>,
        std::vector<std::shared_ptr<Process>>,
        Cmp
    > ready_pq_;

    std::vector<std::shared_ptr<Process>> processes_;
    std::shared_ptr<Process> current_process_;
    int next_pid_;
    int current_time_;

    // Helpers
    void update_wait_times();
    void dispatch_next();   // Toma el de menor remaining_time
};

#endif // SCHEDULER_HPP