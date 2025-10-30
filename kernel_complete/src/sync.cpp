#include "../include/sync.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <iomanip>

ProducerConsumer::ProducerConsumer(int buffer_size)
    : buffer_size_(buffer_size),
      count_(0),
      in_(0),
      out_(0),
      total_produced_(0),
      total_consumed_(0),
      producer_blocks_(0),
      consumer_blocks_(0) {
    
    buffer_.resize(buffer_size, -1);
    
    std::cout << Color::GREEN << "[SYNC] Buffer inicializado (tamaño=" 
              << buffer_size << ")" << Color::RESET << std::endl;
}

bool ProducerConsumer::produce(int item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Esperar si el buffer está lleno
    if (count_ >= buffer_size_) {
        std::cout << Color::YELLOW << "[PRODUCTOR] Buffer lleno, bloqueado..." 
                  << Color::RESET << std::endl;
        producer_blocks_++;
        
        // En CLI solo reportamos
        return false;
    }
    
    // Producir el item
    buffer_[in_] = item;
    in_ = (in_ + 1) % buffer_size_;
    count_++;
    total_produced_++;
    
    std::cout << Color::GREEN << "[PRODUCTOR] "
              << "Item " << item << " producido "
              << "(buffer: " << count_ << "/" << buffer_size_ << ")"
              << Color::RESET << std::endl;
    
    // Notificar a consumidores
    not_empty_.notify_one();
    
    return true;
}

bool ProducerConsumer::consume(int& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Esperar si el buffer está vacío
    if (count_ == 0) {
        std::cout << Color::YELLOW << "[CONSUMIDOR] Buffer vacío, bloqueado..." 
                  << Color::RESET << std::endl;
        consumer_blocks_++;
        
        // En CLI no bloqueamos indefinidamente, solo reportamos
        return false;
    }
    
    // Consumir el item
    item = buffer_[out_];
    buffer_[out_] = -1; // Marcar como vacío
    out_ = (out_ + 1) % buffer_size_;
    count_--;
    total_consumed_++;
    
    std::cout << Color::CYAN << "[CONSUMIDOR] "
              << "Item " << item << " consumido "
              << "(buffer: " << count_ << "/" << buffer_size_ << ")"
              << Color::RESET << std::endl;
    
    // Notificar a productores
    not_full_.notify_one();
    
    return true;
}

void ProducerConsumer::display_buffer() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    print_header("BUFFER COMPARTIDO");
    
    std::cout << "Estado: " << count_ << "/" << buffer_size_ << " elementos" << std::endl;
    std::cout << "IN  = " << in_ << " (próxima inserción)" << std::endl;
    std::cout << "OUT = " << out_ << " (próxima extracción)" << std::endl;
    std::cout << "\nContenido del buffer:\n";
    
    std::cout << "┌";
    for (int i = 0; i < buffer_size_; i++) {
        std::cout << "─────┬";
    }
    std::cout << "\b┐\n│";
    
    // Mostrar índices
    for (int i = 0; i < buffer_size_; i++) {
        std::cout << std::setw(4) << i << " │";
    }
    std::cout << "\n├";
    for (int i = 0; i < buffer_size_; i++) {
        std::cout << "─────┼";
    }
    std::cout << "\b┤\n│";
    
    // Mostrar contenido
    for (int i = 0; i < buffer_size_; i++) {
        if (buffer_[i] == -1) {
            std::cout << Color::WHITE << " --- " << Color::RESET << "│";
        } else {
            std::cout << Color::GREEN << std::setw(4) << buffer_[i] 
                      << Color::RESET << " │";
        }
    }
    std::cout << "\n└";
    for (int i = 0; i < buffer_size_; i++) {
        std::cout << "─────┴";
    }
    std::cout << "\b┘\n";
    
    // Indicadores visuales
    std::cout << " ";
    for (int i = 0; i < buffer_size_; i++) {
        if (i == in_ && i == out_ && count_ == 0) {
            std::cout << "  ↕   ";  // IN y OUT en mismo lugar (vacío)
        } else if (i == in_) {
            std::cout << Color::GREEN << "  ↓   " << Color::RESET;  // IN
        } else if (i == out_) {
            std::cout << Color::CYAN << "  ↑   " << Color::RESET;   // OUT
        } else {
            std::cout << "      ";
        }
    }
    std::cout << "\n" << std::endl;
}

void ProducerConsumer::display_stats() const {
    print_header("ESTADÍSTICAS DE SINCRONIZACIÓN");
    
    std::cout << " Buffer:" << std::endl;
    std::cout << "   ├─ Tamaño:             " << buffer_size_ << std::endl;
    std::cout << "   └─ Ocupación actual:   " << count_ << " elementos" << std::endl;
    
    std::cout << "\n Operaciones:" << std::endl;
    std::cout << "   ├─ Items producidos:   " << total_produced_ << std::endl;
    std::cout << "   └─ Items consumidos:   " << total_consumed_ << std::endl;
    
    std::cout << "\n Bloqueos:" << std::endl;
    std::cout << "   ├─ Productor bloqueado: " << producer_blocks_ 
              << " veces (buffer lleno)" << std::endl;
    std::cout << "   └─ Consumidor bloqueado: " << consumer_blocks_ 
              << " veces (buffer vacío)" << std::endl;
    
    int balance = total_produced_ - total_consumed_;
    std::cout << "\n  Balance:              ";
    if (balance == count_) {
        std::cout << Color::GREEN << " Correcto" << Color::RESET 
                  << " (" << balance << " items en buffer)";
    } else {
        std::cout << Color::RED << "Error" << Color::RESET 
                  << " (esperado=" << balance << ", actual=" << count_ << ")";
    }
    std::cout << "\n" << std::endl;
}

void ProducerConsumer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    count_ = 0;
    in_ = 0;
    out_ = 0;
    total_produced_ = 0;
    total_consumed_ = 0;
    producer_blocks_ = 0;
    consumer_blocks_ = 0;
    
    for (auto& slot : buffer_) {
        slot = -1;
    }
    
    std::cout << Color::CYAN << "[SYNC] Buffer reiniciado" 
              << Color::RESET << std::endl;
}
