#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <iostream>
#include <iomanip>

// Colores ANSI
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string BOLD    = "\033[1m";
    const std::string RED     = "\033[31m";
    const std::string GREEN   = "\033[32m";
    const std::string YELLOW  = "\033[33m";
    const std::string BLUE    = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN    = "\033[36m";
    const std::string WHITE   = "\033[37m";
}

// Helper para imprimir línea separadora
inline void print_separator(int width = 80) {
    std::cout << std::string(width, '=') << std::endl;
}

// Helper para imprimir encabezado
inline void print_header(const std::string& title) {
    std::cout << Color::MAGENTA << Color::BOLD 
              << "\n╔═══ " << title << " ═══╗\n" 
              << Color::RESET << std::endl;
}

#endif // UTILS_HPP
