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
#include "statistics.h"

// ───────────────────────────────────────────────────────
// Estructura de configuración

/**
 * @struct PIDConfig_t
 * @brief Estructura que contiene la configuración del controlador PID
 * 
 * @details Esta estructura almacena todos los parámetros de configuración necesarios
 * para el funcionamiento del controlador PID, incluyendo:
 * - Límites de salida (output_max, output_min)
 * - Tiempo de muestreo (sample_time_ms)
 * - Parámetros de estabilidad (watchdog_rise, stable_threshold, stable_cycles_for_reset)
 * - Parámetros de autotuning (autotune_hysteresis, autotune_relay_high, autotune_relay_low,
 *   autotune_min_cycles, autotune_delay_ms)
 * - Parámetros PID predeterminados (kp_default, ki_default, kd_default)
 */
typedef struct {
    float output_max;           ///< Límite máximo de salida del controlador (porcentaje)
    float output_min;           ///< Límite mínimo de salida del controlador (porcentaje)
    uint32_t sample_time_ms;    ///< Tiempo entre muestras del controlador en milisegundos
    float watchdog_rise;        ///< Umbral de subida para el watchdog en grados
    float stable_threshold;     ///< Umbral de estabilidad en grados
    uint8_t stable_cycles_for_reset;  ///< Número de ciclos estables necesarios para resetear
    float autotune_hysteresis;  ///< Histéresis para el autotuning en grados
    float autotune_relay_high;  ///< Valor alto del relé durante autotuning (porcentaje)
    float autotune_relay_low;   ///< Valor bajo del relé durante autotuning (porcentaje)
    uint8_t autotune_min_cycles;///< Número mínimo de ciclos para el autotuning
    uint32_t autotune_delay_ms; ///< Retardo entre ciclos de autotuning en milisegundos
    float kp_default;           ///< Ganancia proporcional por defecto
    float ki_default;           ///< Ganancia integral por defecto
    float kd_default;           ///< Ganancia derivativa por defecto
} PIDConfig_t;

/**
 * @brief Configuración global del controlador PID
 * 
 * @details Define los parámetros por defecto para el control PID del horno:
 * - Límites de salida (0-100%)
 * - Tiempo de muestreo (5 segundos)
 * - Umbrales de estabilidad y watchdog
 * - Parámetros de autotuning
 * - Valores PID predeterminados
 * 
 * @note Esta configuración se utiliza como base para el control de temperatura
 * del horno y puede ser modificada según las necesidades específicas.
 */
static const PIDConfig_t pid_config = {
    .output_max = 100.0f,           // Límite máximo de salida (100%)
    .output_min = 0.0f,             // Límite mínimo de salida (0%)
    .sample_time_ms = 5000,         // Tiempo entre muestras (5 segundos)
    .watchdog_rise = 2.0f,          // Umbral de subida para watchdog (2°C)
    .stable_threshold = 0.5f,       // Umbral de estabilidad (0.5°C)
    .stable_cycles_for_reset = 3,   // Ciclos estables para resetear
    .autotune_hysteresis = 0.5f,    // Histéresis para autotuning (0.5°C)
    .autotune_relay_high = 100.0f,  // Valor alto del relé (100%)
    .autotune_relay_low = 0.0f,     // Valor bajo del relé (0%)
    .autotune_min_cycles = 5,       // Mínimo de ciclos para autotuning
    .autotune_delay_ms = 100,       // Retardo entre ciclos (100ms)
    .kp_default = 1.0f,             // Ganancia proporcional por defecto
    .ki_default = 0.1f,             // Ganancia integral por defecto
    .kd_default = 2.0f              // Ganancia derivativa por defecto
};


/**
 * @struct PIDController
 * @brief Estructura que implementa un controlador PID para el horno
 * 
 * @details Esta estructura mantiene el estado interno del controlador PID:
 * - Constantes de control (kp, ki, kd)
 * - Punto de consigna (setpoint)
 * - Término integral acumulado
 * - Error anterior para cálculo derivativo
 * - Salida del controlador
 * - Estado de habilitación
 * - Estado del SSR (Solid State Relay)
 */
typedef struct {
    float kp, ki, kd;           // Constantes PID
    float setpoint;             // Temperatura objetivo
    float integral;             // Término integral acumulado
    float previous_error;       // Error anterior para derivativo
    float output;               // Salida del controlador (0-100%)
    bool enabled;               // Estado de habilitación
    bool ssr_status;            // Estado del SSR
} PIDController;

/**
 * @brief Instancia global del controlador PID
 * 
 * @details Inicializa el controlador PID con valores predeterminados:
 * - kp: 1.5 (ganancia proporcional)
 * - ki: 0.03 (ganancia integral)
 * - kd: 25.0 (ganancia derivativa)
 * - setpoint: 0.0 (temperatura objetivo inicial)
 * - integral: 0.0 (término integral inicial)
 * - previous_error: 0.0 (error anterior inicial)
 * - output: 0.0 (salida inicial)
 * - enabled: false (controlador deshabilitado inicialmente)
 * - ssr_status: false (SSR desactivado inicialmente)
 */
static PIDController pid = {
    .kp = 1.5f,              // Ganancia proporcional inicial
    .ki = 0.03f,             // Ganancia integral inicial
    .kd = 25.0f,             // Ganancia derivativa inicial
    .setpoint = 0.0f,        // Temperatura objetivo inicial
    .integral = 0.0f,        // Término integral inicial
    .previous_error = 0.0f,  // Error anterior inicial
    .output = 0.0f,          // Salida del controlador inicial
    .enabled = false,        // Controlador deshabilitado inicialmente
    .ssr_status = false      // SSR desactivado inicialmente
};

// Variables de estado
static float last_temp = 0.0f;

// ───────────────────────────────────────────────────────
// Control del relé SSR

/**
 * @brief Activa la salida digital DO1 (SSR) mediante el CH422G.
 */
void activar_ssr(void) {
    CH422G_EnsurePushPullMode();
    CH422G_od_output(0x00);
    pid.ssr_status = true;
    
    // Notificar al módulo de estadísticas el cambio de estado
    statistics_update_ssr_state(true);
}

/**
 * @brief Desactiva la salida digital DO1 (SSR).
 */
void desactivar_ssr(void) {
    CH422G_EnsurePushPullMode();
    CH422G_od_output(0x02);
    pid.ssr_status = false;
    
    // Notificar al módulo de estadísticas el cambio de estado
    statistics_update_ssr_state(false);
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
 * 
 * @note Comentado para evitar warning de función no utilizada
 */
#if 0  // Función no utilizada - comentada para evitar warnings
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
#endif  // Fin de función comentada

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
