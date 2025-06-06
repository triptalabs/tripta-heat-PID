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
// Parámetros predeterminados y configuración

static float kp_default = 1.0f;
static float ki_default = 0.1f;
static float kd_default = 2.0f;

#define PID_OUTPUT_MAX 100.0f
#define PID_OUTPUT_MIN 0.0f
#define SAMPLE_TIME_MS 5000

typedef struct {
    float kp, ki, kd;
    float setpoint;
    float integral;
    float previous_error;
    float output;
    bool enabled;
} PIDController;

static PIDController pid = {
    .kp = 1.5f,
    .ki = 0.03f,
    .kd = 25.0f,
    .setpoint = 0.0f,
    .integral = 0.0f,
    .previous_error = 0.0f,
    .output = 0.0f,
    .enabled = false
};

static bool ssr_activo = false;

// Variables auxiliares de control de estabilidad
static float last_temp = 0.0f;
static uint8_t stable_cycle_count = 0;
#define WATCHDOG_RISE 2.0f
#define STABLE_THRESHOLD 0.5f
#define STABLE_CYCLES_FOR_RESET 3

// ───────────────────────────────────────────────────────
// Control del relé SSR

/**
 * @brief Activa la salida digital DO1 (SSR) mediante el CH422G.
 */
void activar_ssr(void) {
    CH422G_EnsurePushPullMode();
    CH422G_od_output(0x00);
    ssr_activo = true;
}

/**
 * @brief Desactiva la salida digital DO1 (SSR).
 */
void desactivar_ssr(void) {
    CH422G_EnsurePushPullMode();
    CH422G_od_output(0x02);
    ssr_activo = false;
}

/**
 * @brief Verifica si el SSR está activo.
 * @return true si está encendido, false si está apagado.
 */
bool pid_ssr_activo(void) {
    return ssr_activo;
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
    float dt = SAMPLE_TIME_MS / 1000.0f;
    float error = pid.setpoint - current_temp;
    pid.integral += error * dt;
    float derivative = (error - pid.previous_error) / dt;

    float output = pid.kp * error + pid.ki * pid.integral + pid.kd * derivative;

    if (output > PID_OUTPUT_MAX) {
        output = PID_OUTPUT_MAX;
        pid.integral -= error * dt;
    } else if (output < PID_OUTPUT_MIN) {
        output = PID_OUTPUT_MIN;
        pid.integral -= error * dt;
    }

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
    const TickType_t xDelay = pdMS_TO_TICKS(SAMPLE_TIME_MS);

    while (1) {
        float current_temp = read_ema_temp();
        last_temp = current_temp;

        if (pid.enabled) {
            float error = pid.setpoint - current_temp;

            if (error < -0.5f) {
                desactivar_ssr();
                printf("[PID] 🧊 Sobrepasó el setpoint +0.5°C → SSR apagado\n");
                vTaskDelay(xDelay);
                continue;
            }

            float control = pid_compute(current_temp);
            uint32_t on_time_ms = (uint32_t)((control / 100.0f) * SAMPLE_TIME_MS);
            uint32_t off_time_ms = SAMPLE_TIME_MS - on_time_ms;

            if (on_time_ms > 0) {
                printf("[PID] 🔌 Encendiendo SSR por %lu ms (Control %.2f%%)\n", (unsigned long)on_time_ms, control);
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

    const float hysteresis = 0.5f;
    const float relay_high = 100.0f;
    const float relay_low  = 0.0f;
    const float d = (relay_high - relay_low) / 2.0f;

    float setpoint = 50.0f;

    uint8_t cycleCount = 0;
    const uint8_t minCycles = 5;
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

        vTaskDelay(pdMS_TO_TICKS(100));
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
        pid.kp = kp_default;
        pid.ki = ki_default;
        pid.kd = kd_default;
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
void pid_enable(void) {
    pid.enabled = true;
}

/**
 * @brief Desactiva el controlador PID y apaga el SSR.
 */
void pid_disable(void) {
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
