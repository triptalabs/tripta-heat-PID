#include "autotuning.h"
#include "pid_controller.h"
#include "esp_log.h"
#include "ziegler_nichols.h"
#include "astrom_hagglund.h"

static const char *TAG = "AUTOTUNE";

static autotune_config_t g_config;
static bool g_running = false;

esp_err_t autotuning_init(const autotune_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Configuración nula");
        return ESP_ERR_INVALID_ARG;
    }
    g_config = *config;
    g_running = false;
    ESP_LOGI(TAG, "Módulo de autotuning inicializado. Método=%d, SP=%.2f", g_config.method, g_config.setpoint);
    return ESP_OK;
}

esp_err_t autotuning_start(void)
{
    if (g_running) {
        ESP_LOGW(TAG, "Autotuning ya en ejecución");
        return ESP_ERR_INVALID_STATE;
    }
    g_running = true;
    ESP_LOGI(TAG, "Autotuning iniciado (método=%d)", g_config.method);

    // Desactivar el controlador PID para evitar interferencia
    disable_pid();

    esp_err_t res;
    switch (g_config.method) {
        case AUTOTUNE_METHOD_ZN:
            res = ziegler_nichols_start(g_config.setpoint);
            break;
        case AUTOTUNE_METHOD_AH:
            res = astrom_hagglund_start(g_config.setpoint);
            break;
        default:
            ESP_LOGE(TAG, "Método desconocido");
            res = ESP_ERR_INVALID_ARG;
            break;
    }

    if (res != ESP_OK) {
        g_running = false;
        // Si falló el inicio del autotune, se puede volver a habilitar el PID
        enable_pid();
    }
    return res;
}

bool autotuning_is_running(void)
{
    return g_running;
}

esp_err_t autotuning_cancel(void)
{
    if (!g_running) {
        return ESP_ERR_INVALID_STATE;
    }
    g_running = false;
    ESP_LOGW(TAG, "Autotuning cancelado por el usuario");
    return ESP_OK;
}

esp_err_t autotuning_get_pid(float *kp, float *ki, float *kd)
{
    if (!kp || !ki || !kd) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!g_running) {
        ESP_LOGW(TAG, "Autotuning no está en ejecución");
    }

    switch (g_config.method) {
        case AUTOTUNE_METHOD_ZN:
            return ziegler_nichols_get_pid(kp, ki, kd);
        case AUTOTUNE_METHOD_AH:
            return astrom_hagglund_get_pid(kp, ki, kd);
        default:
            return ESP_ERR_INVALID_STATE;
    }
} 