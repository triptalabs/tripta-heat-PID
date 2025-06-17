/**
 * @file integrity_checker.c
 * @brief Implementación del módulo de verificación de integridad
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#include "integrity_checker.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "mbedtls/sha256.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

/* ================================
 * VARIABLES PRIVADAS
 * ================================ */

/**
 * @brief Handle NVS para datos de integridad
 */
static nvs_handle_t integrity_nvs_handle = 0;

/**
 * @brief Flag de inicialización del módulo
 */
static bool integrity_checker_initialized = false;

/* ================================
 * FUNCIONES PRIVADAS
 * ================================ */

/**
 * @brief Convierte byte a caracteres hexadecimales
 */
static void byte_to_hex(uint8_t byte, char* hex_chars) {
    const char hex_digits[] = "0123456789abcdef";
    hex_chars[0] = hex_digits[(byte >> 4) & 0xF];
    hex_chars[1] = hex_digits[byte & 0xF];
}

/**
 * @brief Convierte carácter hexadecimal a byte
 */
static uint8_t hex_char_to_byte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

/* ================================
 * FUNCIONES PÚBLICAS
 * ================================ */

esp_err_t integrity_checker_init(void) {
    if (integrity_checker_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Inicializando módulo de verificación de integridad");

    // Abrir namespace NVS para integridad
    esp_err_t ret = nvs_open(BOOTLOADER_NVS_NAMESPACE, NVS_READWRITE, &integrity_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "Error abriendo NVS para integridad: %s", esp_err_to_name(ret));
        return ret;
    }

    integrity_checker_initialized = true;
    ESP_LOGI(BOOTLOADER_TAG, "Módulo de integridad inicializado exitosamente");
    return ESP_OK;
}

esp_err_t calculate_partition_sha256(const esp_partition_t* partition, uint8_t* hash_output) {
    if (!partition || !hash_output) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Calculando SHA256 de partición %s (tamaño: %lu bytes)", 
             partition->label, partition->size);

    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0); // 0 para SHA256 (no SHA224)

    uint8_t* buffer = malloc(BOOTLOADER_BUFFER_SIZE);
    if (!buffer) {
        ESP_LOGE(BOOTLOADER_TAG, "Error allocando buffer para SHA256");
        mbedtls_sha256_free(&sha256_ctx);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    size_t bytes_processed = 0;
    size_t bytes_to_read;

    while (bytes_processed < partition->size) {
        bytes_to_read = (partition->size - bytes_processed > BOOTLOADER_BUFFER_SIZE) 
                       ? BOOTLOADER_BUFFER_SIZE 
                       : (partition->size - bytes_processed);

        ret = esp_partition_read(partition, bytes_processed, buffer, bytes_to_read);
        if (ret != ESP_OK) {
            ESP_LOGE(BOOTLOADER_TAG, "Error leyendo partición en offset %zu: %s", 
                     bytes_processed, esp_err_to_name(ret));
            break;
        }

        mbedtls_sha256_update(&sha256_ctx, buffer, bytes_to_read);
        bytes_processed += bytes_to_read;

        // Log de progreso cada 1MB
        if (bytes_processed % (1024 * 1024) == 0) {
            ESP_LOGI(BOOTLOADER_TAG, "Progreso SHA256: %zu/%lu bytes (%.1f%%)", 
                     bytes_processed, partition->size, 
                     (float)bytes_processed / partition->size * 100.0);
        }
    }

    if (ret == ESP_OK) {
        mbedtls_sha256_finish(&sha256_ctx, hash_output);
        ESP_LOGI(BOOTLOADER_TAG, "SHA256 calculado exitosamente (%zu bytes procesados)", bytes_processed);
    }

    free(buffer);
    mbedtls_sha256_free(&sha256_ctx);
    return ret;
}

bool compare_sha256_hashes(const uint8_t* hash1, const uint8_t* hash2) {
    if (!hash1 || !hash2) {
        return false;
    }
    return memcmp(hash1, hash2, 32) == 0;
}

esp_err_t read_stored_firmware_hash(uint8_t* hash_output) {
    if (!hash_output || !integrity_checker_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t required_size = 32;
    esp_err_t ret = nvs_get_blob(integrity_nvs_handle, NVS_KEY_APP_HASH, hash_output, &required_size);
    
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(BOOTLOADER_TAG, "No hay hash almacenado en NVS");
        return ESP_ERR_NOT_FOUND;
    } else if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "Error leyendo hash desde NVS: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    if (required_size != 32) {
        ESP_LOGE(BOOTLOADER_TAG, "Hash almacenado tiene tamaño incorrecto: %zu", required_size);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

esp_err_t store_firmware_hash(const uint8_t* hash) {
    if (!hash || !integrity_checker_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = nvs_set_blob(integrity_nvs_handle, NVS_KEY_APP_HASH, hash, 32);
    if (ret == ESP_OK) {
        ret = nvs_commit(integrity_nvs_handle);
    }

    if (ret == ESP_OK) {
        ESP_LOGI(BOOTLOADER_TAG, "Hash de firmware almacenado en NVS");
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "Error almacenando hash en NVS: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t verify_app_partition_integrity(firmware_info_t *firmware_info) {
    if (!firmware_info) {
        return ESP_ERR_INVALID_ARG;
    }

    // Inicializar estructura
    memset(firmware_info, 0, sizeof(firmware_info_t));

    // Obtener partición de aplicación actual
    const esp_partition_t* app_partition = esp_ota_get_running_partition();
    if (!app_partition) {
        ESP_LOGE(BOOTLOADER_TAG, "No se pudo obtener partición de aplicación");
        return ESP_FAIL;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Verificando integridad de partición %s", app_partition->label);

    // Validar tamaño de partición
    esp_err_t ret = validate_partition_size(app_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "Tamaño de partición inválido");
        return ret;
    }

    firmware_info->size = app_partition->size;

    // Calcular hash actual de la partición
    ret = calculate_partition_sha256(app_partition, firmware_info->calculated_hash);
    if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "Error calculando SHA256 actual");
        return ret;
    }

    // Intentar leer hash almacenado
    ret = read_stored_firmware_hash(firmware_info->stored_hash);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(BOOTLOADER_TAG, "No hay hash de referencia - primer boot detectado");
        // Almacenar hash actual como referencia
        store_firmware_hash(firmware_info->calculated_hash);
        firmware_info->hash_match = true;
        firmware_info->valid = true;
        return ESP_OK;
    } else if (ret != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "Error leyendo hash almacenado");
        return ret;
    }

    // Comparar hashes
    firmware_info->hash_match = compare_sha256_hashes(
        firmware_info->calculated_hash, 
        firmware_info->stored_hash
    );

    firmware_info->valid = firmware_info->hash_match;

    if (firmware_info->hash_match) {
        ESP_LOGI(BOOTLOADER_TAG, "✅ Verificación de integridad exitosa");
        return ESP_OK;
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "❌ Hash no coincide - firmware corrupto detectado");
        
        // Log de hashes para debugging
        char calc_hash_str[65], stored_hash_str[65];
        hash_to_hex_string(firmware_info->calculated_hash, calc_hash_str);
        hash_to_hex_string(firmware_info->stored_hash, stored_hash_str);
        
        ESP_LOGE(BOOTLOADER_TAG, "Hash calculado:  %s", calc_hash_str);
        ESP_LOGE(BOOTLOADER_TAG, "Hash almacenado: %s", stored_hash_str);
        
        return ESP_ERR_INVALID_CRC;
    }
}

esp_err_t calculate_data_sha256(const uint8_t* data, size_t size, uint8_t* hash_output) {
    if (!data || !hash_output || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0);
    mbedtls_sha256_update(&sha256_ctx, data, size);
    mbedtls_sha256_finish(&sha256_ctx, hash_output);
    mbedtls_sha256_free(&sha256_ctx);

    return ESP_OK;
}

esp_err_t get_current_firmware_info(firmware_info_t* info) {
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }

    return verify_app_partition_integrity(info);
}

esp_err_t validate_partition_size(const esp_partition_t* partition) {
    if (!partition) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!IS_VALID_FIRMWARE_SIZE(partition->size)) {
        ESP_LOGE(BOOTLOADER_TAG, "Tamaño de partición inválido: %lu bytes (rango: %d-%d)", 
                 partition->size, FIRMWARE_MIN_SIZE, FIRMWARE_MAX_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

esp_err_t clear_integrity_data(void) {
    if (!integrity_checker_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Limpiando datos de integridad");

    esp_err_t ret = nvs_erase_key(integrity_nvs_handle, NVS_KEY_APP_HASH);
    if (ret == ESP_OK || ret == ESP_ERR_NVS_NOT_FOUND) {
        ret = nvs_commit(integrity_nvs_handle);
    }

    if (ret == ESP_OK) {
        ESP_LOGI(BOOTLOADER_TAG, "Datos de integridad limpiados");
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "Error limpiando datos de integridad: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t hash_to_hex_string(const uint8_t* hash, char* hex_string) {
    if (!hash || !hex_string) {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 0; i < 32; i++) {
        byte_to_hex(hash[i], &hex_string[i * 2]);
    }
    hex_string[64] = '\0';

    return ESP_OK;
}

esp_err_t hex_string_to_hash(const char* hex_string, uint8_t* hash) {
    if (!hex_string || !hash) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t len = strlen(hex_string);
    if (len != 64) {
        return ESP_ERR_INVALID_SIZE;
    }

    for (int i = 0; i < 32; i++) {
        hash[i] = (hex_char_to_byte(hex_string[i * 2]) << 4) | 
                  hex_char_to_byte(hex_string[i * 2 + 1]);
    }

    return ESP_OK;
}

esp_err_t verify_firmware_header(const uint8_t* firmware_data, size_t size, firmware_header_t* header) {
    if (!firmware_data || !header || size < sizeof(firmware_header_t)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Copiar header
    memcpy(header, firmware_data, sizeof(firmware_header_t));

    // Verificar magic number
    if (!IS_VALID_FIRMWARE_MAGIC(header->magic)) {
        ESP_LOGE(BOOTLOADER_TAG, "Magic number inválido: 0x%08lX", header->magic);
        return ESP_ERR_INVALID_CRC;
    }

    // Verificar tamaño
    if (!IS_VALID_FIRMWARE_SIZE(header->size)) {
        ESP_LOGE(BOOTLOADER_TAG, "Tamaño de firmware inválido: %lu bytes", header->size);
        return ESP_ERR_INVALID_SIZE;
    }

    if (header->size > size) {
        ESP_LOGE(BOOTLOADER_TAG, "Tamaño declarado (%lu) mayor que datos disponibles (%zu)", 
                 header->size, size);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Header de firmware válido - versión: %lu, tamaño: %lu bytes", 
             header->version, header->size);

    return ESP_OK;
} 