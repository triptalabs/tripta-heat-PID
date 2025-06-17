/**
 * @file statusbar_manager.c
 * @brief Implementación del módulo para gestión dinámica de la barra de estado
 * 
 * Este archivo contiene la implementación completa del módulo de gestión de barra de estado,
 * incluyendo control dinámico de iconos, actualización de hora en tiempo real y configuración
 * flexible del comportamiento del módulo.
 * 
 * @version 1.0
 * @date 2025-01-27
 */

#include "statusbar_manager.h"
#include "ui_comp_statusbar.h"
#include "esp_log.h"
#include <time.h>
#include <string.h>

static const char* TAG = "StatusBar";

// Instancia global del manager (singleton)
static statusbar_manager_t g_statusbar_manager = {0};

// Array de mapeo de iconos a sus índices en el componente statusbar
static const int ICON_COMPONENT_MAPPING[STATUSBAR_ICON_COUNT] = {
    UI_COMP_STATUSBAR_ICONS_ICONWIFI,     // STATUSBAR_ICON_WIFI
    UI_COMP_STATUSBAR_ICONS_ICONBT,       // STATUSBAR_ICON_BLUETOOTH  
    UI_COMP_STATUSBAR_ICONS_ICONHT,       // STATUSBAR_ICON_HEATING
    UI_COMP_STATUSBAR_ICONS_ICONWARN,     // STATUSBAR_ICON_WARNING
    UI_COMP_STATUSBAR_ICONS_ICONUD        // STATUSBAR_ICON_UPDATE
};

/**
 * @brief Obtiene el objeto LVGL de un icono desde el componente statusbar
 * 
 * @param icon Icono del que obtener el objeto
 * @return lv_obj_t* Objeto LVGL del icono, o NULL si no existe
 */
static lv_obj_t* _get_icon_from_component(statusbar_icon_t icon) {
    if (!g_statusbar_manager.initialized || 
        icon >= STATUSBAR_ICON_COUNT || 
        !g_statusbar_manager.statusbar_obj) {
        return NULL;
    }
    
    int component_index = ICON_COMPONENT_MAPPING[icon];
    return ui_comp_get_child(g_statusbar_manager.statusbar_obj, component_index);
}

/**
 * @brief Callback interno para actualización periódica de hora
 * 
 * @param timer Timer LVGL que activó el callback
 */
void _statusbar_time_callback(lv_timer_t* timer) {
    (void)timer; // Suprimir warning de parámetro no usado
    statusbar_update_time(false);
}

statusbar_config_t statusbar_get_default_config(void) {
    statusbar_config_t config = {
        .time_update_interval_ms = 60000,  // 1 minuto
        .enable_auto_time_update = true,
        .time_format = "%d %b %Y   |   %H:%M",
        .no_time_text = "Sin hora"
    };
    return config;
}

bool statusbar_manager_init(lv_obj_t* statusbar_obj, const statusbar_config_t* config) {
    if (g_statusbar_manager.initialized) {
        ESP_LOGW(TAG, "Manager ya está inicializado, desinicializando primero");
        statusbar_manager_deinit();
    }
    
    if (!statusbar_obj) {
        ESP_LOGE(TAG, "Objeto statusbar no puede ser NULL");
        return false;
    }
    
    // Configuración
    if (config) {
        g_statusbar_manager.config = *config;
    } else {
        g_statusbar_manager.config = statusbar_get_default_config();
    }
    
    // Validar formato de configuración
    if (!g_statusbar_manager.config.time_format) {
        g_statusbar_manager.config.time_format = "%d %b %Y   |   %H:%M";
    }
    if (!g_statusbar_manager.config.no_time_text) {
        g_statusbar_manager.config.no_time_text = "Sin hora";
    }
    
    // Almacenar referencia al objeto statusbar
    g_statusbar_manager.statusbar_obj = statusbar_obj;
    
    // Obtener referencia al label de fecha/hora
    g_statusbar_manager.datetime_label = ui_comp_get_child(
        statusbar_obj, 
        UI_COMP_STATUSBAR_DATETIME_DATETIME1
    );
    
    if (!g_statusbar_manager.datetime_label) {
        ESP_LOGE(TAG, "No se pudo obtener el label de fecha/hora");
        return false;
    }
    
    // Mapear iconos
    for (int i = 0; i < STATUSBAR_ICON_COUNT; i++) {
        g_statusbar_manager.icons[i] = _get_icon_from_component((statusbar_icon_t)i);
        if (!g_statusbar_manager.icons[i]) {
            ESP_LOGW(TAG, "No se pudo mapear el icono %d", i);
        }
        // Por defecto todos los iconos están visibles
        g_statusbar_manager.icons_visible[i] = true;
    }
    
    // Crear timer para actualización de hora si está habilitado
    g_statusbar_manager.time_timer = NULL;
    if (g_statusbar_manager.config.enable_auto_time_update) {
        g_statusbar_manager.time_timer = lv_timer_create(
            _statusbar_time_callback,
            g_statusbar_manager.config.time_update_interval_ms,
            NULL
        );
        
        if (!g_statusbar_manager.time_timer) {
            ESP_LOGE(TAG, "No se pudo crear el timer de actualización de hora");
            return false;
        }
    }
    
    // Actualizar hora inicial
    statusbar_update_time(true);
    
    g_statusbar_manager.initialized = true;
    ESP_LOGI(TAG, "Manager inicializado correctamente");
    
    return true;
}

void statusbar_manager_deinit(void) {
    if (!g_statusbar_manager.initialized) {
        return;
    }
    
    // Eliminar timer si existe
    if (g_statusbar_manager.time_timer) {
        lv_timer_del(g_statusbar_manager.time_timer);
        g_statusbar_manager.time_timer = NULL;
    }
    
    // Limpiar referencias
    memset(&g_statusbar_manager, 0, sizeof(statusbar_manager_t));
    
    ESP_LOGI(TAG, "Manager desinicializado");
}

bool statusbar_set_icon_visible(statusbar_icon_t icon, bool visible) {
    if (!g_statusbar_manager.initialized || icon >= STATUSBAR_ICON_COUNT) {
        ESP_LOGE(TAG, "Manager no inicializado o icono inválido (%d)", icon);
        return false;
    }
    
    lv_obj_t* icon_obj = g_statusbar_manager.icons[icon];
    if (!icon_obj) {
        ESP_LOGW(TAG, "Objeto de icono %d no encontrado", icon);
        return false;
    }
    
    if (visible) {
        lv_obj_clear_flag(icon_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(icon_obj, LV_OBJ_FLAG_HIDDEN);
    }
    
    g_statusbar_manager.icons_visible[icon] = visible;
    
    ESP_LOGD(TAG, "Icono %d ahora está %s", icon, visible ? "visible" : "oculto");
    return true;
}

bool statusbar_get_icon_visible(statusbar_icon_t icon) {
    if (!g_statusbar_manager.initialized || icon >= STATUSBAR_ICON_COUNT) {
        return false;
    }
    
    return g_statusbar_manager.icons_visible[icon];
}

bool statusbar_update_time(bool force_update) {
    if (!g_statusbar_manager.initialized || !g_statusbar_manager.datetime_label) {
        return false;
    }
    
    time_t now;
    struct tm timeinfo;
    static char time_buffer[64];
    static char last_time_str[64] = {0};
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Verificar si tenemos una hora válida (año > 1970)
    if (timeinfo.tm_year > 70) {
        // Formatear la hora según la configuración
        strftime(time_buffer, sizeof(time_buffer), 
                g_statusbar_manager.config.time_format, &timeinfo);
        
        // Solo actualizar si ha cambiado o se fuerza la actualización
        if (force_update || strcmp(time_buffer, last_time_str) != 0) {
            lv_label_set_text(g_statusbar_manager.datetime_label, time_buffer);
            strcpy(last_time_str, time_buffer);
            ESP_LOGD(TAG, "Hora actualizada: %s", time_buffer);
        }
    } else {
        // No hay hora válida, mostrar texto alternativo
        if (force_update || strcmp(last_time_str, g_statusbar_manager.config.no_time_text) != 0) {
            lv_label_set_text(g_statusbar_manager.datetime_label, 
                             g_statusbar_manager.config.no_time_text);
            strcpy(last_time_str, g_statusbar_manager.config.no_time_text);
            ESP_LOGD(TAG, "Sin hora válida, mostrando: %s", g_statusbar_manager.config.no_time_text);
        }
    }
    
    return true;
}

void statusbar_set_custom_time_text(const char* custom_text) {
    if (!g_statusbar_manager.initialized || !g_statusbar_manager.datetime_label) {
        return;
    }
    
    if (custom_text) {
        // Establecer texto personalizado
        lv_label_set_text(g_statusbar_manager.datetime_label, custom_text);
        ESP_LOGD(TAG, "Texto personalizado establecido: %s", custom_text);
    } else {
        // Volver a hora automática
        statusbar_update_time(true);
        ESP_LOGD(TAG, "Volviendo a hora automática");
    }
}

void statusbar_set_auto_time_update(bool enabled) {
    if (!g_statusbar_manager.initialized) {
        return;
    }
    
    if (enabled && !g_statusbar_manager.time_timer) {
        // Crear timer si no existe
        g_statusbar_manager.time_timer = lv_timer_create(
            _statusbar_time_callback,
            g_statusbar_manager.config.time_update_interval_ms,
            NULL
        );
        
        if (g_statusbar_manager.time_timer) {
            ESP_LOGI(TAG, "Timer de actualización de hora activado");
        } else {
            ESP_LOGE(TAG, "Error al crear timer de actualización de hora");
        }
    } else if (!enabled && g_statusbar_manager.time_timer) {
        // Eliminar timer si existe
        lv_timer_del(g_statusbar_manager.time_timer);
        g_statusbar_manager.time_timer = NULL;
        ESP_LOGI(TAG, "Timer de actualización de hora desactivado");
    }
    
    g_statusbar_manager.config.enable_auto_time_update = enabled;
}

lv_obj_t* statusbar_get_icon_object(statusbar_icon_t icon) {
    if (!g_statusbar_manager.initialized || icon >= STATUSBAR_ICON_COUNT) {
        return NULL;
    }
    
    return g_statusbar_manager.icons[icon];
}

bool statusbar_update_config(const statusbar_config_t* config) {
    if (!g_statusbar_manager.initialized || !config) {
        return false;
    }
    
    // Guardar estado actual del timer
    bool timer_was_active = (g_statusbar_manager.time_timer != NULL);
    
    // Eliminar timer existente si hay uno
    if (g_statusbar_manager.time_timer) {
        lv_timer_del(g_statusbar_manager.time_timer);
        g_statusbar_manager.time_timer = NULL;
    }
    
    // Actualizar configuración
    g_statusbar_manager.config = *config;
    
    // Validar campos de configuración
    if (!g_statusbar_manager.config.time_format) {
        g_statusbar_manager.config.time_format = "%d %b %Y   |   %H:%M";
    }
    if (!g_statusbar_manager.config.no_time_text) {
        g_statusbar_manager.config.no_time_text = "Sin hora";
    }
    
    // Recrear timer si estaba activo y sigue habilitado
    if (timer_was_active && g_statusbar_manager.config.enable_auto_time_update) {
        g_statusbar_manager.time_timer = lv_timer_create(
            _statusbar_time_callback,
            g_statusbar_manager.config.time_update_interval_ms,
            NULL
        );
        
        if (!g_statusbar_manager.time_timer) {
            ESP_LOGE(TAG, "Error al recrear timer con nueva configuración");
            return false;
        }
    }
    
    // Actualizar hora con nuevo formato
    statusbar_update_time(true);
    
    ESP_LOGI(TAG, "Configuración actualizada correctamente");
    return true;
} 