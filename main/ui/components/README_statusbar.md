# Módulo StatusBar Manager

## Descripción

El módulo `statusbar_manager` es una solución moderna y modular para gestionar la barra de estado con hora en tiempo real y control dinámico de iconos. Reemplaza la implementación hardcodeada anterior con un sistema flexible y configurable.

## Características

- ✅ **Hora en tiempo real**: Actualización automática de fecha y hora
- ✅ **Control dinámico de iconos**: Mostrar/ocultar iconos según el estado del sistema
- ✅ **Configuración flexible**: Personalizar formato de hora, intervalos de actualización
- ✅ **Gestión de memoria**: Sin memory leaks, cleanup automático
- ✅ **Logging integrado**: Monitoreo del estado y errores
- ✅ **Thread-safe**: Uso seguro con LVGL y FreeRTOS

## Iconos Disponibles

| Icono | Descripción | Uso típico |
|-------|-------------|------------|
| `STATUSBAR_ICON_WIFI` | WiFi/Conectividad | Mostrar cuando WiFi está conectado |
| `STATUSBAR_ICON_BLUETOOTH` | Bluetooth | Mostrar cuando Bluetooth está habilitado |
| `STATUSBAR_ICON_HEATING` | Calentamiento | Mostrar durante operación de calentamiento |
| `STATUSBAR_ICON_WARNING` | Advertencia | Mostrar cuando hay errores o alertas |
| `STATUSBAR_ICON_UPDATE` | Actualización | Mostrar cuando hay actualizaciones disponibles |

## Uso Básico

### 1. Inicialización

```c
#include "statusbar_manager.h"

// En main.c o función de inicialización
if (lvgl_port_lock(-1)) {
    ui_init(); // Inicializar UI primero
    
    // Configurar módulo de barra de estado
    statusbar_config_t config = statusbar_get_default_config();
    config.time_format = "%d %b %Y   |   %H:%M";
    config.time_update_interval_ms = 60000;  // 1 minuto
    
    if (!statusbar_manager_init(ui_STATUSBAR, &config)) {
        ESP_LOGE(TAG, "Error al inicializar statusbar_manager");
    }
    
    lvgl_port_unlock();
}
```

### 2. Control de Iconos

```c
// Mostrar icono de WiFi cuando se conecta
statusbar_set_icon_visible(STATUSBAR_ICON_WIFI, true);

// Ocultar icono de calentamiento cuando se apaga
statusbar_set_icon_visible(STATUSBAR_ICON_HEATING, false);

// Mostrar icono de advertencia
statusbar_set_icon_visible(STATUSBAR_ICON_WARNING, true);
```

### 3. Personalización de Hora

```c
// Formato personalizado
statusbar_config_t config = statusbar_get_default_config();
config.time_format = "%H:%M:%S";  // Solo hora con segundos
config.time_update_interval_ms = 1000;  // Actualizar cada segundo
statusbar_update_config(&config);

// Texto personalizado temporal
statusbar_set_custom_time_text("Actualizando...");

// Volver a hora automática
statusbar_set_custom_time_text(NULL);
```

## Configuración Avanzada

### Estructura de Configuración

```c
typedef struct {
    uint32_t time_update_interval_ms;   // Intervalo de actualización (default: 60000)
    bool enable_auto_time_update;       // Habilitar actualización automática (default: true)
    const char* time_format;            // Formato strftime (default: "%d %b %Y   |   %H:%M")
    const char* no_time_text;           // Texto sin hora válida (default: "Sin hora")
} statusbar_config_t;
```

### Formatos de Fecha/Hora Comunes

| Formato | Resultado | Descripción |
|---------|-----------|-------------|
| `"%d %b %Y   \|   %H:%M"` | `27 Ene 2025   \|   14:30` | Formato por defecto |
| `"%H:%M"` | `14:30` | Solo hora |
| `"%d/%m/%Y %H:%M"` | `27/01/2025 14:30` | Formato numérico |
| `"%I:%M %p"` | `02:30 PM` | Formato 12 horas |
| `"%A, %d %B"` | `Lunes, 27 Enero` | Formato largo |

## Integración con Otros Módulos

### WiFi Manager

```c
// En wifi_manager.c - después de conectar WiFi
statusbar_set_icon_visible(STATUSBAR_ICON_WIFI, true);

// En caso de desconexión
statusbar_set_icon_visible(STATUSBAR_ICON_WIFI, false);
```

### PID Controller

```c
// En pid_controller.c - cuando inicia calentamiento
statusbar_set_icon_visible(STATUSBAR_ICON_HEATING, true);

// Cuando termina calentamiento
statusbar_set_icon_visible(STATUSBAR_ICON_HEATING, false);
```

### Sistema de Actualizaciones

```c
// Cuando se detecta actualización disponible
statusbar_set_icon_visible(STATUSBAR_ICON_UPDATE, true);

// Durante actualización
statusbar_set_custom_time_text("Actualizando firmware...");

// Después de actualización
statusbar_set_icon_visible(STATUSBAR_ICON_UPDATE, false);
statusbar_set_custom_time_text(NULL);
```

## API Reference

### Funciones Principales

- `statusbar_manager_init()` - Inicializar módulo
- `statusbar_manager_deinit()` - Limpiar recursos
- `statusbar_set_icon_visible()` - Controlar visibilidad de iconos
- `statusbar_update_time()` - Actualizar hora manualmente
- `statusbar_set_custom_time_text()` - Texto personalizado
- `statusbar_update_config()` - Actualizar configuración

### Funciones de Consulta

- `statusbar_get_icon_visible()` - Obtener estado de icono
- `statusbar_get_icon_object()` - Obtener objeto LVGL de icono
- `statusbar_get_default_config()` - Obtener configuración por defecto

## Migración desde Implementación Anterior

### Cambios Realizados

1. **Eliminado**: Variable global `cui_datetime1`
2. **Eliminado**: Timer hardcodeado `lv_timer_create(actualizar_hora_cb, ...)`
3. **Eliminado**: Función `actualizar_hora_cb()` (ahora deprecada)
4. **Modificado**: `wifi_manager_init()` ya no requiere `datetime_label`

### Pasos de Migración

1. Reemplazar inicialización hardcodeada en `main.c`
2. Usar `statusbar_manager_init()` en lugar del timer manual
3. Reemplazar referencias a `cui_datetime1` con API del módulo
4. Actualizar control de iconos desde código hardcodeado a dinámico

## Ejemplos Completos

Ver archivos:
- `statusbar_example.c` - Funciones de ejemplo
- `statusbar_example.h` - API de ejemplo

Para ejecutar ejemplos:

```c
// Llamar periódicamente para demostrar funcionalidad
statusbar_simulate_system_changes();

// O integrar en tu lógica específica
statusbar_update_system_icons();
```

## Debugging

### Logs Útiles

```bash
# Filtrar logs del módulo
idf.py monitor | grep "StatusBar"

# Verificar inicialización
I (12345) StatusBar: Manager inicializado correctamente

# Monitorear cambios de iconos
D (12346) StatusBar: Icono 0 ahora está visible
```

### Problemas Comunes

1. **Iconos no aparecen**: Verificar que `statusbar_manager_init()` se llamó correctamente
2. **Hora no actualiza**: Verificar configuración de SNTP y zona horaria
3. **Memory leaks**: Siempre llamar `statusbar_manager_deinit()` al finalizar

## Notas Técnicas

- El módulo usa un patrón singleton interno
- Thread-safe cuando se usa con `lvgl_port_lock()`
- Compatible con LVGL 8.3.11+
- Requiere ESP-IDF 4.4+

## Futuras Mejoras

- [ ] Animaciones para iconos
- [ ] Temas personalizables
- [ ] Más tipos de iconos
- [ ] Sincronización con servidor NTP personalizado
- [ ] Configuración persistente en NVS 