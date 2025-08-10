/**
 * @file integrity_checker.h
 * @brief Módulo de verificación de integridad para bootloader personalizado
 *
 * Este módulo se encarga de:
 * - Calcular y verificar hashes SHA256 de la partición de aplicación
 * - Comparar con hashes almacenados en NVS
 * - Detectar corrupción automáticamente
 * - Gestionar metadata de integridad
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#ifndef INTEGRITY_CHECKER_H
#define INTEGRITY_CHECKER_H

#include "bootloader_config.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "nvs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================
 * FUNCIONES PÚBLICAS
 * ================================ */

/**
 * @brief Inicializa el módulo de verificación de integridad
 *
 * @return
 *      - ESP_OK: Inicialización exitosa
 *      - ESP_FAIL: Error en la inicialización
 */
esp_err_t integrity_checker_init(void);

/**
 * @brief Verifica la integridad de la partición de aplicación actual
 *
 * Esta función calcula el SHA256 de la partición app0 y lo compara
 * con el hash almacenado en NVS para detectar corrupción.
 *
 * @param[out] firmware_info Información detallada del firmware verificado
 * @return
 *      - ESP_OK: Firmware íntegro y válido
 *      - ESP_ERR_INVALID_CRC: Hash no coincide (firmware corrupto)
 *      - ESP_ERR_NOT_FOUND: No hay hash almacenado en NVS
 *      - ESP_FAIL: Error durante la verificación
 */
esp_err_t verify_app_partition_integrity(firmware_info_t *firmware_info);

/**
 * @brief Calcula el hash SHA256 de una partición completa
 *
 * @param[in] partition Puntero a la partición a verificar
 * @param[out] hash_output Buffer de 32 bytes para almacenar el hash
 * @return
 *      - ESP_OK: Cálculo exitoso
 *      - ESP_ERR_INVALID_ARG: Parámetros inválidos
 *      - ESP_FAIL: Error durante el cálculo
 */
esp_err_t calculate_partition_sha256(const esp_partition_t* partition, uint8_t* hash_output);

/**
 * @brief Compara dos hashes SHA256
 *
 * @param[in] hash1 Primer hash (32 bytes)
 * @param[in] hash2 Segundo hash (32 bytes)
 * @return true si los hashes son idénticos, false si no
 */
bool compare_sha256_hashes(const uint8_t* hash1, const uint8_t* hash2);

/**
 * @brief Lee el hash almacenado del firmware desde NVS
 *
 * @param[out] hash_output Buffer de 32 bytes para el hash leído
 * @return
 *      - ESP_OK: Hash leído exitosamente
 *      - ESP_ERR_NOT_FOUND: No hay hash almacenado
 *      - ESP_FAIL: Error de lectura
 */
esp_err_t read_stored_firmware_hash(uint8_t* hash_output);

/**
 * @brief Almacena el hash del firmware en NVS
 *
 * @param[in] hash Hash SHA256 a almacenar (32 bytes)
 * @return
 *      - ESP_OK: Hash almacenado exitosamente
 *      - ESP_ERR_INVALID_ARG: Hash nulo
 *      - ESP_FAIL: Error de escritura en NVS
 */
esp_err_t store_firmware_hash(const uint8_t* hash);

/**
 * @brief Verifica si un firmware tiene un encabezado válido
 *
 * @param[in] firmware_data Datos del firmware a verificar
 * @param[in] size Tamaño de los datos
 * @param[out] header Encabezado extraído (si es válido)
 * @return
 *      - ESP_OK: Encabezado válido
 *      - ESP_ERR_INVALID_CRC: Magic number incorrecto
 *      - ESP_ERR_INVALID_SIZE: Tamaño inválido
 *      - ESP_ERR_INVALID_ARG: Parámetros inválidos
 */
esp_err_t verify_firmware_header(const uint8_t* firmware_data, size_t size, firmware_header_t* header);

/**
 * @brief Calcula el hash SHA256 de datos en memoria
 *
 * @param[in] data Datos a hashear
 * @param[in] size Tamaño de los datos
 * @param[out] hash_output Buffer de 32 bytes para el hash
 * @return
 *      - ESP_OK: Cálculo exitoso
 *      - ESP_ERR_INVALID_ARG: Parámetros inválidos
 *      - ESP_FAIL: Error durante el cálculo
 */
esp_err_t calculate_data_sha256(const uint8_t* data, size_t size, uint8_t* hash_output);

/**
 * @brief Obtiene información detallada de la partición de aplicación actual
 *
 * @param[out] info Estructura con información del firmware
 * @return
 *      - ESP_OK: Información obtenida exitosamente
 *      - ESP_FAIL: Error obteniendo información
 */
esp_err_t get_current_firmware_info(firmware_info_t* info);

/**
 * @brief Valida que una partición tenga el tamaño correcto para firmware
 *
 * @param[in] partition Partición a validar
 * @return
 *      - ESP_OK: Partición válida
 *      - ESP_ERR_INVALID_SIZE: Tamaño de partición incorrecto
 *      - ESP_ERR_INVALID_ARG: Partición nula
 */
esp_err_t validate_partition_size(const esp_partition_t* partition);

/**
 * @brief Limpia todos los datos de integridad almacenados en NVS
 *
 * Útil para resetear el sistema de verificación.
 *
 * @return
 *      - ESP_OK: Limpieza exitosa
 *      - ESP_FAIL: Error durante la limpieza
 */
esp_err_t clear_integrity_data(void);

/* ================================
 * FUNCIONES DE UTILIDAD
 * ================================ */

/**
 * @brief Convierte un hash binario a string hexadecimal
 *
 * @param[in] hash Hash binario (32 bytes)
 * @param[out] hex_string String de salida (mínimo 65 chars)
 * @return
 *      - ESP_OK: Conversión exitosa
 *      - ESP_ERR_INVALID_ARG: Parámetros inválidos
 */
esp_err_t hash_to_hex_string(const uint8_t* hash, char* hex_string);

/**
 * @brief Convierte un string hexadecimal a hash binario
 *
 * @param[in] hex_string String hexadecimal (64 chars)
 * @param[out] hash Hash binario de salida (32 bytes)
 * @return
 *      - ESP_OK: Conversión exitosa
 *      - ESP_ERR_INVALID_ARG: Parámetros inválidos
 *      - ESP_ERR_INVALID_SIZE: String de tamaño incorrecto
 */
esp_err_t hex_string_to_hash(const char* hex_string, uint8_t* hash);

#ifdef __cplusplus
}
#endif

#endif // INTEGRITY_CHECKER_H 