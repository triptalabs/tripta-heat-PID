/**
 * @file statistics.c
 * @brief Implementación del módulo de estadísticas del horno de vacío
 * @details Este módulo implementa la funcionalidad para recolectar y almacenar estadísticas de uso
 * @author TriptaLabs
 * @version 1.0
 * @date 2025-06-17
 */

#include "statistics.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

#define TAG "STATISTICS"
#define NVS_NAMESPACE "statistics"
#define STATS_UPDATE_PERIOD_MS (1000)  // Actualizar cada segundo

// Variable global para almacenar las estadísticas
static statistics_data_t g_stats = {0};
static bool g_stats_initialized = false;
static esp_timer_handle_t g_stats_timer = NULL;

// Prototipos de funciones privadas
static void statistics_timer_callback(void* arg);
static esp_err_t statistics_save_single_value(const char* key, const void* value, size_t length);
static esp_err_t statistics_load_single_value(const char* key, void* value, size_t* length);
static uint64_t get_current_timestamp_ms(void);
static void format_time_duration(uint64_t seconds, char* buffer, size_t buffer_size);

esp_err_t statistics_init(void)
{
    ESP_LOGI(TAG, "Inicializando módulo de estadísticas");

    // Inicializar NVS si no está inicializado
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Cargar estadísticas desde NVS
    ret = statistics_load_from_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No se pudieron cargar las estadísticas desde NVS, usando valores por defecto");
        memset(&g_stats, 0, sizeof(statistics_data_t));
    }

    // Crear timer para actualización periódica
    const esp_timer_create_args_t timer_args = {
        .callback = &statistics_timer_callback,
        .arg = NULL,
        .name = "stats_timer"
    };

    ret = esp_timer_create(&timer_args, &g_stats_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear timer de estadísticas: %s", esp_err_to_name(ret));
        return ret;
    }

    // Iniciar el timer
    ret = esp_timer_start_periodic(g_stats_timer, STATS_UPDATE_PERIOD_MS * 1000); // microsegundos
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar timer de estadísticas: %s", esp_err_to_name(ret));
        return ret;
    }

    g_stats_initialized = true;
    ESP_LOGI(TAG, "Módulo de estadísticas inicializado correctamente");
    
    return ESP_OK;
}

esp_err_t statistics_start_session(void)
{
    if (!g_stats_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Iniciando nueva sesión");

    // Si ya hay una sesión activa, finalizarla primero
    if (g_stats.session_active) {
        statistics_end_session();
    }

    g_stats.session_active = true;
    g_stats.last_session_start = get_current_timestamp_ms();
    g_stats.total_sessions++;

    // Guardar estadísticas actualizadas
    esp_err_t ret = statistics_save_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Error al guardar estadísticas: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

esp_err_t statistics_end_session(void)
{
    if (!g_stats_initialized || !g_stats.session_active) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Finalizando sesión actual");

    // Calcular tiempo de la sesión
    uint64_t current_time = get_current_timestamp_ms();
    uint64_t session_duration = (current_time - g_stats.last_session_start) / 1000; // convertir a segundos
    
    g_stats.total_operation_time_seconds += session_duration;
    g_stats.session_active = false;

    // Si el SSR estaba activo, actualizar el tiempo de calentamiento
    if (g_stats.ssr_last_state && g_stats.ssr_last_change_time > 0) {
        uint64_t heating_duration = (current_time - g_stats.ssr_last_change_time) / 1000;
        g_stats.total_heating_time_seconds += heating_duration;
    }

    // Guardar estadísticas actualizadas
    esp_err_t ret = statistics_save_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Error al guardar estadísticas: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

esp_err_t statistics_update_ssr_state(bool ssr_active)
{
    if (!g_stats_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint64_t current_time = get_current_timestamp_ms();

    // Si hay un cambio de estado
    if (g_stats.ssr_last_state != ssr_active) {
        ESP_LOGD(TAG, "Cambio de estado SSR: %s -> %s", 
                 g_stats.ssr_last_state ? "ON" : "OFF",
                 ssr_active ? "ON" : "OFF");

        // Si el SSR estaba activo, sumar el tiempo de calentamiento
        if (g_stats.ssr_last_state && g_stats.ssr_last_change_time > 0) {
            uint64_t heating_duration = (current_time - g_stats.ssr_last_change_time) / 1000;
            g_stats.total_heating_time_seconds += heating_duration;
        }

        // Incrementar contador de ciclos si se activa el SSR
        if (ssr_active) {
            g_stats.ssr_cycle_count++;
        }

        // Actualizar estado y timestamp
        g_stats.ssr_last_state = ssr_active;
        g_stats.ssr_last_change_time = current_time;
    }

    return ESP_OK;
}

esp_err_t statistics_get_data(statistics_data_t *stats)
{
    if (!g_stats_initialized || !stats) {
        return ESP_ERR_INVALID_ARG;
    }

    // Actualizar tiempos actuales antes de devolver los datos
    statistics_periodic_update();

    memcpy(stats, &g_stats, sizeof(statistics_data_t));
    return ESP_OK;
}

esp_err_t statistics_get_formatted(statistics_formatted_t *formatted)
{
    if (!g_stats_initialized || !formatted) {
        return ESP_ERR_INVALID_ARG;
    }

    // Actualizar tiempos actuales
    statistics_periodic_update();

    // Formatear tiempo total de operación
    format_time_duration(g_stats.total_operation_time_seconds, 
                        formatted->total_operation_time, 
                        sizeof(formatted->total_operation_time));

    // Formatear tiempo de calentamiento
    format_time_duration(g_stats.total_heating_time_seconds, 
                        formatted->total_heating_time, 
                        sizeof(formatted->total_heating_time));

    // Formatear contador de ciclos SSR
    snprintf(formatted->ssr_cycle_count, sizeof(formatted->ssr_cycle_count), 
             "%lu", (unsigned long)g_stats.ssr_cycle_count);

    // Formatear número de sesiones
    snprintf(formatted->total_sessions, sizeof(formatted->total_sessions), 
             "%lu", (unsigned long)g_stats.total_sessions);

    return ESP_OK;
}

esp_err_t statistics_save_to_nvs(void)
{
    esp_err_t ret;

    // Guardar cada campo individualmente
    ret = statistics_save_single_value("total_op_time", &g_stats.total_operation_time_seconds, sizeof(uint64_t));
    if (ret != ESP_OK) return ret;

    ret = statistics_save_single_value("total_heat_time", &g_stats.total_heating_time_seconds, sizeof(uint64_t));
    if (ret != ESP_OK) return ret;

    ret = statistics_save_single_value("ssr_cycles", &g_stats.ssr_cycle_count, sizeof(uint32_t));
    if (ret != ESP_OK) return ret;

    ret = statistics_save_single_value("total_sessions", &g_stats.total_sessions, sizeof(uint32_t));
    if (ret != ESP_OK) return ret;

    ESP_LOGD(TAG, "Estadísticas guardadas en NVS");
    return ESP_OK;
}

esp_err_t statistics_load_from_nvs(void)
{
    esp_err_t ret;
    size_t required_size;

    // Cargar cada campo individualmente
    required_size = sizeof(uint64_t);
    ret = statistics_load_single_value("total_op_time", &g_stats.total_operation_time_seconds, &required_size);
    if (ret != ESP_OK) g_stats.total_operation_time_seconds = 0;

    required_size = sizeof(uint64_t);
    ret = statistics_load_single_value("total_heat_time", &g_stats.total_heating_time_seconds, &required_size);
    if (ret != ESP_OK) g_stats.total_heating_time_seconds = 0;

    required_size = sizeof(uint32_t);
    ret = statistics_load_single_value("ssr_cycles", &g_stats.ssr_cycle_count, &required_size);
    if (ret != ESP_OK) g_stats.ssr_cycle_count = 0;

    required_size = sizeof(uint32_t);
    ret = statistics_load_single_value("total_sessions", &g_stats.total_sessions, &required_size);
    if (ret != ESP_OK) g_stats.total_sessions = 0;

    ESP_LOGI(TAG, "Estadísticas cargadas desde NVS - Sesiones: %lu, Ciclos SSR: %lu, Tiempo operación: %llu min", 
             (unsigned long)g_stats.total_sessions,
             (unsigned long)g_stats.ssr_cycle_count,
             (unsigned long long)(g_stats.total_operation_time_seconds / 60));

    return ESP_OK;
}

esp_err_t statistics_reset(void)
{
    if (!g_stats_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Reseteando todas las estadísticas");

    memset(&g_stats, 0, sizeof(statistics_data_t));
    
    esp_err_t ret = statistics_save_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al guardar estadísticas reseteadas: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

void statistics_periodic_update(void)
{
    if (!g_stats_initialized) {
        return;
    }

    // Esta función se mantiene por compatibilidad y para futuras expansiones
    // Las estadísticas se actualizan en tiempo real cuando ocurren los eventos:
    // - Tiempo de operación: al finalizar sesión (statistics_end_session)
    // - Tiempo de calentamiento: al cambiar estado SSR (statistics_update_ssr_state)
    // - Ciclos SSR: al activar SSR (statistics_update_ssr_state)
    // - Sesiones: al iniciar sesión (statistics_start_session)
}

// Funciones privadas

static void statistics_timer_callback(void* arg)
{
    statistics_periodic_update();
}

static esp_err_t statistics_save_single_value(const char* key, const void* value, size_t length)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, key, value, length);
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    return ret;
}

static esp_err_t statistics_load_single_value(const char* key, void* value, size_t* length)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_get_blob(nvs_handle, key, value, length);
    nvs_close(nvs_handle);
    
    return ret;
}

static uint64_t get_current_timestamp_ms(void)
{
    return esp_timer_get_time() / 1000; // Convertir de microsegundos a milisegundos
}

static void format_time_duration(uint64_t seconds, char* buffer, size_t buffer_size)
{
    uint64_t hours = seconds / 3600;
    uint64_t minutes = (seconds % 3600) / 60;
    uint64_t remaining_seconds = seconds % 60;

    if (hours > 0) {
        snprintf(buffer, buffer_size, "%lluh %llumin", 
                (unsigned long long)hours, (unsigned long long)minutes);
    } else if (minutes > 0) {
        snprintf(buffer, buffer_size, "%llumin %llus", 
                (unsigned long long)minutes, (unsigned long long)remaining_seconds);
    } else {
        snprintf(buffer, buffer_size, "%llus", (unsigned long long)remaining_seconds);
    }
} 