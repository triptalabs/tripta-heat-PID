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

/**
 * @def TAG
 * @brief Etiqueta para los logs del módulo de actualización OTA.
 */
#define TAG "UPDATE"

/**
 * @def FIRMWARE_URL
 * @brief URL fija del firmware más reciente (.bin) alojado en GitHub.
 *
 * Este archivo será descargado por el ESP32 y almacenado en la microSD para luego
 * ser flasheado. Se espera que el archivo tenga el formato compilado binario (.bin)
 * generado por PlatformIO o ESP-IDF.
 */
#define FIRMWARE_URL "https://github.com/triptalabs/firmware-vacuum-oven/releases/latest/download/lvgl_porting.bin "

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
 * @def VERSION_URL
 * @brief URL del archivo JSON que contiene la versión más reciente disponible del firmware.
 *
 * Se espera que el contenido del archivo tenga el siguiente formato:
 * @code
 * { "version": "1.0.1" }
 * @endcode
 */
#define VERSION_URL "https://github.com/triptalabs/firmware-vacuum-oven/releases/latest/download/version.json "

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
    if (update_available == NULL) {
        ESP_LOGE(TAG, "update_check: Puntero nulo");
        return ESP_ERR_INVALID_ARG;
    }
    *update_available = false;
    is_update_pending = false;

    esp_http_client_config_t config = {
        .url = current_config.version_url,
        .timeout_ms = current_config.version_check_timeout,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "No se pudo inicializar cliente HTTP");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al abrir conexión HTTP: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    char buffer[VERSION_BUFFER_SIZE] = {0};
    int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
    esp_http_client_cleanup(client);
    if (len <= 0) {
        ESP_LOGE(TAG, "No se pudo leer versión remota");
        return ESP_FAIL;
    }
    buffer[len] = '\0';

    const char *version_key = "\"version\":";
    char *start = strstr(buffer, version_key);
    if (!start) {
        ESP_LOGE(TAG, "No se encontró campo 'version' en JSON");
        return ESP_FAIL;
    }
    start += strlen(version_key);

    while (*start && (*start == ' ' || *start == '\"')) start++;
    char remote_version[16] = {0};
    int i = 0;
    while (*start && *start != '\"' && *start != '\n' && i < sizeof(remote_version) - 1) {
        remote_version[i++] = *start++;
    }
    remote_version[i] = '\0';

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
    if (!local_path) {
        ESP_LOGE(TAG, "Ruta local nula para descarga");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = mount_sdcard_if_needed();
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Descargando firmware desde URL: %s", current_config.firmware_url);

    esp_http_client_config_t config = {
        .url = current_config.firmware_url,
        .timeout_ms = current_config.download_timeout,
        .buffer_size = 4096,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "No se pudo inicializar cliente HTTP");
        return ESP_FAIL;
    }

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al abrir conexión HTTP: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    FILE *f = fopen(local_path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "No se pudo abrir archivo en SD: %s", local_path);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    int total_read = 0;
    int data_read;
    char buffer[4096];
    while ((data_read = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, data_read, f) != data_read) {
            ESP_LOGE(TAG, "Error al escribir en archivo");
            fclose(f);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
        total_read += data_read;
    }
    fclose(f);
    esp_http_client_cleanup(client);

    if (total_read == 0) {
        ESP_LOGW(TAG, "No se descargó ningún dato");
        return ESP_FAIL;
    }

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
    if (!firmware_path) {
        ESP_LOGE(TAG, "Ruta nula recibida en flash_from_file");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Flasheando desde archivo: %s", firmware_path);
    FILE *f = fopen(firmware_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "No se pudo abrir el archivo de firmware");
        return ESP_FAIL;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running) {
        ESP_LOGE(TAG, "No se encontró partición activa");
        fclose(f);
        return ESP_FAIL;
    }

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(running, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo esp_ota_begin: %s", esp_err_to_name(err));
        fclose(f);
        return err;
    }

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

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo esp_ota_end: %s", esp_err_to_name(err));
        return err;
    }

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
    if (!firmware_path || !fallback_path) {
        ESP_LOGE(TAG, "Parámetros nulos en update_perform");
        return ESP_ERR_INVALID_ARG;
    }

    if (!is_update_pending) {
        ESP_LOGW(TAG, "No hay actualización pendiente. Abortando flasheo.");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Descargando nuevo firmware a: %s", firmware_path);
    esp_err_t err = update_download_firmware(firmware_path);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al descargar el nuevo firmware");
        return err;
    }

    ESP_LOGI(TAG, "Intentando flashear firmware descargado...");
    err = flash_from_file(firmware_path);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falló la actualización. Intentando restaurar desde backup...");
        err = flash_from_file(fallback_path);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Fallo también la restauración desde el firmware base.");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Restauración completada exitosamente.");
    }

    is_update_pending = false;
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