/**
 * @file sd_recovery.h
 * @brief Módulo de recuperación desde SD para bootloader personalizado
 *
 * Este módulo se encarga de:
 * - Montar y desmontar la tarjeta SD
 * - Buscar y verificar firmware de recovery en SD
 * - Flashear firmware desde SD a la partición de aplicación
 * - Gestionar cleanup de archivos temporales
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#ifndef SD_RECOVERY_H
#define SD_RECOVERY_H

#include "bootloader_config.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================
 * FUNCIONES PÚBLICAS
 * ================================ */

/**
 * @brief Inicializa el módulo de recovery desde SD
 *
 * @return
 *      - ESP_OK: Inicialización exitosa
 *      - ESP_FAIL: Error en la inicialización
 */
esp_err_t sd_recovery_init(void);

/**
 * @brief Monta la tarjeta SD si no está montada
 *
 * @return
 *      - ESP_OK: SD montada exitosamente o ya estaba montada
 *      - ESP_FAIL: Error al montar la SD
 *      - ESP_ERR_NOT_FOUND: SD no detectada
 */
esp_err_t mount_sd_card(void);

/**
 * @brief Desmonta la tarjeta SD
 *
 * @return
 *      - ESP_OK: SD desmontada exitosamente
 *      - ESP_FAIL: Error al desmontar
 */
esp_err_t unmount_sd_card(void);

/**
 * @brief Busca firmware de recovery disponible en la SD
 *
 * Busca en orden de prioridad:
 * 1. update.bin (si existe)
 * 2. base_firmware.bin
 *
 * @param[out] firmware_path Ruta del firmware encontrado
 * @param[out] hash_path Ruta del archivo hash correspondiente
 * @param[in] max_path_len Tamaño máximo de los buffers de ruta
 * @return
 *      - ESP_OK: Firmware encontrado
 *      - ESP_ERR_NOT_FOUND: No se encontró firmware válido
 *      - ESP_ERR_INVALID_ARG: Parámetros inválidos
 */
esp_err_t find_recovery_firmware(char* firmware_path, char* hash_path, size_t max_path_len);

/**
 * @brief Verifica la integridad del firmware en SD usando SHA256
 *
 * @param[in] firmware_path Ruta del archivo firmware
 * @param[in] hash_path Ruta del archivo con hash SHA256
 * @return
 *      - ESP_OK: Firmware íntegro y válido
 *      - ESP_ERR_INVALID_CRC: Hash no coincide
 *      - ESP_ERR_NOT_FOUND: Archivo no encontrado
 *      - ESP_FAIL: Error durante la verificación
 */
esp_err_t verify_sd_firmware_integrity(const char* firmware_path, const char* hash_path);

/**
 * @brief Flashea firmware desde SD a la partición de aplicación
 *
 * @param[in] firmware_path Ruta del firmware en SD
 * @return
 *      - ESP_OK: Flasheo exitoso
 *      - ESP_ERR_INVALID_ARG: Ruta inválida
 *      - ESP_ERR_NOT_FOUND: Archivo no encontrado
 *      - ESP_FAIL: Error durante el flasheo
 */
esp_err_t flash_firmware_from_sd(const char* firmware_path);

/**
 * @brief Limpia archivos temporales de recovery en SD
 *
 * Elimina archivos como update.bin tras un recovery exitoso.
 *
 * @return
 *      - ESP_OK: Limpieza exitosa
 *      - ESP_FAIL: Error durante la limpieza
 */
esp_err_t cleanup_recovery_files(void);

/**
 * @brief Crea directorio de recovery en SD si no existe
 *
 * @return
 *      - ESP_OK: Directorio existe o fue creado
 *      - ESP_FAIL: Error creando directorio
 */
esp_err_t create_recovery_directory(void);

/**
 * @brief Lee el contenido de un archivo hash desde SD
 *
 * @param[in] hash_file_path Ruta del archivo hash
 * @param[out] hash_output Buffer para almacenar hash (32 bytes)
 * @return
 *      - ESP_OK: Hash leído exitosamente
 *      - ESP_ERR_NOT_FOUND: Archivo no encontrado
 *      - ESP_ERR_INVALID_SIZE: Archivo de tamaño incorrecto
 *      - ESP_FAIL: Error de lectura
 */
esp_err_t read_hash_file_from_sd(const char* hash_file_path, uint8_t* hash_output);

/**
 * @brief Escribe un archivo hash en SD
 *
 * @param[in] hash_file_path Ruta donde escribir el hash
 * @param[in] hash Hash a escribir (32 bytes)
 * @return
 *      - ESP_OK: Hash escrito exitosamente
 *      - ESP_ERR_INVALID_ARG: Parámetros inválidos
 *      - ESP_FAIL: Error de escritura
 */
esp_err_t write_hash_file_to_sd(const char* hash_file_path, const uint8_t* hash);

/**
 * @brief Verifica si la SD está montada y accesible
 *
 * @return
 *      - ESP_OK: SD montada y accesible
 *      - ESP_FAIL: SD no accesible
 */
esp_err_t check_sd_accessibility(void);

/**
 * @brief Obtiene información de espacio disponible en SD
 *
 * @param[out] total_bytes Total de bytes en SD
 * @param[out] free_bytes Bytes libres en SD
 * @return
 *      - ESP_OK: Información obtenida
 *      - ESP_FAIL: Error obteniendo información
 */
esp_err_t get_sd_space_info(uint64_t* total_bytes, uint64_t* free_bytes);

/* ================================
 * FUNCIONES DE RECUPERACIÓN AVANZADA
 * ================================ */

/**
 * @brief Realiza recovery completo desde SD
 *
 * Función principal que ejecuta todo el proceso de recovery:
 * 1. Monta SD
 * 2. Busca firmware
 * 3. Verifica integridad
 * 4. Flashea firmware
 * 5. Limpia archivos temporales
 *
 * @param[out] recovery_info Información del proceso de recovery
 * @return
 *      - ESP_OK: Recovery exitoso
 *      - ESP_ERR_NOT_FOUND: No se encontró firmware válido
 *      - ESP_FAIL: Error durante el recovery
 */
esp_err_t perform_full_sd_recovery(recovery_state_t* recovery_info);

/**
 * @brief Crea backup del firmware actual en SD antes de actualizar
 *
 * @param[in] backup_path Ruta donde crear el backup
 * @return
 *      - ESP_OK: Backup creado exitosamente
 *      - ESP_FAIL: Error creando backup
 */
esp_err_t create_firmware_backup_to_sd(const char* backup_path);

/**
 * @brief Valida que un archivo de firmware tenga formato correcto
 *
 * @param[in] firmware_path Ruta del firmware a validar
 * @param[out] firmware_size Tamaño del firmware (si es válido)
 * @return
 *      - ESP_OK: Firmware válido
 *      - ESP_ERR_INVALID_SIZE: Tamaño incorrecto
 *      - ESP_ERR_NOT_FOUND: Archivo no encontrado
 *      - ESP_FAIL: Formato inválido
 */
esp_err_t validate_firmware_file(const char* firmware_path, size_t* firmware_size);

/* ================================
 * FUNCIONES DE LOGGING
 * ================================ */

/**
 * @brief Escribe entrada de log de recovery en SD
 *
 * @param[in] message Mensaje a escribir en el log
 * @param[in] severity Nivel de severidad (INFO, WARN, ERROR)
 * @return
 *      - ESP_OK: Log escrito exitosamente
 *      - ESP_FAIL: Error escribiendo log
 */
esp_err_t write_recovery_log(const char* message, const char* severity);

/**
 * @brief Limpia logs antiguos de recovery
 *
 * @return
 *      - ESP_OK: Logs limpiados
 *      - ESP_FAIL: Error limpiando logs
 */
esp_err_t cleanup_old_recovery_logs(void);

#ifdef __cplusplus
}
#endif

#endif // SD_RECOVERY_H 