# Operative-System-Project

# Kernel Simulator – Operative Systems

- **CPU Scheduling** – Round Robin con quantum fijo y cola de listos.
- **Memory Management** – Gestor de marcos físicos con tablas de página por proceso y reemplazo FIFO.
- **Synchronization** – Mini framework basado en `std::mutex`/`std::condition_variable` que expone el problema productor–consumidor vía CLI.

Cada subsistema imprime estados en ASCII para facilitar el seguimiento durante el live coding.

## Estructura

```
kernel_complete/
├── include/      # Interfaces públicas (.hpp)
├── src/          # Implementaciones (.cpp)
├── compile.sh    # Script de build rápido con g++
├── CMakeLists.txt
└── DEMO_COMPLETO.txt
```

## Construir y ejecutar

```bash
cd kernel_complete
./compile.sh      # genera ./kernel-sim usando g++
./kernel-sim      # inicia la CLI interactiva
```

> o usar (`cmake -S . -B build && cmake --build build`).

## Comandos de la CLI

- **CPU:** `new`, `ps`, `tick`, `run <n>`, `kill <pid>`, `cpu-stats`
- **Memoria:** `mem-init <frames>`, `mem-access <pid> <page>`, `mem-frames`, `mem-table <pid>`, `mem-stats`, `mem-reset`
- **Sync:** `pc-init <size>`, `produce <item>`, `consume`, `pc-buffer`, `pc-stats`, `pc-reset`
- **Generales:** `help`, `clear`, `exit`

Todos los comandos se listan con `help` desde la propia CLI.

## Algoritmos implementados

- **Round Robin:** `src/scheduler.cpp` mantiene la cola de listos con `std::queue`, preemption y cálculo de turnaround/espera para validar fairness.
- **Gestión FIFO:** `src/memory.cpp` usa una cola FIFO para elegir marcos víctimas, registra hits/faults, y representa tablas de página por proceso.
- **Productor–Consumidor:** `src/sync.cpp` modela un buffer circular con exclusión mutua y contadores de bloqueos para ilustrar el uso de monitores.

## Cobertura de requerimientos

-  PCB con campos de ráfaga, estado, espera y turnaround (`include/process.hpp`).
-  Round Robin con quantum fijo, cola lista y comandos `new`, `ps`, `tick`, `kill`, `run`.
-  Gestor de memoria con tablas de página, FIFO, contadores de hits/faults y visualización.
-  Mini framework con mutex/condvars y CLI para productor–consumidor.
-  Pendiente: automatizar pruebas de fairness y calcular métricas PFF/tiempo por acceso para memoria (solo totales actuales).

## Demo rápido

```
kernel> new 10
kernel> mem-init 4
kernel> mem-access 1 0
kernel> pc-init 3
kernel> produce 100
kernel> run 10
kernel> cpu-stats
kernel> mem-stats
kernel> pc-stats
```

El archivo [Demo_Completo](/kernel_complete/DEMO_COMPLETO.txt) que hicimos tiene... un demo más completo.