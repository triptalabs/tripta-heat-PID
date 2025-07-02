#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Métodos de autotuning disponibles
 */
typedef enum {
    AUTOTUNE_METHOD_ZN,   ///< Método Ziegler–Nichols
    AUTOTUNE_METHOD_AH    ///< Método Åström–Hägglund (relay feedback)
} autotune_method_t;

/**
 * @brief Configuración para el proceso de autotuning
 */
typedef struct {
    autotune_method_t method;   ///< Método de autotuning a utilizar
    float setpoint;             ///< Setpoint de temperatura objetivo (°C)
    int   max_duration_ms;      ///< Tiempo máximo de autotuning (ms)
} autotune_config_t;

/**
 * @brief Inicializa el módulo de autotuning con la configuración indicada.
 */
esp_err_t autotuning_init(const autotune_config_t *config);

/**
 * @brief Inicia el proceso de autotuning.
 */
esp_err_t autotuning_start(void);

/**
 * @brief Indica si hay un proceso de autotuning corriendo.
 */
bool autotuning_is_running(void);

/**
 * @brief Cancela el autotuning si está en ejecución.
 */
esp_err_t autotuning_cancel(void);

/**
 * @brief Obtiene los parámetros PID calculados.
 */
esp_err_t autotuning_get_pid(float *kp, float *ki, float *kd);

#ifdef __cplusplus
}
#endif 