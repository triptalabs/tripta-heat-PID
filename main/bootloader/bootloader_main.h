/**
 * @file bootloader_main.h
 * @brief Header principal del bootloader personalizado para TriptaLabs Heat Controller
 *
 * Este archivo expone las APIs públicas del bootloader personalizado que
 * deben ser utilizadas por la aplicación principal.
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#ifndef BOOTLOADER_MAIN_H
#define BOOTLOADER_MAIN_H

#include "bootloader_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================
 * FUNCIONES PÚBLICAS PRINCIPALES
 * ================================ */

/**
 * @brief Inicializa el bootloader personalizado
 *
 * Debe ser llamado una vez al inicio de app_main() antes de cualquier
 * otra operación del bootloader.
 *
 * @return
 *      - ESP_OK: Inicialización exitosa
 *      - ESP_FAIL: Error en la inicialización
 */
esp_err_t bootloader_init(void);

/**
 * @brief Función principal de decisión del bootloader
 *
 * Esta es la función clave que debe ser llamada al inicio de app_main()
 * después de bootloader_init(). Verifica integridad y decide si continuar
 * con boot normal o activar recovery.
 *
 * IMPORTANTE: Esta función puede NO RETORNAR si se requiere recovery
 * exitoso (el sistema se reinicia automáticamente).
 *
 * @return
 *      - ESP_OK: App puede continuar con boot normal
 *      - ESP_FAIL: Recovery falló, sistema en estado crítico
 *      - NO RETORNA: Sistema reinicia tras recovery exitoso
 */
esp_err_t bootloader_check_and_decide(void);

/**
 * @brief Marca el boot actual como exitoso
 *
 * Debe ser llamado por la aplicación una vez que haya iniciado correctamente
 * y todos los sistemas estén funcionando. Esto resetea contadores de fallos
 * y confirma que el firmware actual es estable.
 *
 * Sugerencia: Llamar después de:
 * - Inicialización completa de todos los módulos
 * - Conexión WiFi exitosa (si aplica)
 * - Verificación de funcionamiento de hardware crítico
 */
void bootloader_mark_boot_successful(void);

/**
 * @brief Obtiene estadísticas actuales del bootloader
 *
 * @param[out] stats Estructura para copiar estadísticas
 * @return ESP_OK si exitoso, ESP_ERR_INVALID_ARG si parámetros inválidos
 */
esp_err_t bootloader_get_stats(bootloader_stats_t* stats);

/**
 * @brief Fuerza un recovery desde SD
 *
 * Útil para testing o recovery manual solicitado por el usuario.
 * ADVERTENCIA: Esta función puede NO RETORNAR si recovery es exitoso.
 *
 * @return
 *      - ESP_OK: Recovery forzado exitoso (sistema reinicia)
 *      - ESP_FAIL: Recovery forzado falló
 */
esp_err_t bootloader_force_recovery(void);

/**
 * @brief Limpia todos los datos del bootloader (factory reset)
 *
 * Resetea estadísticas y datos de integridad. Útil para debugging
 * o reset completo del sistema.
 *
 * @return ESP_OK si exitoso
 */
esp_err_t bootloader_factory_reset(void);

/* ================================
 * MACROS DE CONVENIENCIA
 * ================================ */

/**
 * @brief Inicialización completa del bootloader en app_main()
 *
 * Macro que combina inicialización y verificación en una sola llamada.
 * Uso típico al inicio de app_main():
 *
 * @code
 * void app_main(void) {
 *     BOOTLOADER_INIT_AND_CHECK();
 *     
 *     // Resto de la inicialización de la app...
 *     init_hardware();
 *     init_wifi();
 *     // etc...
 *     
 *     // Marcar boot como exitoso al final
 *     bootloader_mark_boot_successful();
 * }
 * @endcode
 */
#define BOOTLOADER_INIT_AND_CHECK() do { \
    ESP_ERROR_CHECK(bootloader_init()); \
    esp_err_t boot_check_result = bootloader_check_and_decide(); \
    if (boot_check_result != ESP_OK) { \
        ESP_LOGE("APP", "Bootloader en estado crítico - abortando"); \
        abort(); \
    } \
} while(0)

/**
 * @brief Verifica si es primer boot del sistema
 *
 * @return true si es primer boot, false si no
 */
#define BOOTLOADER_IS_FIRST_BOOT() ({ \
    bootloader_stats_t stats; \
    (bootloader_get_stats(&stats) == ESP_OK) ? stats.first_boot : false; \
})

/* ================================
 * FUNCIONES DE TESTING Y DEBUG
 * ================================ */

/**
 * @brief Ejecuta una prueba rápida del sistema de bootloader
 *
 * Útil para verificar que todos los módulos funcionan correctamente.
 * No modifica el estado del sistema.
 *
 * @return ESP_OK si todos los tests pasan
 */
esp_err_t bootloader_run_self_test(void);

/**
 * @brief Simula una corrupción de firmware para testing
 *
 * PELIGROSO: Solo para testing. Corrompe el hash almacenado
 * para forzar un recovery en el próximo boot.
 *
 * @return ESP_OK si se ejecutó la simulación
 */
esp_err_t bootloader_simulate_corruption(void);

/**
 * @brief Muestra información detallada del bootloader por UART
 *
 * @return ESP_OK siempre
 */
esp_err_t bootloader_print_detailed_info(void);

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_MAIN_H 