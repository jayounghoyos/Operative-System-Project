#include "../include/scheduler.hpp"
#include "../include/memory.hpp"
#include "../include/sync.hpp"
#include "../include/philosophers.hpp"
#include "../include/io.hpp"
#include "../include/heap.hpp"
#include "../include/disk.hpp"
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
║    • CPU Scheduling (Round Robin / SJF)                  ║
║    • Memory Management (FIFO / LRU)                      ║
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
    std::cout << "  cpu-rr <q>        - Usar Round Robin con quantum q\n";
    std::cout << "  cpu-sjf           - Usar SJF no expropiativo\n";
    std::cout << "  new <burst>       - Crear proceso con tiempo de ráfaga\n";
    std::cout << "  ps                - Listar todos los procesos\n";
    std::cout << "  tick              - Ejecutar 1 tick\n";
    std::cout << "  run <n>           - Ejecutar N ticks\n";
    std::cout << "  kill <pid>        - Terminar proceso\n";
    std::cout << "  suspend <pid>     - Suspender proceso (estado BLOCKED)\n";
    std::cout << "  resume <pid>      - Reanudar proceso suspendido\n";
    std::cout << "  cpu-stats         - Estadísticas del scheduler\n";

    std::cout << "\n"
              << Color::YELLOW << " MEMORY MANAGEMENT " << Color::RESET << std::endl;
    std::cout << "  mem-init <frames> [fifo|lru|pff] - Inicializar memoria (por defecto FIFO)\n";
    std::cout << "  mem-policy <fifo|lru|pff>        - Cambiar política de reemplazo\n";
    std::cout << "  mem-access <pid> <page>          - Acceder a página\n";
    std::cout << "  mem-frames                       - Ver estado de frames\n";
    std::cout << "  mem-table <pid>                  - Ver page table de proceso\n";
    std::cout << "  mem-stats                        - Estadísticas de memoria\n";
    std::cout << "  mem-pff-stats                    - Estadísticas PFF (gestión avanzada)\n";
    std::cout << "  mem-reset                        - Reiniciar estadísticas\n";

    std::cout << "\n"
              << Color::YELLOW << " SYNCHRONIZATION " << Color::RESET << std::endl;
    std::cout << "  pc-init <size>    - Inicializar buffer productor-consumidor\n";
    std::cout << "  produce <item>    - Producir item\n";
    std::cout << "  consume           - Consumir item\n";
    std::cout << "  pc-buffer         - Ver estado del buffer\n";
    std::cout << "  pc-stats          - Estadísticas de sincronización\n";
    std::cout << "  pc-reset          - Reiniciar buffer\n";

    std::cout << "\n"
              << Color::YELLOW << " DINING PHILOSOPHERS " << Color::RESET << std::endl;
    std::cout << "  phil-init         - Inicializar simulación de filósofos\n";
    std::cout << "  phil-step         - Avanzar un paso de simulación\n";
    std::cout << "  phil-run <n>      - Avanzar N pasos\n";
    std::cout << "  phil-table        - Mostrar mesa y estados\n";
    std::cout << "  phil-stats        - Mostrar estadísticas acumuladas\n";
    std::cout << "  phil-forks        - Mostrar tenedores\n";
    std::cout << "  phil-reset        - Reiniciar simulación\n";

    std::cout << "\n"
              << Color::YELLOW << " IO DEVICES " << Color::RESET << std::endl;
    std::cout << "  io-init <name> <spool>   - Registrar dispositivo con spool limitado\n";
    std::cout << "  io-devices               - Listar dispositivos y estado\n";
    std::cout << "  io-device <name>         - Ver detalle de un dispositivo\n";
    std::cout << "  io-request <dev> <pid> <dur> <prio> - Solicitar I/O (bloquea proceso)\n";
    std::cout << "  io-run <ticks>           - Procesar I/O durante N ticks\n";
    std::cout << "  io-reset                 - Reiniciar estado de I/O\n";

    std::cout << "\n"
              << Color::YELLOW << " HEAP ALLOCATOR " << Color::RESET << std::endl;
    std::cout << "  heap-init <total> <min>  - Inicializar buddy allocator\n";
    std::cout << "  heap-alloc <bytes>       - Solicitar bloque\n";
    std::cout << "  heap-free <id>           - Liberar bloque\n";
    std::cout << "  heap-stats               - Ver estadísticas de heap\n";

    std::cout << "\n"
              << Color::YELLOW << " DISK SCHEDULER " << Color::RESET << std::endl;
    std::cout << "  disk-init <tracks>       - Inicializar disco\n";
    std::cout << "  disk-head <pos>          - Mover cabezal a cilindro\n";
    std::cout << "  disk-queue <c1> <c2>...  - Definir cola de peticiones\n";
    std::cout << "  disk-add <c>             - Agregar solicitud individual\n";
    std::cout << "  disk-run <fcfs|sstf|scan|scan-down> - Ejecutar algoritmo\n";
    std::cout << "  disk-stats               - Mostrar métricas\n";
    std::cout << "  disk-view                - Vista de cilindros y cabezal\n";
    std::cout << "  disk-reset               - Limpiar estado\n";

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
    std::unique_ptr<IScheduler> scheduler = nullptr;
    std::unique_ptr<MemoryManager> memory = nullptr;
    std::unique_ptr<ProducerConsumer> pc_buffer = nullptr;
    std::unique_ptr<DiningPhilosophers> philosophers = nullptr;
    std::unique_ptr<IOManager> io_manager = std::make_unique<IOManager>();
    std::unique_ptr<BuddyAllocator> heap = nullptr;
    std::unique_ptr<DiskScheduler> disk = std::make_unique<DiskScheduler>();

    // Configuración por defecto: Round Robin (q=3)
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

            //  CAMBIO DE ALGORITMO DE CPU
            else if (command == "cpu-rr")
            {
                int q;
                if (iss >> q && q > 0)
                {
                    scheduler = std::make_unique<RoundRobinScheduler>(q);
                    std::cout << Color::GREEN << "[CPU] Cambiado a Round Robin (q=" << q << ")"
                              << Color::RESET << std::endl;
                }
                else
                {
                    std::cout << Color::RED << "Uso: cpu-rr <quantum>" << Color::RESET << std::endl;
                }
            }
            else if (command == "cpu-sjf")
            {
                scheduler = std::make_unique<SJFScheduler>();
                std::cout << Color::GREEN << "[CPU] Cambiado a SJF (no expropiativo)"
                          << Color::RESET << std::endl;
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
            else if (command == "suspend")
            {
                int pid;
                if (iss >> pid)
                {
                    scheduler->suspend_process(pid);
                }
                else
                {
                    std::cout << Color::RED << "Uso: suspend <pid>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "resume")
            {
                int pid;
                if (iss >> pid)
                {
                    scheduler->resume_process(pid);
                }
                else
                {
                    std::cout << Color::RED << "Uso: resume <pid>"
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
                int frames; std::string pol;
                if ((iss >> frames) && frames > 0)
                {
                    if (iss >> pol)
                    {
                        if (pol == "fifo" || pol == "FIFO")
                            memory = std::make_unique<MemoryManager>(frames, ReplacementPolicy::FIFO);
                        else if (pol == "lru" || pol == "LRU")
                            memory = std::make_unique<MemoryManager>(frames, ReplacementPolicy::LRU);
                        else if (pol == "pff" || pol == "PFF")
                            memory = std::make_unique<MemoryManager>(frames, ReplacementPolicy::PFF);
                        else
                        {
                            std::cout << Color::YELLOW << "Política no reconocida, usando FIFO"
                                      << Color::RESET << std::endl;
                            memory = std::make_unique<MemoryManager>(frames, ReplacementPolicy::FIFO);
                        }
                    }
                    else
                    {
                        memory = std::make_unique<MemoryManager>(frames, ReplacementPolicy::FIFO);
                    }
                }
                else
                {
                    std::cout << Color::RED << "Uso: mem-init <frames> [fifo|lru|pff]"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "mem-policy")
            {
                if (!memory)
                {
                    std::cout << Color::RED << "Error: Memoria no inicializada"
                              << Color::RESET << std::endl;
                }
                else
                {
                    std::string pol;
                    if (iss >> pol)
                    {
                        if (pol == "fifo" || pol == "FIFO") memory->set_policy(ReplacementPolicy::FIFO);
                        else if (pol == "lru" || pol == "LRU") memory->set_policy(ReplacementPolicy::LRU);
                        else if (pol == "pff" || pol == "PFF") memory->set_policy(ReplacementPolicy::PFF);
                        else std::cout << Color::RED << "Uso: mem-policy <fifo|lru|pff>" << Color::RESET << std::endl;
                    }
                    else
                    {
                        std::cout << Color::RED << "Uso: mem-policy <fifo|lru|pff>" << Color::RESET << std::endl;
                    }
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
            else if (command == "mem-pff-stats")
            {
                if (!memory)
                {
                    std::cout << Color::RED << "Error: Memoria no inicializada"
                              << Color::RESET << std::endl;
                }
                else
                {
                    memory->display_pff_stats();
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
                    // Mensaje ya mostrado dentro de consume()
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

            //  DINING PHILOSOPHERS
            else if (command == "phil-init")
            {
                philosophers = std::make_unique<DiningPhilosophers>();
                std::cout << Color::GREEN << "[PHIL] Simulación inicializada (5 filósofos)"
                          << Color::RESET << std::endl;
            }
            else if (command == "phil-step")
            {
                if (!philosophers)
                {
                    std::cout << Color::RED << "Error: Inicializa con phil-init"
                              << Color::RESET << std::endl;
                }
                else
                {
                    philosophers->simulate_step();
                }
            }
            else if (command == "phil-run")
            {
                if (!philosophers)
                {
                    std::cout << Color::RED << "Error: Inicializa con phil-init"
                              << Color::RESET << std::endl;
                }
                else
                {
                    int n;
                    if (iss >> n && n > 0)
                    {
                        philosophers->simulate_steps(n);
                    }
                    else
                    {
                        std::cout << Color::RED << "Uso: phil-run <n>"
                                  << Color::RESET << std::endl;
                    }
                }
            }
            else if (command == "phil-table")
            {
                if (!philosophers)
                {
                    std::cout << Color::RED << "Error: Inicializa con phil-init"
                              << Color::RESET << std::endl;
                }
                else
                {
                    philosophers->display_table();
                }
            }
            else if (command == "phil-stats")
            {
                if (!philosophers)
                {
                    std::cout << Color::RED << "Error: Inicializa con phil-init"
                              << Color::RESET << std::endl;
                }
                else
                {
                    philosophers->display_stats();
                }
            }
            else if (command == "phil-forks")
            {
                if (!philosophers)
                {
                    std::cout << Color::RED << "Error: Inicializa con phil-init"
                              << Color::RESET << std::endl;
                }
                else
                {
                    philosophers->display_forks();
                }
            }
            else if (command == "phil-reset")
            {
                if (!philosophers)
                {
                    std::cout << Color::RED << "Error: Inicializa con phil-init"
                              << Color::RESET << std::endl;
                }
                else
                {
                    philosophers->reset();
                }
            }

            //  IO DEVICES
            else if (command == "io-init")
            {
                std::string name;
                int spool;
                if (iss >> name >> spool)
                {
                    io_manager->add_device(name, spool);
                }
                else
                {
                    std::cout << Color::RED << "Uso: io-init <name> <spool>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "io-devices")
            {
                io_manager->display_status();
            }
            else if (command == "io-device")
            {
                std::string name;
                if (iss >> name)
                {
                    io_manager->display_device(name);
                }
                else
                {
                    std::cout << Color::RED << "Uso: io-device <name>"
                              << Color::RESET << std::endl;
                }
            }
    else if (command == "io-request")
    {
        std::string device;
        int pid, duration, priority;
        if (!(iss >> device >> pid >> duration >> priority))
        {
            std::cout << Color::RED << "Uso: io-request <dev> <pid> <dur> <prio>"
                      << Color::RESET << std::endl;
            continue;
        }

        if (!io_manager->device_exists(device))
        {
            std::cout << Color::RED << "[IO] Dispositivo no registrado"
                      << Color::RESET << std::endl;
            continue;
        }

        if (!scheduler)
        {
            std::cout << Color::RED << "[IO] Scheduler no inicializado"
                      << Color::RESET << std::endl;
            continue;
        }

        // Aquí preferimos suspender primero para que, si se llena el spool,
        // podamos revertir el bloqueo y dejar al proceso en READY.
        if (!scheduler->suspend_process(pid))
        {
            continue;
        }

                std::string error;
                if (!io_manager->submit_request(device, pid, duration, priority, error))
                {
                    std::cout << Color::RED << "[IO] " << error
                              << Color::RESET << std::endl;
                    scheduler->resume_process(pid);
                }
            }
            else if (command == "io-run")
            {
                int ticks;
                if (!(iss >> ticks) || ticks <= 0)
                {
                    std::cout << Color::RED << "Uso: io-run <ticks> (ticks > 0)"
                              << Color::RESET << std::endl;
                    continue;
                }

                auto completions = io_manager->process_ticks(ticks);
                if (completions.empty())
                {
                    std::cout << Color::YELLOW << "[IO] No se completó ninguna solicitud"
                              << Color::RESET << std::endl;
                }
                else
                {
                    for (const auto& completion : completions)
                    {
                        int pid = completion.first;
                        const std::string& dev_name = completion.second;
                        std::cout << Color::GREEN << "[IO] " << dev_name
                                  << " completó trabajo de P" << pid
                                  << Color::RESET << std::endl;
                        scheduler->resume_process(pid);
                    }
                }
            }
            else if (command == "io-reset")
            {
                io_manager->reset();
            }

            //  HEAP ALLOCATOR
            else if (command == "heap-init")
            {
                size_t total, min_block;
                if (iss >> total >> min_block)
                {
                    heap = std::make_unique<BuddyAllocator>();
                    heap->init(total, min_block);
                }
                else
                {
                    std::cout << Color::RED << "Uso: heap-init <total> <min>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "heap-alloc")
            {
                if (!heap)
                {
                    std::cout << Color::RED << "Error: Primero heap-init"
                              << Color::RESET << std::endl;
                    continue;
                }

                size_t bytes;
                if (iss >> bytes)
                {
                    heap->alloc(bytes);
                }
                else
                {
                    std::cout << Color::RED << "Uso: heap-alloc <bytes>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "heap-free")
            {
                if (!heap)
                {
                    std::cout << Color::RED << "Error: Primero heap-init"
                              << Color::RESET << std::endl;
                    continue;
                }

                int alloc_id;
                if (iss >> alloc_id)
                {
                    heap->free(alloc_id);
                }
                else
                {
                    std::cout << Color::RED << "Uso: heap-free <id>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "heap-stats")
            {
                if (!heap)
                {
                    std::cout << Color::RED << "Error: Primero heap-init"
                              << Color::RESET << std::endl;
                }
                else
                {
                    heap->display_stats();
                }
            }

            //  DISK SCHEDULER
            else if (command == "disk-init")
            {
                int tracks;
                if (iss >> tracks)
                {
                    disk->init(tracks);
                }
                else
                {
                    std::cout << Color::RED << "Uso: disk-init <tracks>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "disk-head")
            {
                int pos;
                if (iss >> pos)
                {
                    disk->set_head(pos);
                }
                else
                {
                    std::cout << Color::RED << "Uso: disk-head <pos>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "disk-queue")
            {
                std::vector<int> queue;
                int cyl;
                while (iss >> cyl)
                {
                    queue.push_back(cyl);
                }

                if (queue.empty())
                {
                    std::cout << Color::RED << "Uso: disk-queue <c1> <c2> ..."
                              << Color::RESET << std::endl;
                }
                else
                {
                    disk->set_queue(queue);
                }
            }
            else if (command == "disk-add")
            {
                int cyl;
                if (iss >> cyl)
                {
                    disk->add_request(cyl);
                }
                else
                {
                    std::cout << Color::RED << "Uso: disk-add <cilindro>"
                              << Color::RESET << std::endl;
                }
            }
            else if (command == "disk-run")
            {
                std::string algo;
                if (!(iss >> algo))
                {
                    std::cout << Color::RED << "Uso: disk-run <fcfs|sstf|scan|scan-down>"
                              << Color::RESET << std::endl;
                    continue;
                }

                DiskAlgorithm alg;
                if (algo == "fcfs")
                    alg = DiskAlgorithm::FCFS;
                else if (algo == "sstf")
                    alg = DiskAlgorithm::SSTF;
                else if (algo == "scan")
                    alg = DiskAlgorithm::SCAN_UP;
                else if (algo == "scan-down")
                    alg = DiskAlgorithm::SCAN_DOWN;
                else
                {
                    std::cout << Color::RED << "Algoritmo no válido"
                              << Color::RESET << std::endl;
                    continue;
                }

                // Dejamos que el módulo de disco se encargue del cálculo;
                // la CLI solo valida el nombre del algoritmo.
                disk->run(alg);
            }
            else if (command == "disk-stats")
            {
                disk->display_stats();
            }
            else if (command == "disk-view")
            {
                disk->display_view();
            }
            else if (command == "disk-reset")
            {
                disk->reset();
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
