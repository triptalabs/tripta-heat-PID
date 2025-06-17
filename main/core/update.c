/**
 * @file update.c
 * @brief Implementación del módulo de actualizaciones OTA para ESP32 con fallback desde microSD.
 *
 * Este módulo permite:
 *  - Verificar si hay una nueva versión disponible en un repositorio remoto.
 *  - Descargar la nueva versión a la microSD.
 *  - Flashear el firmware directamente desde la microSD (sin partición OTA).
 *  - Restaurar automáticamente el firmware base desde una copia de seguridad si algo falla.
 *
 * Requiere:
 *  - Sistema de archivos montado (ej: FAT para microSD).
 *  - Conexión WiFi activa antes de iniciar la descarga.
 *
 * @author Tripta
 * @date 2025
 */

#include "update.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_https_ota.h"
#include "esp_spiffs.h"
#include "esp_partition.h"
#include <string.h>
#include <stdio.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_vfs_fat.h"
#include "bootloader_main.h"
#include "integrity_checker.h"
#include "sd_recovery.h"
#include "mbedtls/sha256.h"

// Intentar incluir configuración secreta si existe
#ifdef __has_include
    #if __has_include("update_config.h")
        #include "update_config.h"
        #define HAVE_UPDATE_CONFIG 1
    #endif
#else
    // Fallback para compiladores que no soportan __has_include
    #include "update_config.h"
    #define HAVE_UPDATE_CONFIG 1
#endif

/**
 * @def TAG
 * @brief Etiqueta para los logs del módulo de actualización OTA.
 */
#define TAG "UPDATE"

/**
 * @def MOUNT_POINT
 * @brief Ruta del punto de montaje de la microSD en el sistema de archivos VFS.
 *
 * Este valor se usa para montar y acceder a archivos en la microSD desde el firmware.
 */
#define MOUNT_POINT "/sdcard/"

/**
 * @def FIRMWARE_VERSION
 * @brief Versión actual del firmware compilado que está corriendo en el dispositivo.
 *
 * Esta versión será comparada con la versión publicada en línea (version.json) para
 * determinar si hay una nueva actualización disponible.
 */
#define FIRMWARE_VERSION "1.0.0"

/**
 * @def VERSION_BUFFER_SIZE
 * @brief Tamaño del buffer para leer el archivo de versión
 */
#define VERSION_BUFFER_SIZE 128

/**
 * @var is_update_pending
 * @brief Indicador interno del módulo que señala si hay una actualización pendiente.
 *
 * Esta bandera es actualizada por la función update_check() y consultada por update_there_is_update().
 */
static bool is_update_pending = false;

/**
 * @var sd_mounted
 * @brief Estado del sistema de archivos de la microSD.
 *
 * Si es true, significa que la tarjeta SD ya fue montada exitosamente.
 * Se usa internamente para evitar montajes repetidos.
 */
static bool sd_mounted = false;

/**
 * @var current_config
 * @brief Configuración actual del módulo de actualización
 */
static update_config_t current_config = {
    .firmware_url = FIRMWARE_URL_DEFAULT,
    .version_url = VERSION_URL_DEFAULT,
    .version_check_timeout = VERSION_CHECK_TIMEOUT_DEFAULT,
    .download_timeout = DOWNLOAD_TIMEOUT_DEFAULT
};

/**
 * @brief Monta la tarjeta microSD si aún no ha sido montada.
 *
 * Esta función es interna y se llama automáticamente desde otras funciones.
 * Monta el sistema de archivos FAT en la ruta /sdcard.
 *
 * @return
 *      - ESP_OK: Montaje exitoso o ya montado
 *      - ESP_FAIL: Error al montar
 */
static esp_err_t mount_sdcard_if_needed(void) {
    if (sd_mounted) return ESP_OK;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SD montada en '%s'", MOUNT_POINT);
        sd_mounted = true;
    } else {
        ESP_LOGE(TAG, "Error montando SD: %s", esp_err_to_name(ret));
    }
    return ret;
}

/**
 * @brief Inicializa el sistema de actualización OTA.
 *
 * Esta función realiza las siguientes acciones:
 *  - Resetea el indicador de actualización pendiente.
 *  - Llama a update_check() para verificar si hay actualizaciones disponibles.
 *  - Actualiza el estado de is_update_pending según el resultado.
 *
 * @return
 *      - ESP_OK: Inicialización exitosa.
 */
esp_err_t update_init(void) {
    ESP_LOGI(TAG, "Inicializando módulo de actualización OTA...");
    
    // Inicializar configuración por defecto
    current_config.firmware_url = FIRMWARE_URL_DEFAULT;
    current_config.version_url = VERSION_URL_DEFAULT;
    current_config.version_check_timeout = VERSION_CHECK_TIMEOUT_DEFAULT;
    current_config.download_timeout = DOWNLOAD_TIMEOUT_DEFAULT;
    
    // Reset por defecto
    is_update_pending = false;
    
    // Intentar verificar si hay actualización
    bool update_flag = false;
    esp_err_t err = update_check(&update_flag);
    if (err == ESP_OK) {
        is_update_pending = update_flag;
    } else {
        ESP_LOGW(TAG, "No se pudo verificar actualizaciones: %s", esp_err_to_name(err));
        // Mantiene is_update_pending = false
    }
    return ESP_OK;
}

/**
 * @brief Verifica si hay una actualización disponible comparando versiones.
 *
 * Realiza una solicitud HTTP a un archivo JSON en el servidor que contiene la
 * última versión disponible. Compara contra la versión local.
 *
 * @param[out] update_available true si hay una nueva versión, false si no.
 * @return
 *      - ESP_OK: Verificación exitosa
 *      - ESP_FAIL: Error de red, de lectura o de formato
 *      - ESP_ERR_INVALID_ARG: Si el puntero update_available es nulo
 */
esp_err_t update_check(bool *update_available) {
    // Verificar que el puntero de salida no sea nulo
    if (update_available == NULL) {
        ESP_LOGE(TAG, "update_check: Puntero nulo");
        return ESP_ERR_INVALID_ARG;
    }
    // Inicializar variables de estado
    *update_available = false; // Inicializa la variable de salida
    is_update_pending = false;

    // Configurar cliente HTTP para obtener la versión
    esp_http_client_config_t config = {
        .url = current_config.version_url,
        .timeout_ms = current_config.version_check_timeout,
    };

    // Inicializar cliente HTTP
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "No se pudo inicializar cliente HTTP");
        return ESP_FAIL;
    }

    // Abrir conexión HTTP
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al abrir conexión HTTP: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    // Leer respuesta del servidor
    char buffer[VERSION_BUFFER_SIZE] = {0};
    int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
    esp_http_client_cleanup(client);
    if (len <= 0) {
        ESP_LOGE(TAG, "No se pudo leer versión remota");
        return ESP_FAIL;
    }
    buffer[len] = '\0';

    // Buscar y extraer la versión del JSON
    const char *version_key = "\"version\":";
    char *start = strstr(buffer, version_key);
    if (!start) {
        ESP_LOGE(TAG, "No se encontró campo 'version' en JSON");
        return ESP_FAIL;
    }
    start += strlen(version_key);

    // Limpiar espacios y comillas iniciales
    while (*start && (*start == ' ' || *start == '\"')) start++;
    
    // Extraer la versión remota
    char remote_version[16] = {0};
    int i = 0;
    while (*start && *start != '\"' && *start != '\n' && i < sizeof(remote_version) - 1) {
        remote_version[i++] = *start++;
    }
    remote_version[i] = '\0';

    // Comparar versiones y actualizar estado
    ESP_LOGI(TAG, "Versión remota: %s | Versión local: %s", remote_version, FIRMWARE_VERSION);
    if (strcmp(remote_version, FIRMWARE_VERSION) != 0) {
        *update_available = true;
        is_update_pending = true;
        ESP_LOGI(TAG, "Actualización disponible");
    } else {
        ESP_LOGI(TAG, "Firmware ya actualizado");
    }
    return ESP_OK;
}

/**
 * @brief Descarga el firmware desde una URL fija a la microSD.
 *
 * Esta función:
 *  - Monta automáticamente la microSD (solo una vez).
 *  - Descarga el firmware desde la URL definida en FIRMWARE_URL.
 *  - Lo guarda en la ruta `local_path` (dentro de la microSD).
 *
 * Requiere que la tarjeta SD esté conectada físicamente y tenga sistema de archivos FAT válido.
 *
 * @param[in] local_path Ruta completa en la microSD donde se guardará el archivo .bin (ej: "/sdcard/update.bin")
 * @return
 *      - ESP_OK: Descarga exitosa
 *      - ESP_FAIL: Fallo al montar SD, abrir archivo, o descargar
 *      - ESP_ERR_INVALID_ARG: Si la ruta es nula
 */
esp_err_t update_download_firmware(const char *local_path) {
    // Verificar que la ruta de destino sea válida
    if (!local_path) {
        ESP_LOGE(TAG, "Ruta local nula para descarga");
        return ESP_ERR_INVALID_ARG;
    }

    // Montar la tarjeta SD si no está montada
    esp_err_t err = mount_sdcard_if_needed();
    if (err != ESP_OK) return err;

    // Iniciar descarga desde la URL configurada
    ESP_LOGI(TAG, "Descargando firmware desde URL: %s", current_config.firmware_url);

    // Configurar cliente HTTP con parámetros de descarga
    esp_http_client_config_t config = {
        .url = current_config.firmware_url,        // URL del firmware
        .timeout_ms = current_config.download_timeout,  // Timeout configurado
        .buffer_size = 4096,                      // Buffer de lectura
        .keep_alive_enable = true,                // Mantener conexión activa
    };

    // Inicializar cliente HTTP
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "No se pudo inicializar cliente HTTP");
        return ESP_FAIL;
    }

    // Abrir conexión HTTP
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al abrir conexión HTTP: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    // Abrir archivo en SD para escritura
    FILE *f = fopen(local_path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "No se pudo abrir archivo en SD: %s", local_path);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Descargar datos en bloques y escribir a archivo
    int total_read = 0;
    int data_read;
    char buffer[4096];
    while ((data_read = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
        // Escribir bloque descargado al archivo
        if (fwrite(buffer, 1, data_read, f) != data_read) {
            ESP_LOGE(TAG, "Error al escribir en archivo");
            fclose(f);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
        total_read += data_read;
    }

    // Cerrar archivo y limpiar cliente HTTP
    fclose(f);
    esp_http_client_cleanup(client);

    // Verificar que se descargó al menos un byte
    if (total_read == 0) {
        ESP_LOGW(TAG, "No se descargó ningún dato");
        return ESP_FAIL;
    }

    // Reportar éxito y tamaño descargado
    ESP_LOGI(TAG, "Descarga completada: %d bytes", total_read);
    return ESP_OK;
}

/**
 * @brief Flashea el firmware desde un archivo en la microSD.
 *
 * Esta función lee el archivo especificado, lo escribe en la partición OTA activa
 * y establece la partición como la nueva partición de arranque.
 *
 * @param[in] firmware_path Ruta completa del archivo .bin en la microSD.
 * @return
 *      - ESP_OK: Flasheo exitoso.
 *      - ESP_FAIL: Error durante el proceso de flasheo.
 *      - ESP_ERR_INVALID_ARG: Si la ruta es nula.
 */
static esp_err_t flash_from_file(const char *firmware_path) {
    // Verificar que la ruta del firmware sea válida
    if (!firmware_path) {
        ESP_LOGE(TAG, "Ruta nula recibida en flash_from_file");
        return ESP_ERR_INVALID_ARG;
    }

    // Abrir archivo de firmware
    ESP_LOGI(TAG, "Flasheando desde archivo: %s", firmware_path);
    FILE *f = fopen(firmware_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "No se pudo abrir el archivo de firmware");
        return ESP_FAIL;
    }

    // Obtener partición OTA activa
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running) {
        ESP_LOGE(TAG, "No se encontró partición activa");
        fclose(f);
        return ESP_FAIL;
    }

    // Iniciar sesión OTA
    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(running, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo esp_ota_begin: %s", esp_err_to_name(err));
        fclose(f);
        return err;
    }

    // Leer y escribir firmware en bloques
    char buffer[4096];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        err = esp_ota_write(ota_handle, buffer, read_bytes);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error escribiendo OTA: %s", esp_err_to_name(err));
            esp_ota_end(ota_handle);
            fclose(f);
            return err;
        }
    }
    fclose(f);

    // Finalizar sesión OTA
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo esp_ota_end: %s", esp_err_to_name(err));
        return err;
    }

    // Establecer nueva partición de arranque
    err = esp_ota_set_boot_partition(running);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al establecer nueva partición: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Firmware flasheado correctamente.");
    return ESP_OK;
}

/**
 * @brief Realiza la actualización del firmware desde la microSD.
 *
 * Esta función:
 *  - Descarga el nuevo firmware desde el servidor.
 *  - Intenta flashear el firmware descargado.
 *  - En caso de fallo, restaura el firmware desde el archivo de respaldo.
 *
 * @param[in] firmware_path Ruta completa del archivo .bin descargado en la microSD.
 * @param[in] fallback_path Ruta completa del archivo .bin de respaldo en la microSD.
 * @return
 *      - ESP_OK: Actualización exitosa.
 *      - ESP_FAIL: Error durante la actualización o restauración.
 *      - ESP_ERR_INVALID_ARG: Si alguna ruta es nula.
 *      - ESP_ERR_INVALID_STATE: Si no hay actualización pendiente.
 */
esp_err_t update_perform(const char *firmware_path, const char *fallback_path) {
    // Verificar que los parámetros de entrada no sean nulos
    if (!firmware_path || !fallback_path) {
        ESP_LOGE(TAG, "Parámetros nulos en update_perform");
        return ESP_ERR_INVALID_ARG;
    }

    // Verificar que haya una actualización pendiente antes de proceder
    if (!is_update_pending) {
        ESP_LOGW(TAG, "No hay actualización pendiente. Abortando flasheo.");
        return ESP_ERR_INVALID_STATE;
    }

    // Paso 1: Descargar el nuevo firmware desde el servidor
    ESP_LOGI(TAG, "Descargando nuevo firmware a: %s", firmware_path);
    esp_err_t err = update_download_firmware(firmware_path);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al descargar el nuevo firmware");
        return err;
    }

    // Paso 2: Intentar flashear el firmware descargado
    ESP_LOGI(TAG, "Intentando flashear firmware descargado...");
    err = flash_from_file(firmware_path);
    if (err != ESP_OK) {
        // Paso 3: Si falla, intentar restaurar desde el backup
        ESP_LOGE(TAG, "Falló la actualización. Intentando restaurar desde backup...");
        err = flash_from_file(fallback_path);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Fallo también la restauración desde el firmware base.");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Restauración completada exitosamente.");
    }

    // Limpiar bandera de actualización pendiente
    is_update_pending = false;
    
    // Generar hash del nuevo firmware antes de reiniciar
    ESP_LOGI(TAG, "Generando hash de integridad para el nuevo firmware...");
    update_generate_integrity_hash();
    
    // Reiniciar el dispositivo para aplicar los cambios
    ESP_LOGI(TAG, "Reiniciando sistema para aplicar nueva actualización...");
    esp_restart();  // Reinicio tras flasheo exitoso
    return ESP_OK;  // No se alcanza, pero queda por consistencia
}

/**
 * @brief Consulta si hay una actualización pendiente detectada.
 *
 * @return true si hay una actualización pendiente, false en caso contrario.
 */
bool update_there_is_update(void) {
    return is_update_pending;
}

/**
 * @brief Limpia la bandera interna de actualización pendiente.
 *
 * Esta función puede ser llamada manualmente para reiniciar el estado de actualización.
 */
void update_clear_flag(void) {
    is_update_pending = false;
    ESP_LOGI(TAG, "Estado de actualización reiniciado manualmente");
}

/**
 * @brief Configura los parámetros de actualización.
 *
 * @param[in] config Estructura de configuración con URLs y timeouts.
 * @return ESP_OK en caso de éxito, ESP_ERR_INVALID_ARG si la configuración es inválida.
 */
esp_err_t update_set_config(const update_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!config->firmware_url || !config->version_url) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->version_check_timeout <= 0 || config->download_timeout <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    current_config = *config;
    return ESP_OK;
}

/**
 * @brief Genera hash SHA256 del firmware actual y lo almacena para verificación de integridad.
 */
esp_err_t update_generate_integrity_hash(void) {
    ESP_LOGI(TAG, "Generando hash de integridad del firmware actual...");
    
    const esp_partition_t* app_partition = esp_ota_get_running_partition();
    if (!app_partition) {
        ESP_LOGE(TAG, "No se pudo obtener partición de aplicación");
        return ESP_FAIL;
    }
    
    uint8_t firmware_hash[32];
    esp_err_t ret = calculate_partition_sha256(app_partition, firmware_hash);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error calculando hash del firmware: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = store_firmware_hash(firmware_hash);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error almacenando hash del firmware: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✅ Hash de integridad generado y almacenado exitosamente");
    return ESP_OK;
}

/**
 * @brief Prepara archivos de recovery en la SD para el bootloader.
 */
esp_err_t update_prepare_recovery_files(void) {
    ESP_LOGI(TAG, "Preparando archivos de recovery en SD...");
    
    esp_err_t ret = mount_sdcard_if_needed();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error montando SD para recovery");
        return ret;
    }
    
    ret = create_recovery_directory();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creando directorio de recovery");
        return ret;
    }
    
    // Obtener partición actual
    const esp_partition_t* app_partition = esp_ota_get_running_partition();
    if (!app_partition) {
        ESP_LOGE(TAG, "No se pudo obtener partición de aplicación");
        return ESP_FAIL;
    }
    
    // Leer firmware actual y escribirlo en SD
    const char* recovery_firmware_path = "/sdcard/recovery/base_firmware.bin";
    const char* recovery_hash_path = "/sdcard/recovery/base_firmware.bin.sha256";
    
    FILE* recovery_file = fopen(recovery_firmware_path, "wb");
    if (!recovery_file) {
        ESP_LOGE(TAG, "Error creando archivo de recovery");
        return ESP_FAIL;
    }
    
    // Leer partición y escribir a archivo
    uint8_t* buffer = malloc(4096);
    if (!buffer) {
        fclose(recovery_file);
        return ESP_ERR_NO_MEM;
    }
    
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0);
    
    size_t offset = 0;
    size_t total_written = 0;
    
    while (offset < app_partition->size) {
        size_t read_size = (app_partition->size - offset > 4096) ? 4096 : (app_partition->size - offset);
        
        ret = esp_partition_read(app_partition, offset, buffer, read_size);
        if (ret != ESP_OK) {
            break;
        }
        
        // Escribir a archivo
        if (fwrite(buffer, 1, read_size, recovery_file) != read_size) {
            ret = ESP_FAIL;
            break;
        }
        
        // Actualizar hash
        mbedtls_sha256_update(&sha256_ctx, buffer, read_size);
        
        offset += read_size;
        total_written += read_size;
    }
    
    fclose(recovery_file);
    
    if (ret == ESP_OK) {
        // Finalizar hash y escribir archivo
        uint8_t firmware_hash[32];
        mbedtls_sha256_finish(&sha256_ctx, firmware_hash);
        
        ret = write_hash_file_to_sd(recovery_hash_path, firmware_hash);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✅ Archivos de recovery preparados (%.1f MB)", 
                     (float)total_written / (1024 * 1024));
        }
    }
    
    free(buffer);
    mbedtls_sha256_free(&sha256_ctx);
    
    return ret;
}

/**
 * @brief Verifica la integridad del firmware actual usando hash SHA256.
 */
esp_err_t update_verify_firmware_integrity(bool *integrity_ok) {
    if (!integrity_ok) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Verificando integridad del firmware actual...");
    
    firmware_info_t firmware_info;
    esp_err_t ret = verify_app_partition_integrity(&firmware_info);
    *integrity_ok = (ret == ESP_OK);
    
    if (*integrity_ok) {
        ESP_LOGI(TAG, "✅ Integridad del firmware verificada exitosamente");
    } else {
        ESP_LOGE(TAG, "❌ Fallo en la verificación de integridad: %s", esp_err_to_name(ret));
    }
    
    return ESP_OK;
}