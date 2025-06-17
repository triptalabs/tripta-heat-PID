/**
 * @file statusbar_manager.h
 * @brief Módulo para gestión dinámica de la barra de estado con hora y control de iconos
 * 
 * Este módulo permite controlar dinámicamente los iconos de la barra de estado (WiFi, Bluetooth,
 * calentamiento, advertencias, actualizaciones) y mantiene actualizada la hora en tiempo real.
 * Reemplaza la implementación hardcodeada anterior con un sistema modular y configurable.
 * 
 * @version 1.0
 * @date 2025-01-27
 * 
 * @note Utiliza LVGL para la interfaz gráfica y FreeRTOS para la gestión de tareas.
 */

#ifndef STATUSBAR_MANAGER_H
#define STATUSBAR_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

/**
 * @brief Enumeración de los iconos disponibles en la barra de estado
 */
typedef enum {
    STATUSBAR_ICON_WIFI = 0,        ///< Icono de WiFi
    STATUSBAR_ICON_BLUETOOTH,       ///< Icono de Bluetooth
    STATUSBAR_ICON_HEATING,         ///< Icono de calentamiento
    STATUSBAR_ICON_WARNING,         ///< Icono de advertencia
    STATUSBAR_ICON_UPDATE,          ///< Icono de actualización
    STATUSBAR_ICON_COUNT            ///< Número total de iconos
} statusbar_icon_t;

/**
 * @brief Estructura de configuración para el módulo de barra de estado
 */
typedef struct {
    uint32_t time_update_interval_ms;   ///< Intervalo de actualización de hora en ms (default: 60000)
    bool enable_auto_time_update;       ///< Habilitar actualización automática de hora
    const char* time_format;            ///< Formato de fecha/hora (default: "%d %b %Y   |   %H:%M")
    const char* no_time_text;           ///< Texto a mostrar cuando no hay hora válida
} statusbar_config_t;

/**
 * @brief Estructura que representa el estado del módulo de barra de estado
 */
typedef struct {
    lv_obj_t* statusbar_obj;            ///< Objeto principal de la barra de estado
    lv_obj_t* datetime_label;           ///< Etiqueta de fecha/hora
    lv_obj_t* icons[STATUSBAR_ICON_COUNT]; ///< Array de objetos de iconos
    lv_timer_t* time_timer;             ///< Timer para actualización de hora
    statusbar_config_t config;          ///< Configuración del módulo
    bool icons_visible[STATUSBAR_ICON_COUNT]; ///< Estado de visibilidad de iconos
    bool initialized;                   ///< Flag de inicialización
} statusbar_manager_t;

/**
 * @brief Obtiene la configuración por defecto para el módulo de barra de estado
 * 
 * @return statusbar_config_t Configuración por defecto
 */
statusbar_config_t statusbar_get_default_config(void);

/**
 * @brief Inicializa el módulo de gestión de barra de estado
 * 
 * @param statusbar_obj Objeto LVGL de la barra de estado existente
 * @param config Configuración del módulo (puede ser NULL para usar valores por defecto)
 * @return true si la inicialización fue exitosa, false en caso contrario
 */
bool statusbar_manager_init(lv_obj_t* statusbar_obj, const statusbar_config_t* config);

/**
 * @brief Desinicializa el módulo de gestión de barra de estado
 */
void statusbar_manager_deinit(void);

/**
 * @brief Controla la visibilidad de un icono específico
 * 
 * @param icon Icono a controlar
 * @param visible true para mostrar, false para ocultar
 * @return true si la operación fue exitosa, false en caso contrario
 */
bool statusbar_set_icon_visible(statusbar_icon_t icon, bool visible);

/**
 * @brief Obtiene el estado de visibilidad de un icono
 * 
 * @param icon Icono a consultar
 * @return true si está visible, false si está oculto o hubo error
 */
bool statusbar_get_icon_visible(statusbar_icon_t icon);

/**
 * @brief Actualiza manualmente la hora mostrada en la barra de estado
 * 
 * @param force_update true para forzar actualización aunque no haya cambiado la hora
 * @return true si la actualización fue exitosa, false en caso contrario
 */
bool statusbar_update_time(bool force_update);

/**
 * @brief Establece un texto personalizado para la hora (anula la hora automática)
 * 
 * @param custom_text Texto personalizado a mostrar (NULL para volver a hora automática)
 */
void statusbar_set_custom_time_text(const char* custom_text);

/**
 * @brief Habilita o deshabilita la actualización automática de hora
 * 
 * @param enabled true para habilitar, false para deshabilitar
 */
void statusbar_set_auto_time_update(bool enabled);

/**
 * @brief Obtiene el objeto LVGL de un icono específico
 * 
 * @param icon Icono del que obtener el objeto
 * @return lv_obj_t* Objeto LVGL del icono, o NULL si no existe
 */
lv_obj_t* statusbar_get_icon_object(statusbar_icon_t icon);

/**
 * @brief Actualiza la configuración del módulo en tiempo de ejecución
 * 
 * @param config Nueva configuración
 * @return true si la actualización fue exitosa, false en caso contrario
 */
bool statusbar_update_config(const statusbar_config_t* config);

/**
 * @brief Callback interno para actualización periódica de hora (no usar directamente)
 * 
 * @param timer Timer LVGL que activó el callback
 */
void _statusbar_time_callback(lv_timer_t* timer);

#ifdef __cplusplus
}
#endif

#endif // STATUSBAR_MANAGER_H 