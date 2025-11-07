# Kernel Simulator – Operative Systems

Este repositorio contiene un simulador modular de núcleo escrito en C++17. Desde la CLI podemos crear procesos, monitorear memoria virtual, probar sincronización, lanzar solicitudes de I/O, ejercitar un buddy allocator y comparar algoritmos de planificación de disco. Cada módulo imprime métricas en ASCII para que los experimentos sean reproducibles en clase.

## Módulos implementados

- **CPU Scheduling** – Round Robin (quantum configurable) y SJF no expropiativo, con suspensión/reanudación manual e integración con las colas de I/O.
- **Memoria Virtual** – FIFO, LRU y PFF (gestión avanzada). Se muestran `mem-frames`, `mem-stats`, `mem-pff-stats` y tablas por proceso.
- **Sincronización** – Productor–consumidor con `std::mutex`/`std::condition_variable` y el caso de estudio de los filósofos (estrategia asimétrica).
- **Entrada/Salida** – `IOManager` con dispositivos que tienen spool, prioridad y cola; las solicitudes bloquean al proceso hasta que se completan.
- **Asignador en Heap** – Buddy allocator con medición de fragmentación interna y bytes en uso.
- **Planificación de Disco** – FCFS, SSTF y SCAN con cálculo de movimiento total y vista de cilindros + cabezal.

## Estructura del repositorio

```
.
├── kernel_complete/
│   ├── include/        # Interfaces públicas (.hpp)
│   ├── src/            # Implementaciones (.cpp)
│   ├── DEMO_EXPO.txt   # Guion para la demo en vivo
│   ├── DEMO_PFF.txt    # Script dedicado a PFF
│   ├── compile.sh      # Build rápido con g++
│   └── CMakeLists.txt
├── scripts/            # Paquete de experimentos reproducibles
└── README.md
```

## Compilar y ejecutar

```bash
cd kernel_complete
./compile.sh          # genera ./kernel-sim usando g++
./kernel-sim          # inicia la CLI interactiva
# Alternativa:
# cmake -S . -B build && cmake --build build && ./build/kernel-sim
```

## Comandos principales

- **CPU**: `cpu-rr <q>`, `cpu-sjf`, `new <burst>`, `ps`, `tick`, `run <n>`, `kill <pid>`, `suspend <pid>`, `resume <pid>`, `cpu-stats`
- **Memoria**: `mem-init <frames> [fifo|lru|pff]`, `mem-policy <alg>`, `mem-access <pid> <page>`, `mem-frames`, `mem-table <pid>`, `mem-stats`, `mem-pff-stats`, `mem-reset`
- **Sincronización**: `pc-init`, `produce`, `consume`, `pc-buffer`, `pc-stats`, `pc-reset`, `phil-*`
- **I/O**: `io-init <name> <spool>`, `io-devices`, `io-device <name>`, `io-request <dev> <pid> <dur> <prio>`, `io-run <ticks>`, `io-reset`
- **Heap**: `heap-init <total> <min>`, `heap-alloc <bytes>`, `heap-free <id>`, `heap-stats`
- **Disco**: `disk-init <tracks>`, `disk-head <pos>`, `disk-queue …`, `disk-add <cylinder>`, `disk-run <fcfs|sstf|scan|scan-down>`, `disk-stats`, `disk-view`, `disk-reset`
- **Generales**: `help`, `clear`, `exit`

## Scripts reproducibles (`scripts/`)

| Script | Módulo | Descripción |
|--------|--------|-------------|
| `mem_fifo_vs_lru.txt` | Memoria | Misma traza en FIFO y LRU, imprime `mem-frames` / `mem-stats`. |
| `mem_pff.txt` | Memoria | Ventanas con alta/baja tasa de fallos para observar PFF. |
| `proc_rr_vs_sjf.txt` | CPU | Misma carga bajo Round Robin y SJF, compara `cpu-stats`. |
| `proc_rr_io.txt` | CPU + I/O | Proceso que solicita I/O, se bloquea y retoma al completar. |
| `disk_fcfs_scan.txt` | Disco | Cola `[10 22 20 2 40 6 38]` con FCFS, SSTF y SCAN + `disk-view`. |

Ejecuta cualquiera con:

```bash
./kernel_complete/kernel-sim < scripts/<nombre>.txt
```

## Demo sugerida

Consulta `kernel_complete/DEMO_EXPO.txt` para ver el orden recomendado durante la exposición (CPU → Memoria → Productor/Consumidor → Filósofos → I/O → Heap → Disco).

## Licencia

Proyecto académico para la asignatura de Sistemas Operativos (Universidad EAFIT, 2025-2). Puedes reutilizar el código con fines educativos citando la fuente.

