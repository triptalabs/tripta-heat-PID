/**
 * @file sensor.h
 * @brief Interfaz para lectura de temperatura vía Modbus RTU en el horno de vacío.
 * 
 * Este módulo proporciona funciones para inicializar la comunicación UART en modo RS485,
 * lanzar la tarea de lectura periódica de temperatura, y obtener valores en crudo
 * o filtrados por EMA (Media Exponencial Móvil).
 *
 * @version 1.0
 * @date 2024-01-27
 */

#ifndef SENSOR_H
#define SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Inicializa el UART y lanza la tarea FreeRTOS de lectura de temperatura.
 *
 * Esta función configura UART1 en modo RS485 half-duplex y crea una tarea
 * que lee periódicamente la temperatura del sensor PT100 (vía Modbus RTU),
 * aplicando filtro EMA y actualizando la gráfica y el estado en la interfaz LVGL.
 */
void start_temperature_task(void);

/**
 * @brief Realiza una lectura directa de la temperatura sin aplicar ningún filtro.
 *
 * Envía una consulta Modbus RTU al esclavo definido y retorna la temperatura cruda
 * en grados Celsius.
 *
 * @return float Temperatura leída del sensor, o -1 si hubo error de comunicación.
 */
float read_temperature_raw(void);

/**
 * @brief Retorna la última temperatura medida con filtro EMA aplicado.
 *
 * @return float Temperatura suavizada (exponencialmente) en grados Celsius.
 */
float read_ema_temp(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_H
