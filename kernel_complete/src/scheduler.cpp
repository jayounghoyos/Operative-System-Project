#include "../include/scheduler.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>


// ROUND ROBIN


RoundRobinScheduler::RoundRobinScheduler(int quantum)
    : quantum_(quantum),
      current_quantum_(0),
      current_process_(nullptr),
      next_pid_(1),
      current_time_(0) {}

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
    int blocked = 0;

    for (const auto& proc : processes_) {
        if (proc->get_state() == ProcessState::TERMINATED) {
            total_wait += proc->get_wait_time();
            total_turnaround += proc->get_turnaround_time();
            completed++;
        } else if (proc->get_state() == ProcessState::RUNNING) {
            running++;
        } else if (proc->get_state() == ProcessState::READY) {
            ready++;
        } else if (proc->get_state() == ProcessState::BLOCKED) {
            blocked++;
        }
    }

    std::cout << " Tiempo actual:         " << current_time_ << std::endl;
    std::cout << " Quantum:               " << quantum_ << std::endl;
    std::cout << " Procesos totales:      " << processes_.size() << std::endl;
    std::cout << "  ├─ En ejecución:      " << running << std::endl;
    std::cout << "  ├─ Listos (READY):    " << ready << std::endl;
    std::cout << "  ├─ Suspendidos:       " << blocked << std::endl;
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

bool RoundRobinScheduler::suspend_process(int pid) {
    auto it = std::find_if(processes_.begin(), processes_.end(),
                           [pid](const auto& p) { return p->get_pid() == pid; });

    if (it == processes_.end()) {
        std::cout << Color::RED << "[ERROR] Proceso P" << pid
                  << " no encontrado" << Color::RESET << std::endl;
        return false;
    }

    auto& proc = *it;
    auto state = proc->get_state();

    if (state == ProcessState::TERMINATED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " ya terminó; no se puede suspender" << Color::RESET << std::endl;
        return false;
    }
    if (state == ProcessState::BLOCKED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " ya está suspendido" << Color::RESET << std::endl;
        return false;
    }

    if (state == ProcessState::RUNNING) {
        proc->set_state(ProcessState::BLOCKED);
        current_process_ = nullptr;
        current_quantum_ = 0;

        std::cout << Color::BLUE << "[SUSPEND] P" << pid
                  << " suspendido desde CPU" << Color::RESET << std::endl;
    } else if (state == ProcessState::READY) {
        std::queue<std::shared_ptr<Process>> rebuilt;
        while (!ready_queue_.empty()) {
            auto candidate = ready_queue_.front();
            ready_queue_.pop();
            if (candidate->get_pid() == pid) continue;
            rebuilt.push(candidate);
        }
        ready_queue_ = std::move(rebuilt);

        proc->set_state(ProcessState::BLOCKED);
        std::cout << Color::BLUE << "[SUSPEND] P" << pid
                  << " suspendido (se retira de READY)" << Color::RESET << std::endl;
    } else {
        // Procesos recién creados deberían estar como READY, pero manejamos por si acaso.
        proc->set_state(ProcessState::BLOCKED);
        std::cout << Color::BLUE << "[SUSPEND] P" << pid
                  << " marcado como suspendido" << Color::RESET << std::endl;
    }
    return true;
}

bool RoundRobinScheduler::resume_process(int pid) {
    auto it = std::find_if(processes_.begin(), processes_.end(),
                           [pid](const auto& p) { return p->get_pid() == pid; });

    if (it == processes_.end()) {
        std::cout << Color::RED << "[ERROR] Proceso P" << pid
                  << " no encontrado" << Color::RESET << std::endl;
        return false;
    }

    auto& proc = *it;
    auto state = proc->get_state();

    if (state == ProcessState::TERMINATED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " ya terminó; no se puede reanudar" << Color::RESET << std::endl;
        return false;
    }
    if (state != ProcessState::BLOCKED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " no está suspendido" << Color::RESET << std::endl;
        return false;
    }

    proc->set_state(ProcessState::READY);
    ready_queue_.push(proc);

    std::cout << Color::GREEN << "[RESUME] P" << pid
              << " reanudado y enviado a READY" << Color::RESET << std::endl;
    return true;
}


// SJF NO EXPROPIATIVO


SJFScheduler::SJFScheduler()
    : current_process_(nullptr), next_pid_(1), current_time_(0) {}

void SJFScheduler::create_process(int burst_time) {
    auto p = std::make_shared<Process>(next_pid_++, burst_time);
    p->set_state(ProcessState::READY);
    p->set_arrival_time(current_time_);
    processes_.push_back(p);
    ready_pq_.push(p);

    std::cout << Color::GREEN << "[t=" << current_time_ << "] "
              << "Proceso P" << p->get_pid()
              << " creado (burst=" << burst_time << ", SJF)"
              << Color::RESET << std::endl;
}

void SJFScheduler::dispatch_next() {
    if (!ready_pq_.empty()) {
        current_process_ = ready_pq_.top();
        ready_pq_.pop();
        current_process_->set_state(ProcessState::RUNNING);
        std::cout << Color::CYAN << "[t=" << current_time_ << "] "
                  << "DISPATCH (SJF) → P" << current_process_->get_pid()
                  << " entra a CPU"
                  << Color::RESET << std::endl;
    }
}

void SJFScheduler::tick() {
    current_time_++;

    if (current_process_ == nullptr) {
        dispatch_next();
    }

    if (current_process_ != nullptr) {
        current_process_->execute();
        if (current_process_->get_state() == ProcessState::TERMINATED) {
            current_process_->calculate_turnaround(current_time_);
            std::cout << Color::RED << "[t=" << current_time_ << "] "
                      << "P" << current_process_->get_pid()
                      << " TERMINADO (TAT="
                      << current_process_->get_turnaround_time() << ")"
                      << Color::RESET << std::endl;
            current_process_ = nullptr; // CPU libre; el próximo tick tomará el más corto
        }
    }

    update_wait_times();
}

void SJFScheduler::run(int n) {
    std::cout << Color::BLUE << Color::BOLD
              << "\n▶ Ejecutando (SJF) " << n << " ticks...\n"
              << Color::RESET << std::endl;
    for (int i = 0; i < n; ++i) tick();
    std::cout << Color::BLUE << Color::BOLD
              << "\n Simulación SJF completada (t=" << current_time_ << ")\n"
              << Color::RESET << std::endl;
}

void SJFScheduler::update_wait_times() {
    for (auto& p : processes_) {
        if (p->get_state() == ProcessState::READY) p->increment_wait_time();
    }
}

void SJFScheduler::list_processes() const {
    print_header("PROCESOS (SJF, t=" + std::to_string(current_time_) + ")");

    std::cout << std::left
              << std::setw(6)  << "PID"
              << std::setw(12) << "Estado"
              << std::setw(8)  << "Burst"
              << std::setw(10) << "Restante"
              << std::setw(10) << "Espera"
              << std::setw(12) << "Turnaround"
              << std::endl;
    print_separator(60);

    for (const auto& p : processes_) p->print_info();
    std::cout << std::endl;
}

void SJFScheduler::show_stats() const {
    print_header("ESTADÍSTICAS DE SCHEDULER (SJF)");

    int total_wait = 0, total_tat = 0;
    int completed = 0, running = 0, ready = 0, blocked = 0;

    for (const auto& p : processes_) {
        if (p->get_state() == ProcessState::TERMINATED) {
            total_wait += p->get_wait_time();
            total_tat  += p->get_turnaround_time();
            completed++;
        } else if (p->get_state() == ProcessState::RUNNING) {
            running++;
        } else if (p->get_state() == ProcessState::READY) {
            ready++;
        } else if (p->get_state() == ProcessState::BLOCKED) {
            blocked++;
        }
    }

    std::cout << " Tiempo actual:         " << current_time_ << "\n"
              << " Algoritmo:             SJF (no expropiativo)\n"
              << " Procesos totales:      " << processes_.size() << "\n"
              << "  ├─ En ejecución:      " << running << "\n"
              << "  ├─ Listos (READY):    " << ready << "\n"
              << "  ├─ Suspendidos:       " << blocked << "\n"
              << "  └─ Terminados:        " << completed << "\n";

    if (completed > 0) {
        std::cout << std::fixed << std::setprecision(2)
                  << "\nFairness:\n"
                  << "   ├─ Espera promedio:     " << (double)total_wait / completed << " unidades\n"
                  << "   └─ Turnaround promedio: " << (double)total_tat  / completed << " unidades\n";
    }
    std::cout << std::endl;
}

void SJFScheduler::kill_process(int pid) {
    auto it = std::find_if(processes_.begin(), processes_.end(),
                           [pid](const auto& p){ return p->get_pid() == pid; });

    if (it == processes_.end()) {
        std::cout << Color::RED << "[ERROR] Proceso P" << pid
                  << " no encontrado" << Color::RESET << std::endl;
        return;
    }

    if (current_process_ && current_process_->get_pid() == pid) {
        current_process_.reset();
    }

    (*it)->set_state(ProcessState::TERMINATED);
    (*it)->calculate_turnaround(current_time_);
    std::cout << Color::RED << "[KILL] Proceso P" << pid
              << " terminado forzosamente (SJF)" << Color::RESET << std::endl;
}

bool SJFScheduler::suspend_process(int pid) {
    auto it = std::find_if(processes_.begin(), processes_.end(),
                           [pid](const auto& p){ return p->get_pid() == pid; });

    if (it == processes_.end()) {
        std::cout << Color::RED << "[ERROR] Proceso P" << pid
                  << " no encontrado" << Color::RESET << std::endl;
        return false;
    }

    auto& proc = *it;
    auto state = proc->get_state();

    if (state == ProcessState::TERMINATED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " ya terminó; no se puede suspender" << Color::RESET << std::endl;
        return false;
    }
    if (state == ProcessState::BLOCKED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " ya está suspendido" << Color::RESET << std::endl;
        return false;
    }

    if (state == ProcessState::RUNNING) {
        proc->set_state(ProcessState::BLOCKED);
        current_process_.reset();

        std::cout << Color::BLUE << "[SUSPEND] P" << pid
                  << " suspendido desde CPU (SJF)" << Color::RESET << std::endl;
    } else if (state == ProcessState::READY) {
        std::vector<std::shared_ptr<Process>> buffer;
        while (!ready_pq_.empty()) {
            auto candidate = ready_pq_.top();
            ready_pq_.pop();
            if (candidate->get_pid() == pid) continue;
            buffer.push_back(candidate);
        }
        for (const auto& candidate : buffer) ready_pq_.push(candidate);

        proc->set_state(ProcessState::BLOCKED);
        std::cout << Color::BLUE << "[SUSPEND] P" << pid
                  << " suspendido (se retira de READY SJF)" << Color::RESET << std::endl;
    } else {
        proc->set_state(ProcessState::BLOCKED);
        std::cout << Color::BLUE << "[SUSPEND] P" << pid
                  << " marcado como suspendido (SJF)" << Color::RESET << std::endl;
    }
    return true;
}

bool SJFScheduler::resume_process(int pid) {
    auto it = std::find_if(processes_.begin(), processes_.end(),
                           [pid](const auto& p){ return p->get_pid() == pid; });

    if (it == processes_.end()) {
        std::cout << Color::RED << "[ERROR] Proceso P" << pid
                  << " no encontrado" << Color::RESET << std::endl;
        return false;
    }

    auto& proc = *it;
    auto state = proc->get_state();

    if (state == ProcessState::TERMINATED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " ya terminó; no se puede reanudar" << Color::RESET << std::endl;
        return false;
    }
    if (state != ProcessState::BLOCKED) {
        std::cout << Color::YELLOW << "[WARN] Proceso P" << pid
                  << " no está suspendido" << Color::RESET << std::endl;
        return false;
    }

    proc->set_state(ProcessState::READY);
    ready_pq_.push(proc);

    std::cout << Color::GREEN << "[RESUME] P" << pid
              << " reanudado y enviado a READY (SJF)" << Color::RESET << std::endl;
    return true;
}
