#include "../include/disk.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

DiskScheduler::DiskScheduler()
    : total_tracks_(0),
      head_position_(0),
      total_movement_(0),
      initialized_(false) {}

bool DiskScheduler::init(int tracks) {
    if (tracks <= 0) {
        std::cout << Color::RED << "[DISK] Núm. de cilindros debe ser > 0"
                  << Color::RESET << std::endl;
        return false;
    }
    total_tracks_ = tracks;
    head_position_ = 0;
    initial_queue_.clear();
    execution_order_.clear();
    total_movement_ = 0;
    initialized_ = true;
    std::cout << Color::GREEN << "[DISK] Inicializado con " << tracks
              << " cilindros" << Color::RESET << std::endl;
    return true;
}

bool DiskScheduler::set_head(int position) {
    if (!initialized_) {
        std::cout << Color::RED << "[DISK] Ejecuta disk-init primero"
                  << Color::RESET << std::endl;
        return false;
    }
    if (position < 0 || position >= total_tracks_) {
        std::cout << Color::RED << "[DISK] Posición fuera de rango"
                  << Color::RESET << std::endl;
        return false;
    }
    head_position_ = position;
    std::cout << Color::CYAN << "[DISK] Cabezal movido a " << position
              << Color::RESET << std::endl;
    return true;
}

bool DiskScheduler::set_queue(const std::vector<int>& requests) {
    if (!initialized_) {
        std::cout << Color::RED << "[DISK] Ejecuta disk-init primero"
                  << Color::RESET << std::endl;
        return false;
    }
    for (int cyl : requests) {
        if (cyl < 0 || cyl >= total_tracks_) {
            std::cout << Color::RED << "[DISK] Cilindro fuera de rango: "
                      << cyl << Color::RESET << std::endl;
            return false;
        }
    }
    initial_queue_ = requests;
    std::cout << Color::CYAN << "[DISK] Cola actualizada (" << requests.size()
              << " solicitudes)" << Color::RESET << std::endl;
    return true;
}

void DiskScheduler::add_request(int cylinder) {
    if (!initialized_) {
        std::cout << Color::RED << "[DISK] Ejecuta disk-init primero"
                  << Color::RESET << std::endl;
        return;
    }
    if (cylinder < 0 || cylinder >= total_tracks_) {
        std::cout << Color::RED << "[DISK] Cilindro fuera de rango"
                  << Color::RESET << std::endl;
        return;
    }
    initial_queue_.push_back(cylinder);
    std::cout << Color::CYAN << "[DISK] Agregada solicitud a cilindro "
              << cylinder << Color::RESET << std::endl;
}

bool DiskScheduler::run(DiskAlgorithm algo) {
    if (!initialized_) {
        std::cout << Color::RED << "[DISK] Ejecuta disk-init primero"
                  << Color::RESET << std::endl;
        return false;
    }
    if (initial_queue_.empty()) {
        std::cout << Color::YELLOW << "[DISK] Cola vacía"
                  << Color::RESET << std::endl;
        return false;
    }

    total_movement_ = 0;
    execution_order_.clear();

    switch (algo) {
    case DiskAlgorithm::FCFS:
        run_fcfs();
        break;
    case DiskAlgorithm::SSTF:
        run_sstf();
        break;
    case DiskAlgorithm::SCAN_UP:
        run_scan(true);
        break;
    case DiskAlgorithm::SCAN_DOWN:
        run_scan(false);
        break;
    }
    // Guardamos el último movimiento para mostrarlo en la CLI
    // y así poder comparar algoritmos sin cálculos externos.

    std::cout << Color::GREEN << "[DISK] Ejecución completada (movimiento total="
              << total_movement_ << ")" << Color::RESET << std::endl;
    return true;
}

void DiskScheduler::run_fcfs() {
    int head = head_position_;
    for (int cyl : initial_queue_) {
        total_movement_ += std::abs(cyl - head);
        head = cyl;
        execution_order_.push_back(cyl);
    }
}

void DiskScheduler::run_sstf() {
    int head = head_position_;
    auto remaining = initial_queue_;

    while (!remaining.empty()) {
        auto closest_it = std::min_element(
            remaining.begin(), remaining.end(),
            [head](int a, int b) {
                return std::abs(a - head) < std::abs(b - head);
            });
        int cyl = *closest_it;
        total_movement_ += std::abs(cyl - head);
        head = cyl;
        execution_order_.push_back(cyl);
        remaining.erase(closest_it);
    }
}

void DiskScheduler::run_scan(bool upwards) {
    int head = head_position_;
    auto remaining = initial_queue_;
    std::sort(remaining.begin(), remaining.end());

    auto split_it = std::partition(remaining.begin(), remaining.end(),
                                   [head](int cyl) { return cyl < head; });
    std::vector<int> lower(remaining.begin(), split_it);
    std::vector<int> upper(split_it, remaining.end());

    std::reverse(lower.begin(), lower.end());

    auto process_sequence = [&](const std::vector<int>& seq) {
        for (int cyl : seq) {
            total_movement_ += std::abs(cyl - head);
            head = cyl;
            execution_order_.push_back(cyl);
        }
    };

    if (upwards) {
        process_sequence(upper);
        if (!lower.empty()) {
            total_movement_ += std::abs(head - (total_tracks_ - 1));
            head = total_tracks_ - 1;
            process_sequence(lower);
        }
    } else {
        process_sequence(lower);
        if (!upper.empty()) {
            total_movement_ += std::abs(head - 0);
            head = 0;
            process_sequence(upper);
        }
    }
}

void DiskScheduler::display_stats() const {
    if (!initialized_) {
        std::cout << Color::RED << "[DISK] Ejecuta disk-init primero"
                  << Color::RESET << std::endl;
        return;
    }

    print_header("ESTADÍSTICAS DE DISCO");
    std::cout << " Cilindros totales:      " << total_tracks_ << std::endl;
    std::cout << " Cabezal actual:         " << head_position_ << std::endl;
    std::cout << " Solicitudes en cola:    " << initial_queue_.size() << std::endl;
    std::cout << " Movimiento total (última simulación): "
              << total_movement_ << std::endl;

    if (!execution_order_.empty()) {
        std::cout << "\n Orden atendido: ";
        for (size_t i = 0; i < execution_order_.size(); ++i) {
            std::cout << execution_order_[i];
            if (i + 1 < execution_order_.size()) std::cout << " -> ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void DiskScheduler::display_view() const {
    if (!initialized_) {
        std::cout << Color::RED << "[DISK] Ejecuta disk-init primero"
                  << Color::RESET << std::endl;
        return;
    }

    const int width = 50;
    std::string line(width, '-');

    print_header("VISTA DE DISCO");
    std::cout << "0" << std::string(width - 2, ' ') << (total_tracks_ - 1) << std::endl;
    std::cout << line << std::endl;

    auto to_pos = [&](int cylinder) {
        if (total_tracks_ <= 1) return 0;
        double fraction = static_cast<double>(cylinder) / (total_tracks_ - 1);
        return static_cast<int>(fraction * (width - 1));
    };

    std::string axis(width, ' ');
    axis[to_pos(head_position_)] = 'H';
    std::cout << axis << "  (H = cabezal)" << std::endl;

    if (!initial_queue_.empty()) {
        std::string axis_req(width, ' ');
        for (int cyl : initial_queue_) {
            axis_req[to_pos(cyl)] = '*';
        }
        std::cout << axis_req << "  (* = solicitudes)" << std::endl;
    }

    if (!execution_order_.empty()) {
        std::cout << "\nOrden recorrido: ";
        for (size_t i = 0; i < execution_order_.size(); ++i) {
            std::cout << execution_order_[i];
            if (i + 1 < execution_order_.size()) std::cout << " -> ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void DiskScheduler::reset() {
    initial_queue_.clear();
    execution_order_.clear();
    total_movement_ = 0;
    head_position_ = 0;
    std::cout << Color::CYAN << "[DISK] Estado reiniciado"
              << Color::RESET << std::endl;
}
