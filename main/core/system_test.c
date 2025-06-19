/**
 * @file system_test.c
 * @brief Implementación del módulo de testing del sistema
 * @details Este módulo ejecuta pruebas automáticas del sensor de temperatura y SSR,
 *          proporcionando resultados formateados para mostrar en la interfaz de usuario.
 * @author TriptaLabs
 * @version 1.0
 * @date 2024
 */

#include "system_test.h"
#include "sensor.h"
#include "pid_controller.h"
#include "CH422G.h"
#include "DEV_Config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/**
 * @brief Tag para los logs de este módulo
 */
static const char *TAG = "SYSTEM_TEST";

/**
 * @brief Rango mínimo de temperatura válida para el test (°C)
 */
#define TEMP_MIN_VALID 5.0f

/**
 * @brief Rango máximo de temperatura válida para el test (°C)
 */
#define TEMP_MAX_VALID 200.0f

/**
 * @brief Tiempo de activación del SSR durante el test (ms)
 */
#define SSR_TEST_ACTIVATION_TIME_MS 1000

/**
 * @brief Tiempo de espera entre operaciones del SSR (ms)
 */
#define SSR_TEST_DELAY_MS 500

/**
 * @brief Prueba la funcionalidad del sensor de temperatura
 * @param temperature Puntero donde se almacenará la temperatura leída
 * @return true si el test pasó, false en caso contrario
 */
bool test_temperature_sensor(float *temperature)
{
    ESP_LOGI(TAG, "Iniciando test del sensor de temperatura...");
    
    // Intentar leer la temperatura del sensor
    float temp_reading = read_temperature_raw();
    
    // Verificar si la lectura fue exitosa
    if (temp_reading == -1.0f) {
        ESP_LOGE(TAG, "Error: No se pudo leer del sensor de temperatura");
        if (temperature) *temperature = -1.0f;
        return false;
    }
    
    // Verificar si la temperatura está en un rango válido
    if (temp_reading < TEMP_MIN_VALID || temp_reading > TEMP_MAX_VALID) {
        ESP_LOGW(TAG, "Advertencia: Temperatura fuera de rango válido: %.2f°C", temp_reading);
        if (temperature) *temperature = temp_reading;
        return false;
    }
    
    // Test exitoso
    if (temperature) *temperature = temp_reading;
    ESP_LOGI(TAG, "Test del sensor exitoso: %.2f°C", temp_reading);
    return true;
}

/**
 * @brief Prueba la funcionalidad del SSR (Solid State Relay)
 * @return true si el test pasó, false en caso contrario
 */
bool test_ssr_functionality(void)
{
    ESP_LOGI(TAG, "Iniciando test del SSR...");
    
    // Verificar estado inicial del SSR
    bool initial_state = pid_ssr_status();
    ESP_LOGI(TAG, "Estado inicial del SSR: %s", initial_state ? "ON" : "OFF");
    
    // Asegurar que el SSR esté desactivado al inicio
    desactivar_ssr();
    vTaskDelay(pdMS_TO_TICKS(SSR_TEST_DELAY_MS));
    
    // Verificar que se desactivó
    if (pid_ssr_status()) {
        ESP_LOGE(TAG, "Error: No se pudo desactivar el SSR");
        return false;
    }
    ESP_LOGI(TAG, "SSR desactivado correctamente");
    
    // Activar el SSR brevemente
    activar_ssr();
    vTaskDelay(pdMS_TO_TICKS(SSR_TEST_ACTIVATION_TIME_MS));
    
    // Verificar que se activó
    if (!pid_ssr_status()) {
        ESP_LOGE(TAG, "Error: No se pudo activar el SSR");
        desactivar_ssr(); // Asegurar estado seguro
        return false;
    }
    ESP_LOGI(TAG, "SSR activado correctamente");
    
    // Desactivar el SSR nuevamente
    desactivar_ssr();
    vTaskDelay(pdMS_TO_TICKS(SSR_TEST_DELAY_MS));
    
    // Verificar que se desactivó
    if (pid_ssr_status()) {
        ESP_LOGE(TAG, "Error: No se pudo desactivar el SSR después del test");
        return false;
    }
    
    ESP_LOGI(TAG, "Test del SSR completado exitosamente");
    return true;
}

/**
 * @brief Formatea los resultados de las pruebas para mostrar en la UI
 * @param result Estructura con los resultados de las pruebas
 * @param output Buffer donde se almacenará el resultado formateado
 * @param max_len Longitud máxima del buffer de salida
 * @return ESP_OK si el formateo fue exitoso
 */
esp_err_t format_test_results(const system_test_result_t *result, char *output, size_t max_len)
{
    if (!result || !output || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Buffer temporal para construcción del mensaje
    char temp_buffer[SYSTEM_TEST_RESULT_MAX_LEN];
    int written = 0;
    
    // Título del test
    written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                       "=== TEST DEL SISTEMA ===\n\n");
    
    // Resultado del sensor
    if (result->sensor_test_passed) {
        written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                           "✅ SENSOR: OK - %.1f°C\n", result->sensor_temperature);
    } else {
        if (result->sensor_temperature == -1.0f) {
            written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                               "❌ SENSOR: ERROR - Sin comunicación\n");
        } else {
            written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                               "⚠️ SENSOR: ADVERTENCIA - %.1f°C\n   (Fuera de rango válido)\n", 
                               result->sensor_temperature);
        }
    }
    
    // Resultado del SSR
    if (result->ssr_test_passed) {
        written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                           "✅ SSR: OK - Control operativo\n");
    } else {
        written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                           "❌ SSR: ERROR - Fallo en control\n");
    }
    
    // Estado general del sistema
    written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written, "\n");
    if (result->system_overall_status) {
        written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                           "🎯 SISTEMA: Funcionando correctamente\n");
    } else {
        written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                           "⚠️ SISTEMA: Requiere atención\n");
    }
    
    // Agregar información adicional
    written += snprintf(temp_buffer + written, sizeof(temp_buffer) - written,
                       "\nPrueba ejecutada correctamente.\nRevise los resultados arriba.");
    
    // Copiar al buffer de salida
    strncpy(output, temp_buffer, max_len - 1);
    output[max_len - 1] = '\0'; // Asegurar terminación nula
    
    return ESP_OK;
}

/**
 * @brief Ejecuta todas las pruebas del sistema
 * @param result Puntero a la estructura donde se almacenarán los resultados
 * @return ESP_OK si las pruebas se ejecutaron correctamente, error en caso contrario
 */
esp_err_t system_test_run(system_test_result_t *result)
{
    if (!result) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "=== INICIANDO TEST COMPLETO DEL SISTEMA ===");
    
    // Inicializar estructura de resultados
    memset(result, 0, sizeof(system_test_result_t));
    
    // Test 1: Sensor de temperatura
    ESP_LOGI(TAG, "Ejecutando test del sensor...");
    result->sensor_test_passed = test_temperature_sensor(&result->sensor_temperature);
    
    // Pequeño delay entre tests
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Test 2: SSR
    ESP_LOGI(TAG, "Ejecutando test del SSR...");
    result->ssr_test_passed = test_ssr_functionality();
    
    // Determinar estado general del sistema
    result->system_overall_status = result->sensor_test_passed && result->ssr_test_passed;
    
    // Formatear resultados para la UI
    esp_err_t format_result = format_test_results(result, result->formatted_result, 
                                                 sizeof(result->formatted_result));
    
    if (format_result != ESP_OK) {
        ESP_LOGE(TAG, "Error al formatear resultados del test");
        snprintf(result->formatted_result, sizeof(result->formatted_result),
                "Error interno del test.\nIntente nuevamente.");
        return format_result;
    }
    
    ESP_LOGI(TAG, "=== TEST COMPLETO FINALIZADO ===");
    ESP_LOGI(TAG, "Sensor: %s, SSR: %s, Sistema: %s",
             result->sensor_test_passed ? "PASS" : "FAIL",
             result->ssr_test_passed ? "PASS" : "FAIL", 
             result->system_overall_status ? "OK" : "ERROR");
    
    return ESP_OK;
}

/**
 * @brief Ejecuta una prueba rápida del sistema y retorna el resultado formateado
 * @param result_str Buffer donde se almacenará el resultado formateado
 * @param max_len Longitud máxima del buffer
 * @return ESP_OK si la prueba fue exitosa, error en caso contrario
 */
esp_err_t system_test_run_quick(char *result_str, size_t max_len)
{
    if (!result_str || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    system_test_result_t test_result;
    esp_err_t ret = system_test_run(&test_result);
    
    if (ret == ESP_OK) {
        strncpy(result_str, test_result.formatted_result, max_len - 1);
        result_str[max_len - 1] = '\0';
    } else {
        snprintf(result_str, max_len, 
                "Error ejecutando test del sistema.\nCódigo de error: %s", 
                esp_err_to_name(ret));
    }
    
    return ret;
} 