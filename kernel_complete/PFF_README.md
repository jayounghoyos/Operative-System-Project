# PFF (Page Fault Frequency) - Gestión Avanzada de Memoria

## 📋 Descripción

PFF (Page Fault Frequency) es un algoritmo **avanzado** de gestión de memoria que ajusta dinámicamente el número de frames asignados a cada proceso basándose en su **tasa de fallos de página**.

Este algoritmo cumple con el requisito del proyecto:
> "Añadir UNO entre PFF o Working Set (para reflejar gestión avanzada)"

## 🎯 Objetivo

Prevenir **thrashing** (exceso de intercambio de páginas) optimizando la asignación de frames entre procesos con diferentes patrones de acceso a memoria.

## 🧠 Algoritmo

### Principio de Funcionamiento

1. **Medición:** Calcula la tasa de fallos por proceso en una ventana de tiempo
2. **Decisión:**
   - Si `tasa_fallos > umbral_superior` → **Aumentar frames** asignados
   - Si `tasa_fallos < umbral_inferior` → **Reducir frames** asignados
   - Si está entre umbrales → **Mantener** asignación actual
3. **Ajuste:** Los frames liberados van a un pool global para otros procesos

### Configuración por Defecto

```cpp
Ventana de medición:  10 accesos
Umbral superior:      60% (0.6)
Umbral inferior:      10% (0.1)
Frames mínimo/proc:   2 frames
Frames máximo/proc:   50% del total
```

### Fórmula de Tasa de Fallos

```
tasa_fallos = page_faults_recientes / accesos_totales_en_ventana
```

## 💻 Uso

### Comandos CLI

```bash
# Inicializar memoria con PFF
mem-init <frames> pff

# Ejemplo: 8 frames con PFF
mem-init 8 pff

# Acceder a páginas (triggers PFF logic)
mem-access <pid> <page>

# Ver estadísticas PFF detalladas
mem-pff-stats

# Ver estado de frames
mem-frames

# Cambiar a PFF desde otro algoritmo
mem-policy pff
```

### Ejemplo de Uso

```bash
kernel> mem-init 8 pff
[MEMORY] Inicializada con 8 frames (política=PFF)
[PFF] Configuración:
  ├─ Ventana de medición: 10 accesos
  ├─ Umbral superior:     60%
  ├─ Umbral inferior:     10%
  ├─ Frames mín/proceso:  2
  └─ Frames máx/proceso:  4

kernel> mem-access 1 0
[PFF] Proceso P1 inicializado con 2 frames
[PAGE FAULT #1] P1 página 0
  └─ Página cargada en frame 0

kernel> # ... más accesos ...

kernel> mem-pff-stats
╔═══ ESTADÍSTICAS PFF (Page Fault Frequency) ═══╗

 Procesos activos:
Proceso   Frames      Tasa Fallos    Accesos     Fallos      Estado
P1        3/4         80%            10          8           ALTA
P2        2/2         15%            10          2           NORMAL
P3        1/2         5%             10          1           BAJA
```

## 📊 Comparación con FIFO y LRU

| Característica | FIFO | LRU | PFF |
|----------------|------|-----|-----|
| **Complejidad** | O(1) | O(1) | O(1) por acceso + O(n) por ajuste |
| **Adaptación** | ❌ Fija | ❌ Fija | ✅ Dinámica |
| **Prevención Thrashing** | ❌ No | ⚠️ Parcial | ✅ Sí |
| **Multi-proceso** | ⚠️ Igual para todos | ⚠️ Igual para todos | ✅ Optimizado |
| **Working Set** | ❌ No detecta | ⚠️ Implícito | ✅ Automático |

## 🎬 Demo

Ejecutar el script de demostración completo:

```bash
# Ver demo completa con casos de uso
cat DEMO_PFF.txt

# Ejecutar interactivamente
./kernel-sim < DEMO_PFF.txt
```

### Casos de Uso en Demo

1. **Proceso con alta tasa de fallos** → PFF aumenta frames
2. **Proceso con baja tasa de fallos** → PFF reduce frames
3. **Cambio dinámico de working set** → PFF se adapta
4. **Múltiples procesos** → PFF balancea recursos

## 🔬 Detalles de Implementación

### Estructuras de Datos

```cpp
struct PFFProcessInfo {
    int allocated_frames;      // Frames asignados actualmente
    int min_frames;            // Mínimo garantizado
    int max_frames;            // Máximo permitido
    int recent_faults;         // Fallos en ventana actual
    int recent_accesses;       // Accesos en ventana actual
    int window_start_time;     // Inicio de medición
    double fault_rate;         // Tasa calculada
};
```

### Funciones Clave

- `pff_initialize_process()` - Inicializa proceso con frames mínimos
- `pff_update_stats()` - Actualiza contadores por acceso
- `pff_adjust_allocation()` - Ajusta frames al final de ventana
- `select_victim_pff()` - Selecciona víctima LRU del mismo proceso
- `display_pff_stats()` - Muestra estadísticas detalladas

### Flujo de Ejecución

```
access_page(pid, page)
    ↓
pff_initialize_process(pid) [si es nuevo]
    ↓
¿HIT o FAULT?
    ↓
pff_update_stats(pid, was_fault)
    ↓
¿Ventana completa?
    ↓
pff_adjust_allocation(pid)
    ↓
Si alta tasa → intentar aumentar frames
Si baja tasa → liberar frames
```

## 📈 Ventajas

1. **Adaptación Automática**
   - Detecta working set sin intervención manual
   - Se ajusta a cambios en patrones de acceso

2. **Prevención de Thrashing**
   - Garantiza mínimo de frames por proceso
   - Aumenta frames antes de que thrashing ocurra

3. **Eficiencia en Multiprocesamiento**
   - Procesos con buen locality liberan frames
   - Recursos van donde más se necesitan

4. **Balance Automático**
   - No requiere configuración por proceso
   - Umbrales globales son robustos

## ⚠️ Trade-offs

1. **Complejidad de Implementación**
   - Más código que FIFO/LRU
   - Requiere tracking por proceso

2. **Overhead de Tracking**
   - Memoria extra para estadísticas
   - Cómputo adicional por ventana

3. **Configuración de Umbrales**
   - Requiere elegir valores apropiados
   - Valores por defecto son razonables

4. **Latencia en Ajustes**
   - Solo ajusta al final de ventana
   - Puede tardar en reaccionar a cambios

## 🎓 Para el Informe Técnico

### Métricas a Incluir

1. **Hit Ratio** por algoritmo (FIFO vs LRU vs PFF)
2. **Page Fault Rate** bajo diferentes cargas
3. **Frame Distribution** entre procesos
4. **Tiempo de respuesta** con thrashing vs sin thrashing
5. **Adaptación** a cambios de working set

### Gráficos Recomendados

1. Tasa de fallos vs tiempo (mostrar ajustes de PFF)
2. Distribución de frames entre procesos
3. Comparación FIFO vs LRU vs PFF en hit ratio
4. Impacto de umbrales en rendimiento

### Conclusiones Esperadas

```
PFF es superior cuando:
- Procesos tienen working sets variables
- Hay riesgo de thrashing
- Multiprocesamiento con cargas desiguales

FIFO/LRU son preferibles cuando:
- Working sets son estables
- Overhead de PFF no se justifica
- Simplicidad es prioridad
```

## 📚 Referencias

- Denning, P. J. (1968). "The working set model for program behavior"
- Chu, W. W., & Opderbeck, H. (1976). "The page fault frequency replacement algorithm"
- Tanenbaum, A. S. "Modern Operating Systems" - Sección de Memory Management

## ✅ Cumplimiento de Requisitos

✅ **Requisito PDF:** "Añadir UNO entre PFF o Working Set"
✅ **Implementado:** PFF completo con ajuste dinámico
✅ **Visualización:** Estadísticas detalladas por proceso
✅ **CLI:** Comandos `mem-init pff`, `mem-pff-stats`
✅ **Demo:** Script completo con casos de uso
✅ **Documentación:** Este README + comentarios en código

---

**Implementado por:** Kernel Simulator Team
**Fecha:** Noviembre 2025
**Versión:** 1.0
**Proyecto:** Sistemas Operativos - Universidad EAFIT
