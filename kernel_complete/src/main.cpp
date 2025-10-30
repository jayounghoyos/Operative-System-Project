#include "../include/scheduler.hpp"
#include "../include/memory.hpp"
#include "../include/sync.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <sstream>
#include <memory>

void print_banner()
{
    std::cout << Color::CYAN << Color::BOLD << R"(
╔══════════════════════════════════════════════════════════╗
║                                                          ║
║        SIMULADOR DE KERNEL - SISTEMA OPERATIVO           ║
║                                                          ║
║  Módulos:                                                ║
║    • CPU Scheduling (Round Robin)                        ║
║    • Memory Management (FIFO)                            ║
║    • Synchronization (Producer-Consumer)                 ║
║                                                          ║
╚══════════════════════════════════════════════════════════╝
)" << Color::RESET
              << std::endl;
}

void print_help()
{
    print_header("COMANDOS DISPONIBLES");

    std::cout << Color::YELLOW << " CPU SCHEDULING " << Color::RESET << std::endl;
    std::cout << "  new <burst>       - Crear proceso con tiempo de ráfaga\n";
    std::cout << "  ps                - Listar todos los procesos\n";
    std::cout << "  tick              - Ejecutar 1 tick\n";
    std::cout << "  run <n>           - Ejecutar N ticks\n";
    std::cout << "  kill <pid>        - Terminar proceso\n";
    std::cout << "  cpu-stats         - Estadísticas del scheduler\n";

    std::cout << "\n"
              << Color::YELLOW << " MEMORY MANAGEMENT " << Color::RESET << std::endl;
    std::cout << "  mem-init <frames> - Inicializar memoria\n";
    std::cout << "  mem-access <pid> <page> - Acceder a página\n";
    std::cout << "  mem-frames        - Ver estado de frames\n";
    std::cout << "  mem-table <pid>   - Ver page table de proceso\n";
    std::cout << "  mem-stats         - Estadísticas de memoria\n";
    std::cout << "  mem-reset         - Reiniciar estadísticas\n";

    std::cout << "\n"
              << Color::YELLOW << " SYNCHRONIZATION " << Color::RESET << std::endl;
    std::cout << "  pc-init <size>    - Inicializar buffer productor-consumidor\n";
    std::cout << "  produce <item>    - Producir item\n";
    std::cout << "  consume           - Consumir item\n";
    std::cout << "  pc-buffer         - Ver estado del buffer\n";
    std::cout << "  pc-stats          - Estadísticas de sincronización\n";
    std::cout << "  pc-reset          - Reiniciar buffer\n";

    std::cout << "\n"
              << Color::YELLOW << " GENERAL " << Color::RESET << std::endl;
    std::cout << "  help              - Mostrar esta ayuda\n";
    std::cout << "  clear             - Limpiar pantalla\n";
    std::cout << "  exit              - Salir\n";
    std::cout << std::endl;
}

int main()
{
    print_banner();

    // Inicializar módulos
    std::unique_ptr<RoundRobinScheduler> scheduler = nullptr;
    std::unique_ptr<MemoryManager> memory = nullptr;
    std::unique_ptr<ProducerConsumer> pc_buffer = nullptr;

    // Configuración por defecto
    int default_quantum = 3;
    scheduler = std::make_unique<RoundRobinScheduler>(default_quantum);
    std::cout << Color::GREEN << "[CPU] Scheduler Round Robin inicializado (quantum="
              << default_quantum << ")" << Color::RESET << std::endl;

    std::cout << "\n Escribe 'help' para ver todos los comandos\n"
              << std::endl;

    std::string line;
    while (true)
    {
        std::cout << Color::BOLD << "kernel> " << Color::RESET;
        std::getline(std::cin, line);

        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        try
        {
            //  COMANDOS GENERALES
            if (command == "exit" || command == "quit")
            {
                std::cout << "Saliendo del simulador..." << std::endl;
                break;
            }
            else if (command == "help")
            {
                print_help();
            }
            else if (command == "clear")
            {
                system("clear || cls");
                print_banner();
            }

            //  CPU SCHEDULING
            else if (command == "new")
            {
                int burst;
                if (iss >> burst && burst > 0)
                {
                    scheduler->create_process(burst);
                }
                else
                {
                    std::cout << Color::RED << "Error: burst debe ser > 0"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "ps")
            {
                scheduler->list_processes();
            }
            else if (command == "tick")
            {
                scheduler->tick();
            }
            else if (command == "run")
            {
                int n;
                if (iss >> n && n > 0)
                {
                    scheduler->run(n);
                }
                else
                {
                    std::cout << Color::RED << "Error: n debe ser > 0"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "kill")
            {
                int pid;
                if (iss >> pid)
                {
                    scheduler->kill_process(pid);
                }
                else
                {
                    std::cout << Color::RED << "Uso: kill <pid>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "cpu-stats")
            {
                scheduler->show_stats();
            }

            //  MEMORY MANAGEMENT
            else if (command == "mem-init")
            {
                int frames;
                if (iss >> frames && frames > 0)
                {
                    memory = std::make_unique<MemoryManager>(frames);
                }
                else
                {
                    std::cout << Color::RED << "Error: frames debe ser > 0"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "mem-access")
            {
                if (!memory)
                {
                    std::cout << Color::RED << "Error: Primero inicializa memoria con mem-init"
                              << Color::RESET << std::endl;
                    continue;
                }

                int pid, page;
                if (iss >> pid >> page && page >= 0)
                {
                    memory->access_page(pid, page);
                }
                else
                {
                    std::cout << Color::RED << "Uso: mem-access <pid> <page>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "mem-frames")
            {
                if (!memory)
                {
                    std::cout << Color::RED << "Error: Memoria no inicializada"
                              << Color::RESET << std::endl;
                }
                else
                {
                    memory->display_frames();
                }
            }
            else if (command == "mem-table")
            {
                if (!memory)
                {
                    std::cout << Color::RED << "Error: Memoria no inicializada"
                              << Color::RESET << std::endl;
                    continue;
                }

                int pid;
                if (iss >> pid)
                {
                    memory->display_page_table(pid);
                }
                else
                {
                    std::cout << Color::RED << "Uso: mem-table <pid>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "mem-stats")
            {
                if (!memory)
                {
                    std::cout << Color::RED << "Error: Memoria no inicializada"
                              << Color::RESET << std::endl;
                }
                else
                {
                    memory->display_stats();
                }
            }
            else if (command == "mem-reset")
            {
                if (!memory)
                {
                    std::cout << Color::RED << "Error: Memoria no inicializada"
                              << Color::RESET << std::endl;
                }
                else
                {
                    memory->reset_stats();
                }
            }

            //  SYNCHRONIZATION
            else if (command == "pc-init")
            {
                int size;
                if (iss >> size && size > 0)
                {
                    pc_buffer = std::make_unique<ProducerConsumer>(size);
                }
                else
                {
                    std::cout << Color::RED << "Error: size debe ser > 0"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "produce")
            {
                if (!pc_buffer)
                {
                    std::cout << Color::RED << "Error: Primero inicializa con pc-init"
                              << Color::RESET << std::endl;
                    continue;
                }

                int item;
                if (iss >> item)
                {
                    pc_buffer->produce(item);
                }
                else
                {
                    std::cout << Color::RED << "Uso: produce <item>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "consume")
            {
                if (!pc_buffer)
                {
                    std::cout << Color::RED << "Error: Primero inicializa con pc-init"
                              << Color::RESET << std::endl;
                    continue;
                }

                int item;
                if (pc_buffer->consume(item))
                {
                    // Éxito ya reportado en la función
                }
            }
            else if (command == "pc-buffer")
            {
                if (!pc_buffer)
                {
                    std::cout << Color::RED << "Error: Buffer no inicializado"
                              << Color::RESET << std::endl;
                }
                else
                {
                    pc_buffer->display_buffer();
                }
            }
            else if (command == "pc-stats")
            {
                if (!pc_buffer)
                {
                    std::cout << Color::RED << "Error: Buffer no inicializado"
                              << Color::RESET << std::endl;
                }
                else
                {
                    pc_buffer->display_stats();
                }
            }
            else if (command == "pc-reset")
            {
                if (!pc_buffer)
                {
                    std::cout << Color::RED << "Error: Buffer no inicializado"
                              << Color::RESET << std::endl;
                }
                else
                {
                    pc_buffer->reset();
                }
            }

            //  COMANDO DESCONOCIDO
            else
            {
                std::cout << Color::RED << "Comando desconocido: " << command
                          << Color::RESET << std::endl;
                std::cout << "Escribe 'help' para ver comandos disponibles." << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << Color::RED << "Error: " << e.what()
                      << Color::RESET << std::endl;
        }
    }

    return 0;
}
