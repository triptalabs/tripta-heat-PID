# Propuesta de Reordenamiento del Directorio `main/core`

## 1. Objetivo

Diseñar una arquitectura de carpetas **clara, escalable y mantenible** que elimine el código espagueti, agilice la comprensión del proyecto y facilite la incorporación de nuevas funcionalidades.

## 2. Diagnóstico Actual

* Todos los _sources_ (`*.c`) y _headers_ (`*.h`) conviven en `main/core/`.
* El código mezcla **lógica de aplicación**, **abstracciones de dominio** y **servicios** (WiFi, OTA, tiempo) sin separación explícita.
* Los _headers_ públicos y privados están mezclados, dificultando saber qué API exporta cada módulo.

## 3. Principios de Diseño Adoptados

1. **Alta cohesión, bajo acoplamiento**.
2. **Árbol de dependencias unidireccional** (capas inferiores nunca dependen de capas superiores).
3. Organización por _contexto funcional_ (Domain Driven) y no por tipo de archivo.
4. Crear una carpeta `include/` para exponer únicamente la API pública.

## 4. Nueva Estructura Propuesta

```text
main/
└─ core/
   ├─ include/              # Headers públicos (visible al resto del proyecto)
   │   ├─ pid/
   │   │   └─ pid_controller.h
   │   ├─ autotuning/
   │   │   └─ autotuning.h
   │   ├─ ota/
   │   │   └─ update.h
   │   ├─ wifi/
   │   │   └─ wifi_manager.h
   │   ├─ stats/
   │   │   └─ statistics.h
   │   └─ system/
   │       ├─ system_time.h
   │       └─ system_test.h
   ├─ pid/                  # Implementación de la lógica de control PID
   │   ├─ pid_controller.c
   │   └─ private/
   │       └─ pid_internal.h
   ├─ autotuning/           # Métodos de autotuning
   │   ├─ autotuning.c
   │   ├─ ziegler_nichols.c
   │   └─ astrom_hagglund.c
   ├─ ota/                  # Actualizaciones y recuperación
   │   └─ update.c
   ├─ wifi/                 # Conectividad WiFi + BT
   │   └─ wifi_manager.c
   ├─ stats/                # Estadísticas y métricas
   │   └─ statistics.c
   ├─ system/               # Utilidades de sistema (tiempo, tests, BT)
   │   ├─ system_time.c
   │   ├─ system_test.c
   │   └─ bt.c
   └─ core.c                # Punto único de inicialización (reemplaza main.c)
```

### Ventajas

* **API clara** → Cualquier desarrollador sabe buscar headers en `core/include`.
* **Independencia** → Cada carpeta contiene su dominio; módulos solo exponen lo necesario.
* **Escalabilidad** → Nuevas funcionalidades (p. ej. MQTT) se añaden como otro sub-dominio.
* **Compilación selectiva** → CMake puede incluir subdirectorios de forma explícita.

## 5. Mapeo de Archivos

| Archivo actual | Ubicación nueva |
|----------------|-----------------|
| `core/pid_controller.*` | `core/pid/` + header en `core/include/pid/` |
| `core/autotuning/*` | `core/autotuning/` + header en `core/include/autotuning/` |
| `core/update.*` | `core/ota/` + header en `core/include/ota/` |
| `core/wifi_manager.*` | `core/wifi/` + header en `core/include/wifi/` |
| `core/statistics.*` | `core/stats/` + header en `core/include/stats/` |
| `core/system_time.*` | `core/system/` + header correspondiente |
| `core/system_test.*` | `core/system/` + header correspondiente |
| `core/bt.*` | `core/system/` (o `core/bluetooth/` si crece) |
| `core/main.c` | Renombrado a `core/core.c` (único punto de inicialización) |

## 6. Cambios en CMakeLists.txt

```cmake
# main/CMakeLists.txt (extracto)
idf_component_register(
    SRCS
        "core/core.c"              # Nuevo init
        $<TARGET_OBJECTS:pid>
        $<TARGET_OBJECTS:autotuning>
        $<TARGET_OBJECTS:ota>
        $<TARGET_OBJECTS:wifi>
        $<TARGET_OBJECTS:stats>
        $<TARGET_OBJECTS:system>
    INCLUDE_DIRS
        "core/include"
)
```
*Se recomienda usar `add_subdirectory(core/pid)` con `add_library(pid OBJECT ...)` para cada sub-dominio.*

## 7. Pasos de Migración

1. **Crear carpetas** según la nueva estructura.
2. **Mover archivos** y ajustar include guards / rutas de `#include`.
3. Añadir carpeta `core/include` a `INCLUDE_DIRS` en CMake.
4. Refactorizar `main/core/main.c` → `core/core.c` para orquestar inicializaciones (`pid_init`, `update_init`, …).
5. Ejecutar `idf.py build` y resolver advertencias.
6. Añadir _unit tests_ (Ceedling o Unity) para asegurar que el refactor no rompe funcionalidad.

## 8. Trabajo Futuro

* Separar **dominio vs. infraestructura** (p. ej. una capa `platform/` para HAL, reutilizable en tests host).  
* Introducir **Doxygen** en `core/include` para generar documentación automática de la API.  
* Implementar CI (GitHub Actions) con compilación + static-analysis.

---
> _Documento generado por propuesta de refactor — {{ fecha }}_ 