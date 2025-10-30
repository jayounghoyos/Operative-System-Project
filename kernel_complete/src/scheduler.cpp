#include "../include/scheduler.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

RoundRobinScheduler::RoundRobinScheduler(int quantum)
    : quantum_(quantum),
      current_quantum_(0),
      current_process_(nullptr),
      next_pid_(1),
      current_time_(0) {
}

void RoundRobinScheduler::create_process(int burst_time) {
    auto process = std::make_shared<Process>(next_pid_++, burst_time);
    process->set_state(ProcessState::READY);
    process->set_arrival_time(current_time_);
    processes_.push_back(process);
    ready_queue_.push(process);
    
    std::cout << Color::GREEN << "[t=" << current_time_ << "] "
              << "Proceso P" << process->get_pid() 
              << " creado (burst=" << burst_time << ")" 
              << Color::RESET << std::endl;
}

void RoundRobinScheduler::tick() {
    current_time_++;
    
    // Si no hay proceso en CPU, hacer dispatch
    if (current_process_ == nullptr && !ready_queue_.empty()) {
        dispatch_next();
    }
    
    // Ejecutar el proceso en CPU
    if (current_process_ != nullptr) {
        current_process_->execute();
        current_quantum_++;
        
        // Verificar si terminó
        if (current_process_->get_state() == ProcessState::TERMINATED) {
            current_process_->calculate_turnaround(current_time_);
            std::cout << Color::RED << "[t=" << current_time_ << "] "
                      << "P" << current_process_->get_pid() << " TERMINADO"
                      << " (TAT=" << current_process_->get_turnaround_time() << ")"
                      << Color::RESET << std::endl;
            current_process_ = nullptr;
            current_quantum_ = 0;
        }
        // Verificar quantum expirado
        else if (current_quantum_ >= quantum_) {
            std::cout << Color::YELLOW << "[t=" << current_time_ << "] "
                      << "P" << current_process_->get_pid() 
                      << " QUANTUM EXPIRADO (restante=" 
                      << current_process_->get_remaining_time() << ")"
                      << Color::RESET << std::endl;
            preempt_current();
        }
    }
    
    // Actualizar tiempos de espera
    update_wait_times();
}

void RoundRobinScheduler::run(int n) {
    std::cout << Color::BLUE << Color::BOLD 
              << "\n▶ Ejecutando " << n << " ticks...\n" 
              << Color::RESET << std::endl;
    
    for (int i = 0; i < n; i++) {
        tick();
    }
    
    std::cout << Color::BLUE << Color::BOLD 
              << "\n Simulación completada (t=" << current_time_ << ")\n" 
              << Color::RESET << std::endl;
}

void RoundRobinScheduler::dispatch_next() {
    if (!ready_queue_.empty()) {
        current_process_ = ready_queue_.front();
        ready_queue_.pop();
        current_process_->set_state(ProcessState::RUNNING);
        current_quantum_ = 0;
        
        std::cout << Color::CYAN << "[t=" << current_time_ << "] "
                  << "DISPATCH → P" << current_process_->get_pid() 
                  << " entra en CPU"
                  << Color::RESET << std::endl;
    }
}

void RoundRobinScheduler::preempt_current() {
    if (current_process_ != nullptr) {
        current_process_->set_state(ProcessState::READY);
        ready_queue_.push(current_process_);
        current_process_ = nullptr;
        current_quantum_ = 0;
    }
}

void RoundRobinScheduler::update_wait_times() {
    for (auto& proc : processes_) {
        if (proc->get_state() == ProcessState::READY) {
            proc->increment_wait_time();
        }
    }
}

void RoundRobinScheduler::list_processes() const {
    print_header("PROCESOS (t=" + std::to_string(current_time_) + ")");
    
    std::cout << std::left 
              << std::setw(6) << "PID"
              << std::setw(12) << "Estado"
              << std::setw(8) << "Burst"
              << std::setw(10) << "Restante"
              << std::setw(10) << "Espera"
              << std::setw(12) << "Turnaround"
              << std::endl;
    print_separator(60);
    
    for (const auto& proc : processes_) {
        proc->print_info();
    }
    std::cout << std::endl;
}

void RoundRobinScheduler::show_stats() const {
    print_header("ESTADÍSTICAS DE SCHEDULER");
    
    int total_wait = 0;
    int total_turnaround = 0;
    int completed = 0;
    int running = 0;
    int ready = 0;
    
    for (const auto& proc : processes_) {
        if (proc->get_state() == ProcessState::TERMINATED) {
            total_wait += proc->get_wait_time();
            total_turnaround += proc->get_turnaround_time();
            completed++;
        } else if (proc->get_state() == ProcessState::RUNNING) {
            running++;
        } else if (proc->get_state() == ProcessState::READY) {
            ready++;
        }
    }
    
    std::cout << " Tiempo actual:         " << current_time_ << std::endl;
    std::cout << " Quantum:               " << quantum_ << std::endl;
    std::cout << " Procesos totales:      " << processes_.size() << std::endl;
    std::cout << "  ├─ En ejecución:      " << running << std::endl;
    std::cout << "  ├─ Listos (READY):    " << ready << std::endl;
    std::cout << "  └─ Terminados:        " << completed << std::endl;
    
    if (completed > 0) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\nFairness:" << std::endl;
        std::cout << "   ├─ Espera promedio:    " 
                  << (double)total_wait / completed << " unidades" << std::endl;
        std::cout << "   └─ Turnaround promedio: " 
                  << (double)total_turnaround / completed << " unidades" << std::endl;
    }
    std::cout << std::endl;
}

void RoundRobinScheduler::kill_process(int pid) {
    auto it = std::find_if(processes_.begin(), processes_.end(),
                          [pid](const auto& p) { return p->get_pid() == pid; });
    
    if (it != processes_.end()) {
        (*it)->set_state(ProcessState::TERMINATED);
        (*it)->calculate_turnaround(current_time_);
        
        if (current_process_ && current_process_->get_pid() == pid) {
            current_process_ = nullptr;
            current_quantum_ = 0;
        }
        
        std::cout << Color::RED << "[KILL] Proceso P" << pid 
                  << " terminado forzosamente" << Color::RESET << std::endl;
    } else {
        std::cout << Color::RED << "[ERROR] Proceso P" << pid 
                  << " no encontrado" << Color::RESET << std::endl;
    }
}
