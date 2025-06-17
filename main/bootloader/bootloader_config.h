/**
 * @file bootloader_config.h
 * @brief Configuración principal del bootloader personalizado para TriptaLabs Heat Controller
 *
 * Este archivo contiene todas las definiciones, constantes y configuraciones
 * necesarias para el sistema de bootloader con recovery automático desde SD.
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================
 * CONFIGURACIÓN GENERAL
 * ================================ */

/**
 * @brief Versión del bootloader personalizado
 */
#define BOOTLOADER_VERSION "1.0.0"

/**
 * @brief Magic number para validar firmware válido
 */
#define FIRMWARE_MAGIC_NUMBER 0xDEADBEEF

/**
 * @brief Tamaño mínimo esperado para un firmware válido (en bytes)
 * Firmware actual es ~5.5MB, mínimo seguro 1MB
 */
#define FIRMWARE_MIN_SIZE (1024 * 1024)

/**
 * @brief Tamaño máximo esperado para firmware (en bytes)
 * Partición app0 es 10MB, dejamos margen de seguridad
 */
#define FIRMWARE_MAX_SIZE (9 * 1024 * 1024)

/**
 * @brief Tamaño del buffer para operaciones de lectura/escritura
 */
#define BOOTLOADER_BUFFER_SIZE 4096

/**
 * @brief Timeout para operaciones de SD en milisegundos
 */
#define SD_OPERATION_TIMEOUT_MS 30000

/* ================================
 * CONFIGURACIÓN NVS
 * ================================ */

/**
 * @brief Namespace NVS para datos del bootloader
 */
#define BOOTLOADER_NVS_NAMESPACE "bootloader"

/**
 * @brief Claves NVS para persistencia de estado
 */
#define NVS_KEY_APP_HASH "app_hash"
#define NVS_KEY_BOOT_ATTEMPTS "boot_attempts"
#define NVS_KEY_LAST_BOOT_REASON "last_boot_reason"
#define NVS_KEY_RECOVERY_COUNT "recovery_count"
#define NVS_KEY_FIRST_BOOT "first_boot"

/**
 * @brief Máximo número de intentos de boot antes de forzar recovery
 */
#define MAX_BOOT_ATTEMPTS 3

/**
 * @brief Máximo número de intentos de recovery desde SD
 */
#define MAX_RECOVERY_ATTEMPTS 3

/* ================================
 * CONFIGURACIÓN DE RUTAS SD
 * ================================ */

/**
 * @brief Punto de montaje de la SD
 */
#define SD_MOUNT_POINT "/sdcard"

/**
 * @brief Directorio de recovery en la SD
 */
#define SD_RECOVERY_DIR "/sdcard/recovery"

/**
 * @brief Ruta del firmware base en SD
 */
#define SD_BASE_FIRMWARE_PATH "/sdcard/recovery/base_firmware.bin"

/**
 * @brief Ruta del hash del firmware base en SD
 */
#define SD_BASE_FIRMWARE_HASH_PATH "/sdcard/recovery/base_firmware.bin.sha256"

/**
 * @brief Ruta del firmware de actualización en SD
 */
#define SD_UPDATE_FIRMWARE_PATH "/sdcard/recovery/update.bin"

/**
 * @brief Ruta del hash del firmware de actualización en SD
 */
#define SD_UPDATE_FIRMWARE_HASH_PATH "/sdcard/recovery/update.bin.sha256"

/**
 * @brief Ruta del log de recovery en SD
 */
#define SD_RECOVERY_LOG_PATH "/sdcard/recovery/recovery.log"

/* ================================
 * ESTRUCTURAS DE DATOS
 * ================================ */

/**
 * @brief Estructura del encabezado de firmware
 */
typedef struct {
    uint32_t magic;           ///< Magic number (0xDEADBEEF)
    uint32_t version;         ///< Versión del firmware
    uint32_t size;            ///< Tamaño del firmware en bytes
    uint8_t sha256[32];       ///< Hash SHA256 del firmware
    uint32_t crc32;           ///< CRC32 adicional
    uint32_t timestamp;       ///< Timestamp de compilación
    char build_info[64];      ///< Información de build
} __attribute__((packed)) firmware_header_t;

/**
 * @brief Razones de boot del sistema
 */
typedef enum {
    BOOT_REASON_NORMAL = 0,           ///< Boot normal sin problemas
    BOOT_REASON_CORRUPTION,           ///< App corrupta detectada
    BOOT_REASON_UPDATE_FAILED,        ///< Actualización falló
    BOOT_REASON_RECOVERY,             ///< Modo recovery activado
    BOOT_REASON_MULTIPLE_FAILURES,    ///< Múltiples fallos consecutivos
    BOOT_REASON_SD_RECOVERY,          ///< Recovery desde SD exitoso
    BOOT_REASON_EMERGENCY             ///< Modo emergency (último recurso)
} boot_reason_t;

/**
 * @brief Estados del sistema de recovery
 */
typedef enum {
    RECOVERY_STATE_IDLE = 0,          ///< Sin recovery en progreso
    RECOVERY_STATE_CHECKING,          ///< Verificando integridad
    RECOVERY_STATE_SD_MOUNT,          ///< Montando SD
    RECOVERY_STATE_FIRMWARE_VERIFY,   ///< Verificando firmware en SD
    RECOVERY_STATE_FLASHING,          ///< Flasheando firmware
    RECOVERY_STATE_CLEANUP,           ///< Limpiando archivos temporales
    RECOVERY_STATE_SUCCESS,           ///< Recovery exitoso
    RECOVERY_STATE_FAILED             ///< Recovery falló
} recovery_state_t;

/**
 * @brief Estadísticas de boot y recovery
 */
typedef struct {
    uint8_t boot_attempts;            ///< Intentos de boot actuales
    uint8_t recovery_attempts;        ///< Intentos de recovery actuales
    uint32_t total_boots;             ///< Total de boots realizados
    uint32_t total_recoveries;        ///< Total de recoveries realizados
    boot_reason_t last_boot_reason;   ///< Última razón de boot
    uint32_t last_recovery_timestamp; ///< Timestamp del último recovery
    bool first_boot;                  ///< Flag de primer boot
} bootloader_stats_t;

/**
 * @brief Información de firmware detectado
 */
typedef struct {
    bool valid;                       ///< Firmware es válido
    uint32_t size;                    ///< Tamaño del firmware
    uint8_t calculated_hash[32];      ///< Hash calculado
    uint8_t stored_hash[32];          ///< Hash almacenado en NVS
    bool hash_match;                  ///< Hashes coinciden
    firmware_header_t header;         ///< Encabezado del firmware
} firmware_info_t;

/* ================================
 * CONFIGURACIÓN DE PANTALLA RECOVERY
 * ================================ */

/**
 * @brief Configuración de colores para modo recovery
 */
#define RECOVERY_COLOR_BACKGROUND 0x0000      // Negro
#define RECOVERY_COLOR_ERROR      0xF800      // Rojo
#define RECOVERY_COLOR_WARNING    0xFFE0      // Amarillo
#define RECOVERY_COLOR_SUCCESS    0x07E0      // Verde
#define RECOVERY_COLOR_INFO       0x001F      // Azul
#define RECOVERY_COLOR_TEXT       0xFFFF      // Blanco

/**
 * @brief Timeouts para pantalla de recovery
 */
#define RECOVERY_DISPLAY_TIMEOUT_MS 30000     // 30 segundos
#define RECOVERY_MESSAGE_DELAY_MS   2000      // 2 segundos entre mensajes

/* ================================
 * CONFIGURACIÓN DE LOGGING
 * ================================ */

/**
 * @brief Tag para logs del bootloader
 */
#define BOOTLOADER_TAG "BOOTLOADER"

/**
 * @brief Habilitar logging detallado en SD
 */
#define ENABLE_SD_LOGGING 1

/**
 * @brief Tamaño máximo del archivo de log
 */
#define MAX_LOG_FILE_SIZE (100 * 1024)  // 100KB

/* ================================
 * MACROS DE UTILIDAD
 * ================================ */

/**
 * @brief Macro para verificar y retornar error
 */
#define BOOTLOADER_CHECK_RET(x) do { \
    esp_err_t err_rc_ = (x); \
    if (err_rc_ != ESP_OK) { \
        return err_rc_; \
    } \
} while(0)

/**
 * @brief Macro para logging con timestamp
 */
#define BOOTLOADER_LOG_WITH_TIME(level, format, ...) \
    ESP_LOG##level(BOOTLOADER_TAG, "[%lu] " format, esp_log_timestamp(), ##__VA_ARGS__)

/**
 * @brief Validar magic number de firmware
 */
#define IS_VALID_FIRMWARE_MAGIC(magic) ((magic) == FIRMWARE_MAGIC_NUMBER)

/**
 * @brief Validar tamaño de firmware
 */
#define IS_VALID_FIRMWARE_SIZE(size) \
    ((size) >= FIRMWARE_MIN_SIZE && (size) <= FIRMWARE_MAX_SIZE)

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_CONFIG_H 