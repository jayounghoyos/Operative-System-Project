#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <string>

enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

class Process {
public:
    Process(int pid, int burst_time);
    
    // Getters
    int get_pid() const { return pid_; }
    ProcessState get_state() const { return state_; }
    int get_burst_time() const { return burst_time_; }
    int get_remaining_time() const { return remaining_time_; }
    int get_wait_time() const { return wait_time_; }
    int get_turnaround_time() const { return turnaround_time_; }
    int get_arrival_time() const { return arrival_time_; }
    
    // Setters
    void set_state(ProcessState state) { state_ = state; }
    void set_arrival_time(int time) { arrival_time_ = time; }
    
    // Ejecutar el proceso por 1 unidad de tiempo
    void execute();
    
    // Incrementar tiempo de espera
    void increment_wait_time() { wait_time_++; }
    
    // Calcular turnaround al terminar
    void calculate_turnaround(int current_time);
    
    // Estado como string
    std::string state_to_string() const;
    
    // Para debug
    void print_info() const;

private:
    int pid_;
    ProcessState state_;
    int burst_time_;        // Tiempo total CPU necesario
    int remaining_time_;    // Tiempo restante
    int wait_time_;         // Tiempo en cola READY
    int turnaround_time_;   // Tiempo total en sistema
    int arrival_time_;      // Momento de creación
};

#endif // PROCESS_HPP
