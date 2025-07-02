#include "ziegler_nichols.h"
#include "esp_log.h"
#include "sensor.h"
#include "CH422G.h"
#include "pid_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char *TAG = "ZN_AUTOTUNE";

static TaskHandle_t autotune_task_handle = NULL;

static float last_kp = 0.0f;
static float last_ki = 0.0f;
static float last_kd = 0.0f;
static bool  params_ready = false;

typedef struct {
    float setpoint;
} zn_params_t;

static zn_params_t s_params;

static void ziegler_nichols_task(void *pv)
{
    const float hysteresis    = 0.5f;   // °C
    const float relay_high    = 100.0f; // %
    const float relay_low     = 0.0f;   // %
    const uint8_t min_cycles  = 5;
    const uint32_t delay_ms   = 100;

    const float d = (relay_high - relay_low) / 2.0f;

    uint8_t cycleCount = 0;
    float periodSum    = 0.0f;
    TickType_t lastOnTick = 0;

    float tempMax = -1000.0f;
    float tempMin =  1000.0f;
    bool relayState = false;

    float setpoint = s_params.setpoint;

    ESP_LOGI(TAG, "🧪 Autotune Ziegler-Nichols iniciado (SP=%.2f)", setpoint);

    while (cycleCount < min_cycles) {
        float currentTemp = read_ema_temp();

        if (currentTemp > tempMax) tempMax = currentTemp;
        if (currentTemp < tempMin) tempMin = currentTemp;

        if (!relayState && (currentTemp < setpoint - hysteresis)) {
            relayState = true;
            CH422G_EnsurePushPullMode();
            CH422G_od_output(0x00); // SSR ON

            TickType_t now = xTaskGetTickCount();
            if (lastOnTick != 0) {
                float period = (now - lastOnTick) / 1000.0f; // ticks->s
                periodSum += period;
                cycleCount++;
                ESP_LOGI(TAG, "🔁 Periodo #%d: %.2f s", cycleCount, period);
            }
            lastOnTick = now;
        } else if (relayState && (currentTemp > setpoint + hysteresis)) {
            relayState = false;
            CH422G_EnsurePushPullMode();
            CH422G_od_output(0x02); // SSR OFF
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    // Asegurar SSR apagado
    CH422G_od_output(0x02);

    float Pu = periodSum / cycleCount;
    float amplitude = (tempMax - tempMin) / 2.0f;
    float Ku = (4.0f * d) / (M_PI * amplitude);

    last_kp = 0.6f * Ku;
    last_ki = 1.2f * Ku / Pu;
    last_kd = 0.075f * Ku * Pu;
    params_ready = true;

    ESP_LOGI(TAG, "✅ Autotune finalizado. Kp=%.4f, Ki=%.4f, Kd=%.4f", last_kp, last_ki, last_kd);

    // Aplicar parámetros al PID global y reactivar el controlador
    pid_set_params(last_kp, last_ki, last_kd);
    enable_pid();

    autotune_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t ziegler_nichols_start(float setpoint)
{
    if (autotune_task_handle != NULL) {
        ESP_LOGW(TAG, "Autotune ya está en ejecución");
        return ESP_ERR_INVALID_STATE;
    }

    s_params.setpoint = setpoint;
    params_ready = false;

    BaseType_t res = xTaskCreate(ziegler_nichols_task, "ZN_Autotune", 4096, NULL, 5, &autotune_task_handle);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "No se pudo crear la tarea de autotune");
        autotune_task_handle = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ziegler_nichols_get_pid(float *kp, float *ki, float *kd)
{
    if (!kp || !ki || !kd) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!params_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    *kp = last_kp;
    *ki = last_ki;
    *kd = last_kd;
    return ESP_OK;
} 