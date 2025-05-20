/**
 * @file pid_controller.h
 * @brief Interfaz del controlador PID para regulación de temperatura en el horno de vacío.
 *
 * Este módulo proporciona las funciones necesarias para configurar, iniciar,
 * activar/desactivar y ajustar los parámetros del controlador PID. También permite
 * guardar y recuperar los parámetros desde la NVS del ESP32.
 *
 * @version 1.0
 * @date 2024-01-27
 */

#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el controlador PID con un setpoint inicial y crea la tarea PID.
 *
 * Si existen parámetros almacenados en NVS, se cargan automáticamente.
 *
 * @param setpoint Temperatura objetivo (°C).
 */
void pid_controller_init(float setpoint);

/**
 * @brief Activa el funcionamiento del PID.
 */
void pid_enable(void);

/**
 * @brief Desactiva el funcionamiento del PID y apaga el SSR.
 */
void pid_disable(void);

/**
 * @brief Asigna nuevos valores a los parámetros PID (Kp, Ki, Kd) y los guarda en NVS.
 *
 * @param new_kp Nuevo valor de la constante proporcional Kp.
 * @param new_ki Nuevo valor de la constante integral Ki.
 * @param new_kd Nuevo valor de la constante derivativa Kd.
 */
void pid_set_params(float new_kp, float new_ki, float new_kd);

/**
 * @brief Establece un nuevo setpoint de temperatura para el PID.
 *
 * @param sp Nuevo setpoint en grados Celsius (°C).
 */
void pid_set_setpoint(float sp);

/**
 * @brief Verifica si el relé SSR está actualmente activo.
 *
 * @return true si el SSR está encendido, false si está apagado.
 */
bool pid_ssr_activo(void);

/**
 * @brief Guarda los parámetros PID actuales en la NVS (almacenamiento no volátil).
 *
 * @return esp_err_t ESP_OK si fue exitoso, o un código de error de NVS.
 */
esp_err_t pid_save_params(void);

/**
 * @brief Carga los parámetros PID desde la NVS.
 *
 * @return esp_err_t ESP_OK si fue exitoso, o un código de error de NVS.
 */
esp_err_t pid_load_params(void);

#ifdef __cplusplus
}
#endif

#endif // PID_CONTROLLER_H
