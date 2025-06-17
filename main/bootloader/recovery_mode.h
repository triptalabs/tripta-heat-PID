/**
 * @file recovery_mode.h
 * @brief Módulo de modo recovery para bootloader personalizado
 *
 * Este módulo se encarga de:
 * - Inicializar y mostrar mensajes en pantalla LCD
 * - Enviar información detallada por UART
 * - Gestionar interacción con el usuario en modo recovery
 * - Coordinar recovery manual cuando falla el automático
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#ifndef RECOVERY_MODE_H
#define RECOVERY_MODE_H

#include "bootloader_config.h"
#include "../drivers/display/waveshare_rgb_lcd_port.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================
 * FUNCIONES PÚBLICAS
 * ================================ */

/**
 * @brief Inicializa el módulo de modo recovery
 *
 * @return
 *      - ESP_OK: Inicialización exitosa
 *      - ESP_FAIL: Error en la inicialización
 */
esp_err_t recovery_mode_init(void);

/**
 * @brief Inicializa la pantalla para modo recovery
 *
 * Configura la pantalla LCD y LVGL para mostrar mensajes de recovery.
 *
 * @return
 *      - ESP_OK: Pantalla inicializada exitosamente
 *      - ESP_FAIL: Error inicializando pantalla
 */
esp_err_t init_recovery_display(void);

/**
 * @brief Muestra mensaje de recovery en pantalla LCD
 *
 * @param[in] reason Razón del modo recovery
 * @param[in] details Detalles adicionales del error
 * @param[in] progress Progreso actual (0-100) o -1 si no aplica
 * @return
 *      - ESP_OK: Mensaje mostrado exitosamente
 *      - ESP_FAIL: Error mostrando mensaje
 */
esp_err_t show_recovery_message(boot_reason_t reason, const char* details, int progress);

/**
 * @brief Envía mensaje detallado de recovery por UART
 *
 * @param[in] reason Razón del modo recovery
 * @param[in] details Detalles adicionales del error
 * @param[in] technical_info Información técnica detallada
 * @return
 *      - ESP_OK: Mensaje enviado exitosamente
 *      - ESP_FAIL: Error enviando mensaje
 */
esp_err_t show_uart_recovery_message(boot_reason_t reason, const char* details, const char* technical_info);

/**
 * @brief Espera por acción del usuario en modo recovery
 *
 * Espera por input del usuario (botones, UART) para continuar con recovery manual.
 *
 * @param[in] timeout_ms Timeout en milisegundos (0 = sin timeout)
 * @return
 *      - ESP_OK: Usuario confirmó continuar
 *      - ESP_ERR_TIMEOUT: Timeout expirado
 *      - ESP_FAIL: Error durante la espera
 */
esp_err_t wait_for_recovery_action(uint32_t timeout_ms);

/**
 * @brief Entra en modo recovery completo
 *
 * Función principal que coordina todo el modo recovery:
 * 1. Inicializa pantalla
 * 2. Muestra mensajes de error
 * 3. Proporciona opciones al usuario
 * 4. Ejecuta recovery manual si es posible
 *
 * @param[in] reason Razón del modo recovery
 * @param[in] recovery_info Información del estado de recovery
 * @return
 *      - ESP_OK: Recovery manual exitoso
 *      - ESP_FAIL: Recovery manual falló
 */
esp_err_t enter_recovery_mode(boot_reason_t reason, recovery_state_t* recovery_info);

/* ================================
 * FUNCIONES DE PANTALLA
 * ================================ */

/**
 * @brief Muestra pantalla de bienvenida al recovery
 *
 * @return
 *      - ESP_OK: Pantalla mostrada exitosamente
 *      - ESP_FAIL: Error mostrando pantalla
 */
esp_err_t show_recovery_welcome_screen(void);

/**
 * @brief Muestra información del sistema en pantalla
 *
 * @param[in] firmware_info Información del firmware actual
 * @param[in] bootloader_stats Estadísticas del bootloader
 * @return
 *      - ESP_OK: Información mostrada exitosamente
 *      - ESP_FAIL: Error mostrando información
 */
esp_err_t show_system_info_screen(const firmware_info_t* firmware_info, const bootloader_stats_t* bootloader_stats);

/**
 * @brief Muestra progreso de recovery en pantalla
 *
 * @param[in] operation Descripción de la operación actual
 * @param[in] progress Progreso (0-100)
 * @param[in] status Estado actual
 * @return
 *      - ESP_OK: Progreso mostrado exitosamente
 *      - ESP_FAIL: Error mostrando progreso
 */
esp_err_t show_recovery_progress(const char* operation, int progress, const char* status);

/**
 * @brief Muestra mensaje de error crítico
 *
 * @param[in] error_code Código de error
 * @param[in] error_message Mensaje de error
 * @param[in] recovery_possible Indica si recovery es posible
 * @return
 *      - ESP_OK: Error mostrado exitosamente
 *      - ESP_FAIL: Error mostrando mensaje
 */
esp_err_t show_critical_error(int error_code, const char* error_message, bool recovery_possible);

/* ================================
 * FUNCIONES DE COMUNICACIÓN UART
 * ================================ */

/**
 * @brief Inicializa UART para comunicación de recovery
 *
 * @return
 *      - ESP_OK: UART inicializado exitosamente
 *      - ESP_FAIL: Error inicializando UART
 */
esp_err_t init_recovery_uart(void);

/**
 * @brief Envía información completa del sistema por UART
 *
 * @param[in] firmware_info Información del firmware
 * @param[in] bootloader_stats Estadísticas del bootloader
 * @return
 *      - ESP_OK: Información enviada exitosamente
 *      - ESP_FAIL: Error enviando información
 */
esp_err_t send_system_info_uart(const firmware_info_t* firmware_info, const bootloader_stats_t* bootloader_stats);

/**
 * @brief Envía log detallado de recovery por UART
 *
 * @param[in] log_message Mensaje de log
 * @param[in] timestamp Timestamp del evento
 * @return
 *      - ESP_OK: Log enviado exitosamente
 *      - ESP_FAIL: Error enviando log
 */
esp_err_t send_recovery_log_uart(const char* log_message, uint32_t timestamp);

/**
 * @brief Lee comando del usuario desde UART
 *
 * @param[out] command Buffer para almacenar comando
 * @param[in] max_len Tamaño máximo del buffer
 * @param[in] timeout_ms Timeout en milisegundos
 * @return
 *      - ESP_OK: Comando leído exitosamente
 *      - ESP_ERR_TIMEOUT: Timeout expirado
 *      - ESP_FAIL: Error leyendo comando
 */
esp_err_t read_user_command_uart(char* command, size_t max_len, uint32_t timeout_ms);

/* ================================
 * FUNCIONES DE INTERACCIÓN
 * ================================ */

/**
 * @brief Verifica estado de botones físicos
 *
 * @param[out] button_pressed Qué botón fue presionado
 * @return
 *      - ESP_OK: Estado leído exitosamente
 *      - ESP_FAIL: Error leyendo botones
 */
esp_err_t check_physical_buttons(int* button_pressed);

/**
 * @brief Procesa comando de recovery del usuario
 *
 * @param[in] command Comando recibido
 * @param[out] action Acción a ejecutar
 * @return
 *      - ESP_OK: Comando procesado exitosamente
 *      - ESP_ERR_INVALID_ARG: Comando inválido
 *      - ESP_FAIL: Error procesando comando
 */
esp_err_t process_recovery_command(const char* command, int* action);

/**
 * @brief Ejecuta recovery manual paso a paso
 *
 * Permite al usuario ejecutar recovery manualmente con feedback.
 *
 * @return
 *      - ESP_OK: Recovery manual exitoso
 *      - ESP_FAIL: Recovery manual falló
 */
esp_err_t execute_manual_recovery(void);

/* ================================
 * FUNCIONES DE UTILIDAD
 * ================================ */

/**
 * @brief Convierte razón de boot a string descriptivo
 *
 * @param[in] reason Razón de boot
 * @return String descriptivo de la razón
 */
const char* boot_reason_to_string(boot_reason_t reason);

/**
 * @brief Convierte estado de recovery a string descriptivo
 *
 * @param[in] state Estado de recovery
 * @return String descriptivo del estado
 */
const char* recovery_state_to_string(recovery_state_t state);

/**
 * @brief Formatea información técnica para mostrar
 *
 * @param[in] firmware_info Información del firmware
 * @param[out] formatted_info Buffer para información formateada
 * @param[in] max_len Tamaño máximo del buffer
 * @return
 *      - ESP_OK: Información formateada exitosamente
 *      - ESP_FAIL: Error formateando información
 */
esp_err_t format_technical_info(const firmware_info_t* firmware_info, char* formatted_info, size_t max_len);

/**
 * @brief Limpia recursos del modo recovery
 *
 * @return
 *      - ESP_OK: Recursos limpiados exitosamente
 *      - ESP_FAIL: Error limpiando recursos
 */
esp_err_t cleanup_recovery_mode(void);

#ifdef __cplusplus
}
#endif

#endif // RECOVERY_MODE_H 