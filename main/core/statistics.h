/**
 * @file statistics.h
 * @brief Módulo de estadísticas del horno de vacío
 * @details Este módulo se encarga de recolectar y almacenar estadísticas de uso del equipo:
 *          - Tiempo total de operación
 *          - Tiempo neto de calentamiento
 *          - Número de ciclos del SSR
 *          - Número total de sesiones
 * @author TriptaLabs
 * @version 1.0
 * @date 2025-06-17
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estructura para almacenar las estadísticas de uso del equipo
 */
typedef struct {
    uint64_t total_operation_time_seconds;    ///< Tiempo total de operación en segundos
    uint64_t total_heating_time_seconds;      ///< Tiempo neto de calentamiento en segundos
    uint32_t ssr_cycle_count;                 ///< Número de ciclos del SSR
    uint32_t total_sessions;                  ///< Número total de sesiones
    uint64_t last_session_start;              ///< Timestamp del inicio de la sesión actual
    bool session_active;                      ///< Flag que indica si hay una sesión activa
    bool ssr_last_state;                      ///< Último estado conocido del SSR
    uint64_t ssr_last_change_time;            ///< Timestamp del último cambio de estado del SSR
} statistics_data_t;

/**
 * @brief Estructura para formatear las estadísticas como cadenas de texto
 */
typedef struct {
    char total_operation_time[32];    ///< Tiempo total formateado (ej: "2h 30min")
    char total_heating_time[32];      ///< Tiempo de calentamiento formateado
    char ssr_cycle_count[16];         ///< Número de ciclos como string
    char total_sessions[16];          ///< Número de sesiones como string
} statistics_formatted_t;

/**
 * @brief Inicializa el módulo de estadísticas
 * @details Carga las estadísticas almacenadas en NVS y configura los callbacks necesarios
 * @return ESP_OK si la inicialización fue exitosa
 */
esp_err_t statistics_init(void);

/**
 * @brief Inicia una nueva sesión de uso
 * @details Marca el inicio de una sesión y actualiza el contador de sesiones
 * @return ESP_OK si la operación fue exitosa
 */
esp_err_t statistics_start_session(void);

/**
 * @brief Finaliza la sesión actual
 * @details Actualiza el tiempo total de operación y guarda las estadísticas
 * @return ESP_OK si la operación fue exitosa
 */
esp_err_t statistics_end_session(void);

/**
 * @brief Actualiza las estadísticas del SSR
 * @details Debe llamarse cada vez que cambie el estado del SSR
 * @param ssr_active true si el SSR está activo, false si está inactivo
 * @return ESP_OK si la actualización fue exitosa
 */
esp_err_t statistics_update_ssr_state(bool ssr_active);

/**
 * @brief Obtiene las estadísticas actuales
 * @param stats Puntero a la estructura donde se almacenarán las estadísticas
 * @return ESP_OK si la operación fue exitosa
 */
esp_err_t statistics_get_data(statistics_data_t *stats);

/**
 * @brief Obtiene las estadísticas formateadas como cadenas de texto
 * @param formatted Puntero a la estructura donde se almacenarán las estadísticas formateadas
 * @return ESP_OK si la operación fue exitosa
 */
esp_err_t statistics_get_formatted(statistics_formatted_t *formatted);

/**
 * @brief Guarda las estadísticas en NVS
 * @return ESP_OK si la operación fue exitosa
 */
esp_err_t statistics_save_to_nvs(void);

/**
 * @brief Carga las estadísticas desde NVS
 * @return ESP_OK si la operación fue exitosa
 */
esp_err_t statistics_load_from_nvs(void);

/**
 * @brief Resetea todas las estadísticas a cero
 * @return ESP_OK si la operación fue exitosa
 */
esp_err_t statistics_reset(void);

/**
 * @brief Actualiza periódicamente las estadísticas
 * @details Esta función debe llamarse periódicamente para mantener actualizados los tiempos
 */
void statistics_periodic_update(void);

#ifdef __cplusplus
}
#endif

#endif // STATISTICS_H 