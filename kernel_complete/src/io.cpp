#include "../include/io.hpp"
#include "../include/utils.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

IOManager::IOManager()
    : current_time_(0) {}

bool IOManager::add_device(const std::string& name, int spool_capacity) {
    // Decidimos validar aquí mismo para mantener mensajes homogéneos
    // sin delegar la responsabilidad en la CLI.
    if (spool_capacity <= 0) {
        std::cout << Color::RED << "[IO] Capacidad debe ser > 0"
                  << Color::RESET << std::endl;
        return false;
    }

    if (devices_.count(name) > 0) {
        std::cout << Color::YELLOW << "[IO] Dispositivo '" << name
                  << "' ya existe" << Color::RESET << std::endl;
        return false;
    }

    IODevice device;
    device.name = name;
    device.spool_capacity = spool_capacity;
    device.next_request_id = 1;
    device.total_requests = 0;
    device.completed_requests = 0;
    device.total_wait_time = 0;
    device.total_busy_time = 0;

    devices_[name] = device;

    std::cout << Color::GREEN << "[IO] Dispositivo '" << name
              << "' registrado (spool=" << spool_capacity << ")"
              << Color::RESET << std::endl;
    return true;
}

bool IOManager::device_exists(const std::string& name) const {
    return devices_.count(name) > 0;
}

IOManager::IODevice* IOManager::get_device(const std::string& name) {
    auto it = devices_.find(name);
    if (it == devices_.end()) return nullptr;
    return &it->second;
}

const IOManager::IODevice* IOManager::get_device(const std::string& name) const {
    auto it = devices_.find(name);
    if (it == devices_.end()) return nullptr;
    return &it->second;
}

bool IOManager::submit_request(const std::string& device_name,
                               int pid,
                               int duration,
                               int priority,
                               std::string& error_message) {
    // Centralizamos la validación para poder reutilizarla tanto
    // desde la CLI como desde posibles pruebas.
    error_message.clear();

    auto* device = get_device(device_name);
    if (!device) {
        error_message = "Dispositivo no encontrado";
        return false;
    }

    if (duration <= 0) {
        error_message = "Duración debe ser > 0";
        return false;
    }

    if (static_cast<int>(device->waiting_queue.size()) >= device->spool_capacity) {
        error_message = "Spool lleno, intenta más tarde";
        return false;
    }

    IORequest request;
    request.request_id = device->next_request_id++;
    request.pid = pid;
    request.duration = duration;
    request.remaining_time = duration;
    request.priority = priority;
    request.arrival_time = current_time_;
    request.start_time = -1;

    device->waiting_queue.push_back(request);
    device->total_requests++;

    std::cout << Color::CYAN << "[IO] Solicitud #" << request.request_id
              << " agregada a '" << device_name << "' (P" << pid
              << ", dur=" << duration << ", prio=" << priority << ")"
              << Color::RESET << std::endl;

    return true;
}

void IOManager::assign_next_request(IODevice& device) {
    if (device.current_request.has_value()) return;
    if (device.waiting_queue.empty()) return;

    // Seleccionamos la mejor solicitud usando prioridad y FIFO,
    // tal como acordamos para simular un spool con prioridades.
    auto best_it = std::max_element(
        device.waiting_queue.begin(),
        device.waiting_queue.end(),
        [](const IORequest& a, const IORequest& b) {
            if (a.priority == b.priority)
                return a.arrival_time > b.arrival_time;
            return a.priority < b.priority; // mayor prioridad primero
        });

    IORequest next = *best_it;
    device.waiting_queue.erase(best_it);
    next.start_time = current_time_;
    device.total_wait_time += next.start_time - next.arrival_time;
    device.current_request = next;

    std::cout << Color::BLUE << "[IO] " << device.name
              << " procesando solicitud #" << next.request_id
              << " (P" << next.pid << ")" << Color::RESET << std::endl;
}

std::vector<std::pair<int, std::string>>
IOManager::process_ticks(int ticks) {
    // Nos apoyamos en esta función para simular el paso del tiempo
    // sin acoplarlo al tick de CPU; así podemos correr I/O en batch.
    std::vector<std::pair<int, std::string>> completions;
    if (ticks <= 0) return completions;

    if (devices_.empty()) {
        std::cout << Color::YELLOW << "[IO] No hay dispositivos inicializados"
                  << Color::RESET << std::endl;
        return completions;
    }

    for (int step = 0; step < ticks; ++step) {
        current_time_++;

        for (auto& entry : devices_) {
            auto& device = entry.second;

            if (!device.current_request.has_value()) {
                assign_next_request(device);
            }

            if (device.current_request.has_value()) {
                device.current_request->remaining_time--;
                device.total_busy_time++;

                if (device.current_request->remaining_time <= 0) {
                    completions.emplace_back(device.current_request->pid, device.name);
                    device.completed_requests++;
                    device.current_request.reset();
                    assign_next_request(device);
                }
            }
        }
    }

    return completions;
}

void IOManager::display_status() const {
    if (devices_.empty()) {
        std::cout << Color::YELLOW << "[IO] No hay dispositivos registrados"
                  << Color::RESET << std::endl;
        return;
    }

    print_header("DISPOSITIVOS DE I/O");
    std::cout << std::left
              << std::setw(12) << "Nombre"
              << std::setw(10) << "Spool"
              << std::setw(12) << "En cola"
              << std::setw(15) << "Solicitud actual"
              << std::setw(15) << "Restante"
              << std::setw(10) << "Complet."
              << std::endl;
    print_separator(70);

    for (const auto& entry : devices_) {
        const auto& device = entry.second;
        std::string current_pid = device.current_request.has_value()
                                      ? "P" + std::to_string(device.current_request->pid)
                                      : "-";
        std::string remaining = device.current_request.has_value()
                                    ? std::to_string(device.current_request->remaining_time)
                                    : "-";

        std::cout << std::setw(12) << device.name
                  << std::setw(10) << device.spool_capacity
                  << std::setw(12) << device.waiting_queue.size()
                  << std::setw(15) << current_pid
                  << std::setw(15) << remaining
                  << std::setw(10) << device.completed_requests
                  << std::endl;
    }
    std::cout << std::endl;
}

void IOManager::display_device(const std::string& name) const {
    auto* device = get_device(name);
    if (!device) {
        std::cout << Color::RED << "[IO] Dispositivo '" << name
                  << "' no encontrado" << Color::RESET << std::endl;
        return;
    }

    std::ostringstream title;
    title << "DETALLE I/O - " << name;
    print_header(title.str());

    std::cout << " Spool capacity:    " << device->spool_capacity << std::endl;
    std::cout << " Total requests:    " << device->total_requests << std::endl;
    std::cout << " Completadas:       " << device->completed_requests << std::endl;
    std::cout << " Tiempo de espera:  ";
    if (device->completed_requests > 0) {
        double avg_wait = static_cast<double>(device->total_wait_time) /
                          device->completed_requests;
        std::cout << std::fixed << std::setprecision(2) << avg_wait << " ticks";
    } else {
        std::cout << "-";
    }
    std::cout << std::endl;

    std::cout << " Solicitud en curso: ";
    if (device->current_request.has_value()) {
        const auto& req = device->current_request.value();
        std::cout << "#"<< req.request_id << " (P" << req.pid << "), restante="
                  << req.remaining_time;
    } else {
        std::cout << "-";
    }
    std::cout << std::endl;

    if (device->waiting_queue.empty()) {
        std::cout << "\n Cola vacía.\n" << std::endl;
        return;
    }

    std::cout << "\n Cola de espera (mayor prioridad primero):\n";
    print_separator(60);
    std::cout << std::left
              << std::setw(8) << "ReqID"
              << std::setw(8) << "PID"
              << std::setw(10) << "Prio"
              << std::setw(12) << "Duración"
              << std::setw(12) << "Esperando"
              << std::endl;
    print_separator(60);

    auto queue_copy = device->waiting_queue;
    std::sort(queue_copy.begin(), queue_copy.end(),
              [](const IORequest& a, const IORequest& b) {
                  if (a.priority == b.priority)
                      return a.arrival_time < b.arrival_time;
                  return a.priority > b.priority;
              });

    for (const auto& req : queue_copy) {
        std::cout << std::setw(8) << req.request_id
                  << std::setw(8) << ("P" + std::to_string(req.pid))
                  << std::setw(10) << req.priority
                  << std::setw(12) << req.duration
                  << std::setw(12) << (current_time_ - req.arrival_time)
                  << std::endl;
    }
    std::cout << std::endl;
}

void IOManager::reset() {
    current_time_ = 0;
    for (auto& entry : devices_) {
        auto& device = entry.second;
        device.current_request.reset();
        device.waiting_queue.clear();
        device.total_wait_time = 0;
        device.total_requests = 0;
        device.completed_requests = 0;
        device.total_busy_time = 0;
        device.next_request_id = 1;
    }
    std::cout << Color::CYAN << "[IO] Estado de dispositivos reiniciado"
              << Color::RESET << std::endl;
}
