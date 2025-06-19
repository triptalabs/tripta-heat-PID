/**
 * @file system_test.h
 * @brief Módulo de testing del sistema para verificar funcionalidad del sensor y SSR
 * @details Este módulo proporciona funciones para ejecutar pruebas automáticas del sistema,
 *          incluyendo verificación del sensor de temperatura y control del SSR.
 *          Los resultados se formatean para mostrar en la interfaz de usuario.
 * @author TriptaLabs
 * @version 1.0
 * @date 2024
 */

#ifndef SYSTEM_TEST_H
#define SYSTEM_TEST_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Longitud máxima del string de resultados del test
 */
#define SYSTEM_TEST_RESULT_MAX_LEN 512

/**
 * @brief Estructura que contiene los resultados de las pruebas del sistema
 */
typedef struct {
    bool sensor_test_passed;        /**< Resultado del test del sensor */
    bool ssr_test_passed;          /**< Resultado del test del SSR */
    float sensor_temperature;      /**< Temperatura leída durante el test */
    bool system_overall_status;    /**< Estado general del sistema */
    char formatted_result[SYSTEM_TEST_RESULT_MAX_LEN]; /**< Resultado formateado para UI */
} system_test_result_t;

/**
 * @brief Ejecuta todas las pruebas del sistema
 * @details Realiza pruebas secuenciales del sensor de temperatura y del SSR,
 *          formatea los resultados y los almacena en la estructura de resultados
 * @param result Puntero a la estructura donde se almacenarán los resultados
 * @return ESP_OK si las pruebas se ejecutaron correctamente, error en caso contrario
 */
esp_err_t system_test_run(system_test_result_t *result);

/**
 * @brief Ejecuta una prueba rápida del sistema y retorna el resultado formateado
 * @details Función simplificada que ejecuta las pruebas y retorna directamente
 *          el string formateado para mostrar en la UI
 * @param result_str Buffer donde se almacenará el resultado formateado
 * @param max_len Longitud máxima del buffer
 * @return ESP_OK si la prueba fue exitosa, error en caso contrario
 */
esp_err_t system_test_run_quick(char *result_str, size_t max_len);

/**
 * @brief Prueba la funcionalidad del sensor de temperatura
 * @details Verifica que el sensor responda correctamente y que la temperatura
 *          leída esté en un rango razonable
 * @param temperature Puntero donde se almacenará la temperatura leída
 * @return true si el test pasó, false en caso contrario
 */
bool test_temperature_sensor(float *temperature);

/**
 * @brief Prueba la funcionalidad del SSR (Solid State Relay)
 * @details Activa y desactiva el SSR brevemente para verificar que responde
 *          correctamente a los comandos de control
 * @return true si el test pasó, false en caso contrario
 */
bool test_ssr_functionality(void);

/**
 * @brief Formatea los resultados de las pruebas para mostrar en la UI
 * @details Genera un string formateado con los resultados de todas las pruebas
 *          usando iconos y formato legible para el usuario
 * @param result Estructura con los resultados de las pruebas
 * @param output Buffer donde se almacenará el resultado formateado
 * @param max_len Longitud máxima del buffer de salida
 * @return ESP_OK si el formateo fue exitoso
 */
esp_err_t format_test_results(const system_test_result_t *result, char *output, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_TEST_H 