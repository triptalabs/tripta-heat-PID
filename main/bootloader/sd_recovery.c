/**
 * @file sd_recovery.c
 * @brief Implementación del módulo de recuperación desde SD
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#include "sd_recovery.h"
#include "integrity_checker.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "mbedtls/sha256.h"
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ================================
 * VARIABLES PRIVADAS
 * ================================ */

/**
 * @brief Flag que indica si SD está montada
 */
static bool sd_mounted = false;

/**
 * @brief Card handle para operaciones SD
 */
static sdmmc_card_t* sd_card = NULL;

/**
 * @brief Flag de inicialización del módulo
 */
static bool sd_recovery_initialized = false;

/* ================================
 * FUNCIONES PRIVADAS
 * ================================ */

/**
 * @brief Verifica si un archivo existe
 */
static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/**
 * @brief Obtiene el tamaño de un archivo
 */
static size_t get_file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

/**
 * @brief Lee un archivo hash en formato hexadecimal
 */
static esp_err_t read_hex_hash_file(const char* path, uint8_t* hash_output) {
    FILE* f = fopen(path, "r");
    if (!f) {
        return ESP_ERR_NOT_FOUND;
    }

    char hex_string[65] = {0};
    size_t read_chars = fread(hex_string, 1, 64, f);
    fclose(f);

    if (read_chars != 64) {
        ESP_LOGE(BOOTLOADER_TAG, "Archivo hash tiene tamaño incorrecto: %zu chars", read_chars);
        return ESP_ERR_INVALID_SIZE;
    }

    // Convertir hex string a bytes
    return hex_string_to_hash(hex_string, hash_output);
}

/**
 * @brief Escribe un archivo hash en formato hexadecimal
 */
static esp_err_t write_hex_hash_file(const char* path, const uint8_t* hash) {
    FILE* f = fopen(path, "w");
    if (!f) {
        return ESP_FAIL;
    }

    char hex_string[65];
    hash_to_hex_string(hash, hex_string);
    
    size_t written = fwrite(hex_string, 1, 64, f);
    fclose(f);

    return (written == 64) ? ESP_OK : ESP_FAIL;
}

/* ================================
 * FUNCIONES PÚBLICAS
 * ================================ */

esp_err_t sd_recovery_init(void) {
    if (sd_recovery_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Inicializando módulo de recovery desde SD");
    
    sd_recovery_initialized = true;
    ESP_LOGI(BOOTLOADER_TAG, "Módulo SD recovery inicializado exitosamente");
    return ESP_OK;
}

esp_err_t mount_sd_card(void) {
    if (sd_mounted) {
        return ESP_OK; // Ya montada
    }

    ESP_LOGI(BOOTLOADER_TAG, "Montando tarjeta SD...");

    // Configurar host SD
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 20000; // Reducir frecuencia para mayor compatibilidad

    // Configurar slot
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1; // Usar modo 1-bit para mayor compatibilidad

    // Configurar VFS
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &sd_card);
    
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "Error montando SD: %s", esp_err_to_name(ret));
        if (ret == ESP_FAIL) {
            ESP_LOGE(BOOTLOADER_TAG, "SD no detectada o no formateada");
        }
        return ret;
    }

    sd_mounted = true;
    ESP_LOGI(BOOTLOADER_TAG, "✅ SD montada exitosamente en %s", SD_MOUNT_POINT);

    // Mostrar información de la SD
    uint64_t total_bytes = ((uint64_t) sd_card->csd.capacity) * sd_card->csd.sector_size;
    ESP_LOGI(BOOTLOADER_TAG, "SD Card: %s, %.2f GB", 
             sd_card->cid.name, 
             (float)total_bytes / (1024 * 1024 * 1024));

    return ESP_OK;
}

esp_err_t unmount_sd_card(void) {
    if (!sd_mounted) {
        return ESP_OK;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, sd_card);
    if (ret == ESP_OK) {
        sd_mounted = false;
        sd_card = NULL;
        ESP_LOGI(BOOTLOADER_TAG, "SD desmontada exitosamente");
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "Error desmontando SD: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t find_recovery_firmware(char* firmware_path, char* hash_path, size_t max_path_len) {
    if (!firmware_path || !hash_path || max_path_len < 64) {
        return ESP_ERR_INVALID_ARG;
    }

    // Asegurar que SD está montada
    BOOTLOADER_CHECK_RET(mount_sd_card());

    // Crear directorio de recovery si no existe
    create_recovery_directory();

    // Prioridad 1: Buscar update.bin (actualizaciones pendientes)
    if (file_exists(SD_UPDATE_FIRMWARE_PATH) && file_exists(SD_UPDATE_FIRMWARE_HASH_PATH)) {
        strncpy(firmware_path, SD_UPDATE_FIRMWARE_PATH, max_path_len - 1);
        strncpy(hash_path, SD_UPDATE_FIRMWARE_HASH_PATH, max_path_len - 1);
        firmware_path[max_path_len - 1] = '\0';
        hash_path[max_path_len - 1] = '\0';
        
        ESP_LOGI(BOOTLOADER_TAG, "Encontrado firmware de actualización: %s", firmware_path);
        return ESP_OK;
    }

    // Prioridad 2: Buscar base_firmware.bin (firmware base)
    if (file_exists(SD_BASE_FIRMWARE_PATH) && file_exists(SD_BASE_FIRMWARE_HASH_PATH)) {
        strncpy(firmware_path, SD_BASE_FIRMWARE_PATH, max_path_len - 1);
        strncpy(hash_path, SD_BASE_FIRMWARE_HASH_PATH, max_path_len - 1);
        firmware_path[max_path_len - 1] = '\0';
        hash_path[max_path_len - 1] = '\0';
        
        ESP_LOGI(BOOTLOADER_TAG, "Encontrado firmware base: %s", firmware_path);
        return ESP_OK;
    }

    ESP_LOGW(BOOTLOADER_TAG, "No se encontró firmware válido en SD");
    return ESP_ERR_NOT_FOUND;
}

esp_err_t verify_sd_firmware_integrity(const char* firmware_path, const char* hash_path) {
    if (!firmware_path || !hash_path) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Verificando integridad de firmware: %s", firmware_path);

    // Verificar que ambos archivos existen
    if (!file_exists(firmware_path)) {
        ESP_LOGE(BOOTLOADER_TAG, "Archivo firmware no encontrado: %s", firmware_path);
        return ESP_ERR_NOT_FOUND;
    }

    if (!file_exists(hash_path)) {
        ESP_LOGE(BOOTLOADER_TAG, "Archivo hash no encontrado: %s", hash_path);
        return ESP_ERR_NOT_FOUND;
    }

    // Leer hash esperado desde archivo
    uint8_t expected_hash[32];
    esp_err_t ret = read_hex_hash_file(hash_path, expected_hash);
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "Error leyendo archivo hash: %s", esp_err_to_name(ret));
        return ret;
    }

    // Abrir archivo firmware
    FILE* firmware_file = fopen(firmware_path, "rb");
    if (!firmware_file) {
        ESP_LOGE(BOOTLOADER_TAG, "Error abriendo archivo firmware");
        return ESP_FAIL;
    }

    // Calcular hash del archivo
    uint8_t calculated_hash[32];
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0);

    uint8_t* buffer = malloc(BOOTLOADER_BUFFER_SIZE);
    if (!buffer) {
        fclose(firmware_file);
        mbedtls_sha256_free(&sha256_ctx);
        return ESP_ERR_NO_MEM;
    }

    size_t total_read = 0;
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BOOTLOADER_BUFFER_SIZE, firmware_file)) > 0) {
        mbedtls_sha256_update(&sha256_ctx, buffer, bytes_read);
        total_read += bytes_read;

        // Log progreso cada 1MB
        if (total_read % (1024 * 1024) == 0) {
            ESP_LOGI(BOOTLOADER_TAG, "Verificando: %.1f MB procesados", 
                     (float)total_read / (1024 * 1024));
        }
    }

    mbedtls_sha256_finish(&sha256_ctx, calculated_hash);
    
    free(buffer);
    fclose(firmware_file);
    mbedtls_sha256_free(&sha256_ctx);

    // Comparar hashes
    if (compare_sha256_hashes(calculated_hash, expected_hash)) {
        ESP_LOGI(BOOTLOADER_TAG, "✅ Verificación de integridad exitosa (%.1f MB)", 
                 (float)total_read / (1024 * 1024));
        return ESP_OK;
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "❌ Hash no coincide - archivo corrupto");
        
        // Log hashes para debugging
        char calc_str[65], expected_str[65];
        hash_to_hex_string(calculated_hash, calc_str);
        hash_to_hex_string(expected_hash, expected_str);
        ESP_LOGE(BOOTLOADER_TAG, "Calculado: %s", calc_str);
        ESP_LOGE(BOOTLOADER_TAG, "Esperado:  %s", expected_str);
        
        return ESP_ERR_INVALID_CRC;
    }
}

esp_err_t flash_firmware_from_sd(const char* firmware_path) {
    if (!firmware_path) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Iniciando flasheo desde SD: %s", firmware_path);

    // Abrir archivo firmware
    FILE* firmware_file = fopen(firmware_path, "rb");
    if (!firmware_file) {
        ESP_LOGE(BOOTLOADER_TAG, "Error abriendo archivo firmware");
        return ESP_ERR_NOT_FOUND;
    }

    // Obtener partición de aplicación actual
    const esp_partition_t* app_partition = esp_ota_get_running_partition();
    if (!app_partition) {
        fclose(firmware_file);
        ESP_LOGE(BOOTLOADER_TAG, "No se pudo obtener partición de aplicación");
        return ESP_FAIL;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Flasheando a partición: %s", app_partition->label);

    // Iniciar proceso OTA
    esp_ota_handle_t ota_handle;
    esp_err_t ret = esp_ota_begin(app_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (ret != ESP_OK) {
        fclose(firmware_file);
        ESP_LOGE(BOOTLOADER_TAG, "Error iniciando OTA: %s", esp_err_to_name(ret));
        return ret;
    }

    // Buffer para lectura
    uint8_t* buffer = malloc(BOOTLOADER_BUFFER_SIZE);
    if (!buffer) {
        fclose(firmware_file);
        esp_ota_abort(ota_handle);
        return ESP_ERR_NO_MEM;
    }

    size_t total_written = 0;
    size_t bytes_read;

    // Flashear archivo por bloques
    while ((bytes_read = fread(buffer, 1, BOOTLOADER_BUFFER_SIZE, firmware_file)) > 0) {
        ret = esp_ota_write(ota_handle, buffer, bytes_read);
        if (ret != ESP_OK) {
            ESP_LOGE(BOOTLOADER_TAG, "Error escribiendo OTA: %s", esp_err_to_name(ret));
            break;
        }
        
        total_written += bytes_read;

        // Log progreso cada 1MB
        if (total_written % (1024 * 1024) == 0) {
            ESP_LOGI(BOOTLOADER_TAG, "Flasheando: %.1f MB escritos", 
                     (float)total_written / (1024 * 1024));
        }
    }

    free(buffer);
    fclose(firmware_file);

    if (ret == ESP_OK) {
        // Finalizar OTA
        ret = esp_ota_end(ota_handle);
        if (ret == ESP_OK) {
            // Establecer nueva partición de boot
            ret = esp_ota_set_boot_partition(app_partition);
            if (ret == ESP_OK) {
                ESP_LOGI(BOOTLOADER_TAG, "✅ Flasheo exitoso (%.1f MB)", 
                         (float)total_written / (1024 * 1024));
                
                // Almacenar nuevo hash en NVS
                uint8_t new_hash[32];
                if (calculate_partition_sha256(app_partition, new_hash) == ESP_OK) {
                    store_firmware_hash(new_hash);
                }
            } else {
                ESP_LOGE(BOOTLOADER_TAG, "Error estableciendo partición de boot: %s", 
                         esp_err_to_name(ret));
            }
        } else {
            ESP_LOGE(BOOTLOADER_TAG, "Error finalizando OTA: %s", esp_err_to_name(ret));
        }
    } else {
        // Abortar OTA en caso de error
        esp_ota_abort(ota_handle);
    }

    return ret;
}

esp_err_t cleanup_recovery_files(void) {
    ESP_LOGI(BOOTLOADER_TAG, "Limpiando archivos de recovery...");

    // Eliminar archivos de actualización si existen
    if (file_exists(SD_UPDATE_FIRMWARE_PATH)) {
        if (remove(SD_UPDATE_FIRMWARE_PATH) == 0) {
            ESP_LOGI(BOOTLOADER_TAG, "Eliminado: %s", SD_UPDATE_FIRMWARE_PATH);
        } else {
            ESP_LOGW(BOOTLOADER_TAG, "Error eliminando: %s", SD_UPDATE_FIRMWARE_PATH);
        }
    }

    if (file_exists(SD_UPDATE_FIRMWARE_HASH_PATH)) {
        if (remove(SD_UPDATE_FIRMWARE_HASH_PATH) == 0) {
            ESP_LOGI(BOOTLOADER_TAG, "Eliminado: %s", SD_UPDATE_FIRMWARE_HASH_PATH);
        } else {
            ESP_LOGW(BOOTLOADER_TAG, "Error eliminando: %s", SD_UPDATE_FIRMWARE_HASH_PATH);
        }
    }

    ESP_LOGI(BOOTLOADER_TAG, "Limpieza de archivos completada");
    return ESP_OK;
}

esp_err_t create_recovery_directory(void) {
    struct stat st = {0};
    
    if (stat(SD_RECOVERY_DIR, &st) == -1) {
        if (mkdir(SD_RECOVERY_DIR, 0755) == 0) {
            ESP_LOGI(BOOTLOADER_TAG, "Directorio recovery creado: %s", SD_RECOVERY_DIR);
        } else {
            ESP_LOGE(BOOTLOADER_TAG, "Error creando directorio recovery");
            return ESP_FAIL;
        }
    }
    
    return ESP_OK;
}

esp_err_t check_sd_accessibility(void) {
    // Intentar montar si no está montada
    esp_err_t ret = mount_sd_card();
    if (ret != ESP_OK) {
        return ret;
    }

    // Verificar acceso escribiendo un archivo temporal
    const char* test_file = "/sdcard/test_access.tmp";
    FILE* f = fopen(test_file, "w");
    if (!f) {
        ESP_LOGE(BOOTLOADER_TAG, "SD no accesible para escritura");
        return ESP_FAIL;
    }

    fprintf(f, "test");
    fclose(f);
    remove(test_file);

    return ESP_OK;
}

esp_err_t get_sd_space_info(uint64_t* total_bytes, uint64_t* free_bytes) {
    if (!total_bytes || !free_bytes) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sd_mounted || !sd_card) {
        return ESP_ERR_INVALID_STATE;
    }

    *total_bytes = ((uint64_t) sd_card->csd.capacity) * sd_card->csd.sector_size;
    
    // Para obtener espacio libre necesitaríamos acceso al filesystem
    // Por simplicidad, retornamos información básica
    *free_bytes = *total_bytes / 2; // Estimación aproximada
    
    return ESP_OK;
}

esp_err_t perform_full_sd_recovery(recovery_state_t* recovery_info) {
    if (!recovery_info) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(BOOTLOADER_TAG, "=== Iniciando Recovery Completo desde SD ===");
    
    *recovery_info = RECOVERY_STATE_SD_MOUNT;
    esp_err_t ret = mount_sd_card();
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "❌ Error montando SD");
        *recovery_info = RECOVERY_STATE_FAILED;
        return ret;
    }

    char firmware_path[128], hash_path[128];
    ret = find_recovery_firmware(firmware_path, hash_path, sizeof(firmware_path));
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "❌ No se encontró firmware válido en SD");
        *recovery_info = RECOVERY_STATE_FAILED;
        return ret;
    }

    *recovery_info = RECOVERY_STATE_FIRMWARE_VERIFY;
    ret = verify_sd_firmware_integrity(firmware_path, hash_path);
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "❌ Firmware en SD está corrupto");
        *recovery_info = RECOVERY_STATE_FAILED;
        return ret;
    }

    *recovery_info = RECOVERY_STATE_FLASHING;
    ret = flash_firmware_from_sd(firmware_path);
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "❌ Error flasheando firmware desde SD");
        *recovery_info = RECOVERY_STATE_FAILED;
        return ret;
    }

    *recovery_info = RECOVERY_STATE_CLEANUP;
    cleanup_recovery_files();

    *recovery_info = RECOVERY_STATE_SUCCESS;
    ESP_LOGI(BOOTLOADER_TAG, "✅ Recovery completo exitoso");

    return ESP_OK;
}

esp_err_t write_recovery_log(const char* message, const char* severity) {
    if (!message || !severity) {
        return ESP_ERR_INVALID_ARG;
    }

    if (mount_sd_card() != ESP_OK) {
        return ESP_FAIL; // No se puede escribir log si SD no es accesible
    }

    create_recovery_directory();

    FILE* log_file = fopen(SD_RECOVERY_LOG_PATH, "a");
    if (!log_file) {
        return ESP_FAIL;
    }

    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);

    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s\n",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
            severity, message);

    fclose(log_file);
    return ESP_OK;
}

esp_err_t read_hash_file_from_sd(const char* hash_file_path, uint8_t* hash_output) {
    return read_hex_hash_file(hash_file_path, hash_output);
}

esp_err_t write_hash_file_to_sd(const char* hash_file_path, const uint8_t* hash) {
    return write_hex_hash_file(hash_file_path, hash);
}

esp_err_t validate_firmware_file(const char* firmware_path, size_t* firmware_size) {
    if (!firmware_path) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!file_exists(firmware_path)) {
        return ESP_ERR_NOT_FOUND;
    }

    size_t size = get_file_size(firmware_path);
    if (!IS_VALID_FIRMWARE_SIZE(size)) {
        ESP_LOGE(BOOTLOADER_TAG, "Tamaño de firmware inválido: %zu bytes", size);
        return ESP_ERR_INVALID_SIZE;
    }

    if (firmware_size) {
        *firmware_size = size;
    }

    return ESP_OK;
} 