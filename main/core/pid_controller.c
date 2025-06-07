/**
 * @file pid_controller.c
 * @brief Implementación del controlador PID con soporte para autotuning, control de SSR y almacenamiento en NVS.
 * 
 * Este módulo controla la temperatura en el horno de vacío utilizando un algoritmo PID clásico.
 * Integra control de salidas mediante CH422G, lectura de temperatura filtrada, y funciones para
 * guardar y cargar parámetros de control desde la memoria NVS del ESP32.
 * 
 * @version 1.0
 * @date 2024-01-27
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "CH422G.h"
#include "DEV_Config.h"
#include "ui_events.h"
#include "pid_controller.h"

// ───────────────────────────────────────────────────────
// Estructura de configuración

typedef struct {
    // Límites de salida PID
    float output_max;
    float output_min;
    
    // Tiempo de muestreo
    uint32_t sample_time_ms;
    
    // Parámetros de estabilidad
    float watchdog_rise;
    float stable_threshold;
    uint8_t stable_cycles_for_reset;
    
    // Parámetros de autotuning
    float autotune_hysteresis;
    float autotune_relay_high;
    float autotune_relay_low;
    uint8_t autotune_min_cycles;
    uint32_t autotune_delay_ms;
    
    // Parámetros PID predeterminados
    float kp_default;
    float ki_default;
    float kd_default;
} PIDConfig_t;

// Configuración global
static const PIDConfig_t pid_config = {
    .output_max = 100.0f,
    .output_min = 0.0f,
    .sample_time_ms = 5000,
    .watchdog_rise = 2.0f,
    .stable_threshold = 0.5f,
    .stable_cycles_for_reset = 3,
    .autotune_hysteresis = 0.5f,
    .autotune_relay_high = 100.0f,
    .autotune_relay_low = 0.0f,
    .autotune_min_cycles = 5,
    .autotune_delay_ms = 100,
    .kp_default = 1.0f,
    .ki_default = 0.1f,
    .kd_default = 2.0f
};

// ───────────────────────────────────────────────────────
// Estructura del controlador PID

typedef struct {
    float kp, ki, kd;
    float setpoint;
    float integral;
    float previous_error;
    float output;
    bool enabled;
    bool ssr_status;    // Estado del SSR
} PIDController;

// Instancia global del controlador PID
static PIDController pid = {
    .kp = 1.5f,
    .ki = 0.03f,
    .kd = 25.0f,
    .setpoint = 0.0f,
    .integral = 0.0f,
    .previous_error = 0.0f,
    .output = 0.0f,
    .enabled = false,
    .ssr_status = false
};

// Variables de estado
static float last_temp = 0.0f;
static uint8_t stable_cycle_count = 0;

// ───────────────────────────────────────────────────────
// Control del relé SSR

/**
 * @brief Activa la salida digital DO1 (SSR) mediante el CH422G.
 */
void activar_ssr(void) {
    CH422G_EnsurePushPullMode();
    CH422G_od_output(0x00);
    pid.ssr_status = true;
}

/**
 * @brief Desactiva la salida digital DO1 (SSR).
 */
void desactivar_ssr(void) {
    CH422G_EnsurePushPullMode();
    CH422G_od_output(0x02);
    pid.ssr_status = false;
}

/**
 * @brief Verifica si el SSR está activo.
 * @return true si está encendido, false si está apagado.
 */
bool pid_ssr_status(void) {
    return pid.ssr_status;
}

// ───────────────────────────────────────────────────────
// PID interno

/**
 * @brief Calcula el valor de control PID.
 * 
 * @param current_temp Temperatura actual.
 * @return float Salida PID normalizada entre 0–100.
 */
static float pid_compute(float current_temp) {
    const float dt = pid_config.sample_time_ms / 1000.0f;
    const float error = pid.setpoint - current_temp;
    
    // Cálculo del término integral con anti-windup
    pid.integral += error * dt;
    
    // Cálculo del término derivativo
    const float derivative = (error - pid.previous_error) / dt;
    
    // Cálculo de la salida PID
    float output = pid.kp * error + 
                  pid.ki * pid.integral + 
                  pid.kd * derivative;
    
    // Anti-windup y limitación de salida
    if (output > pid_config.output_max) {
        output = pid_config.output_max;
        pid.integral -= error * dt;  // Anti-windup
    } else if (output < pid_config.output_min) {
        output = pid_config.output_min;
        pid.integral -= error * dt;  // Anti-windup
    }
    
    // Actualización de estado
    pid.previous_error = error;
    pid.output = output;
    
    return output;
}

/**
 * @brief Tarea principal del PID ejecutada periódicamente.
 * 
 * Controla el relé SSR en base al valor de control calculado por el PID.
 * Incluye lógica de protección por sobretemperatura (0.5°C sobre el setpoint).
 */
static void pid_task(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(pid_config.sample_time_ms);
    const float TEMP_OVERSHOOT_THRESHOLD = 0.5f;

    while (1) {
        // Lectura de temperatura actual
        const float current_temp = read_ema_temp();
        last_temp = current_temp;

        if (pid.enabled) {
            const float error = pid.setpoint - current_temp;

            // Protección contra sobretemperatura
            if (error < -TEMP_OVERSHOOT_THRESHOLD) {
                desactivar_ssr();
                printf("[PID] 🧊 Sobrepasó el setpoint +%.1f°C → SSR apagado\n", TEMP_OVERSHOOT_THRESHOLD);
                vTaskDelay(xDelay);
                continue;
            }

            // Cálculo del control PID
            const float control = pid_compute(current_temp);
            const uint32_t on_time_ms = (uint32_t)((control / 100.0f) * pid_config.sample_time_ms);
            const uint32_t off_time_ms = pid_config.sample_time_ms - on_time_ms;

            // Control del SSR con modulación PWM
            if (on_time_ms > 0) {
                printf("[PID] 🔌 Encendiendo SSR por %lu ms (Control %.2f%%)\n", 
                       (unsigned long)on_time_ms, control);
                activar_ssr();
                vTaskDelay(pdMS_TO_TICKS(on_time_ms));
            }

            if (off_time_ms > 0) {
                printf("[PID] ⚡ Apagando SSR por %lu ms\n", (unsigned long)off_time_ms);
                desactivar_ssr();
                vTaskDelay(pdMS_TO_TICKS(off_time_ms));
            }
        } else {
            desactivar_ssr();
            vTaskDelay(xDelay);
        }
    }
}

/**
 * @brief Tarea de autotuning basada en el método de oscilación de Ziegler-Nichols.
 * 
 * Oscila el sistema alrededor de un setpoint fijo y calcula los parámetros óptimos Ku y Pu.
 * Luego, calcula nuevos valores de Kp, Ki y Kd.
 */
static void autotune_task(void *pvParameters) {
    pid.enabled = false;

    const float hysteresis = pid_config.autotune_hysteresis;
    const float relay_high = pid_config.autotune_relay_high;
    const float relay_low = pid_config.autotune_relay_low;
    const float d = (relay_high - relay_low) / 2.0f;

    float setpoint = 50.0f;

    uint8_t cycleCount = 0;
    const uint8_t minCycles = pid_config.autotune_min_cycles;
    float periodSum = 0.0f;
    TickType_t lastOnTick = 0;

    float tempMax = -1000.0f;
    float tempMin =  1000.0f;
    bool relayState = false;

    printf("[Autotune] 🧪 Iniciado con SP=%.2f°C\n", setpoint);

    while (cycleCount < minCycles) {
        float currentTemp = read_ema_temp();

        if (currentTemp > tempMax) tempMax = currentTemp;
        if (currentTemp < tempMin) tempMin = currentTemp;

        if (!relayState && (currentTemp < setpoint - hysteresis)) {
            relayState = true;
            CH422G_EnsurePushPullMode();
            CH422G_od_output(0x00);

            TickType_t now = xTaskGetTickCount();
            if (lastOnTick != 0) {
                float period = (now - lastOnTick) / 1000.0f;
                periodSum += period;
                cycleCount++;
                printf("[Autotune] 🔁 Periodo #%d: %.2fs\n", cycleCount, period);
            }
            lastOnTick = now;
        } else if (relayState && (currentTemp > setpoint + hysteresis)) {
            relayState = false;
            CH422G_EnsurePushPullMode();
            CH422G_od_output(0x02);
        }

        vTaskDelay(pdMS_TO_TICKS(pid_config.autotune_delay_ms));
    }

    CH422G_od_output(0x02);

    float Pu = periodSum / cycleCount;
    float amplitude = (tempMax - tempMin) / 2.0f;
    float Ku = (4.0f * d) / (M_PI * amplitude);

    float new_kp = 0.6f * Ku;
    float new_ki = 1.2f * Ku / Pu;
    float new_kd = 0.075f * Ku * Pu;

    printf("[Autotune] ✅ Finalizado\n");
    printf("Kp = %.4f, Ki = %.4f, Kd = %.4f\n", new_kp, new_ki, new_kd);

    pid_set_params(new_kp, new_ki, new_kd);
    vTaskDelete(NULL);
}

// ───────────────────────────────────────────────────────
// API pública

/**
 * @brief Inicializa el controlador PID y crea la tarea de control.
 * 
 * @param setpoint Temperatura objetivo.
 */
void pid_controller_init(float setpoint) {
    if (pid_load_params() != ESP_OK) {
        pid.kp = pid_config.kp_default;
        pid.ki = pid_config.ki_default;
        pid.kd = pid_config.kd_default;
    }

    pid.setpoint = setpoint;
    pid.integral = 0.0f;
    pid.previous_error = 0.0f;
    pid.output = 0.0f;
    pid.enabled = false;

    xTaskCreate(pid_task, "PID_Task", 4096, NULL, 5, NULL);
    // xTaskCreate(autotune_task, "Autotune_Task", 4096, NULL, 5, NULL);
}

/**
 * @brief Activa el controlador PID.
 */
void enable_pid(void) {
    pid.enabled = true;
}

/**
 * @brief Desactiva el controlador PID y apaga el SSR.
 */
void disable_pid(void) {
    pid.enabled = false;
    desactivar_ssr();
}

/**
 * @brief Asigna nuevos parámetros Kp, Ki y Kd al controlador PID.
 * 
 * @param new_kp Nuevo valor de Kp.
 * @param new_ki Nuevo valor de Ki.
 * @param new_kd Nuevo valor de Kd.
 */
void pid_set_params(float new_kp, float new_ki, float new_kd) {
    pid.kp = new_kp;
    pid.ki = new_ki;
    pid.kd = new_kd;
    pid_save_params();
}

/**
 * @brief Establece un nuevo setpoint para el controlador PID.
 * 
 * @param sp Nuevo setpoint en °C.
 */
void pid_set_setpoint(float sp) {
    pid.setpoint = sp;
}

// ───────────────────────────────────────────────────────
// Manejo de NVS

/**
 * @brief Guarda los parámetros Kp, Ki y Kd en NVS.
 * 
 * @return esp_err_t ESP_OK si fue exitoso.
 */
esp_err_t pid_save_params(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("pid_params", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, "Kp", &pid.kp, sizeof(pid.kp));
    if (err != ESP_OK) { nvs_close(handle); return err; }

    err = nvs_set_blob(handle, "Ki", &pid.ki, sizeof(pid.ki));
    if (err != ESP_OK) { nvs_close(handle); return err; }

    err = nvs_set_blob(handle, "Kd", &pid.kd, sizeof(pid.kd));
    if (err != ESP_OK) { nvs_close(handle); return err; }

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

/**
 * @brief Carga los parámetros Kp, Ki y Kd desde NVS.
 * 
 * @return esp_err_t ESP_OK si fue exitoso.
 */
esp_err_t pid_load_params(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("pid_params", NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    size_t size = sizeof(pid.kp);
    err = nvs_get_blob(handle, "Kp", &pid.kp, &size);
    if (err != ESP_OK) { nvs_close(handle); return err; }

    size = sizeof(pid.ki);
    err = nvs_get_blob(handle, "Ki", &pid.ki, &size);
    if (err != ESP_OK) { nvs_close(handle); return err; }

    size = sizeof(pid.kd);
    err = nvs_get_blob(handle, "Kd", &pid.kd, &size);
    nvs_close(handle);
    return err;
}
