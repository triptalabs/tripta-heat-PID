/**
 * @file update.h
 * @brief Módulo para manejo de actualizaciones OTA desde repositorio GitHub usando microSD.
 *
 * Este módulo permite:
 * - Verificar si hay una versión más reciente del firmware.
 * - Descargar el firmware desde GitHub a la microSD.
 * - Flashear el ESP32 desde ese archivo sin usar particiones OTA.
 * - Restaurar una copia de respaldo si la actualización falla.
 *
 * La microSD se monta automáticamente si no está montada al llamar las funciones.
 * El archivo version.json en GitHub debe tener el formato:
 * @code
 * { "version": "1.0.1" }
 * @endcode
 *
 * @note Este módulo requiere:
 * - Una tarjeta microSD formateada con sistema de archivos FAT.
 * - Conexión WiFi activa antes de iniciar la descarga.
 *
 * @author Tripta
 * @date 2025
 */

#ifndef UPDATE_H
#define UPDATE_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @def FIRMWARE_URL_DEFAULT
 * @brief URL del firmware más reciente (.bin) - Definida en archivo de configuración.
 */
#ifdef SECRET_FIRMWARE_URL
#define FIRMWARE_URL_DEFAULT SECRET_FIRMWARE_URL
#else
#error "FIRMWARE_URL_DEFAULT no definida. Crear update_config.h con SECRET_FIRMWARE_URL"
#endif

/**
 * @def VERSION_URL_DEFAULT
 * @brief URL del archivo JSON de versión - Definida en archivo de configuración.
 */
#ifdef SECRET_VERSION_URL
#define VERSION_URL_DEFAULT SECRET_VERSION_URL
#else
#error "VERSION_URL_DEFAULT no definida. Crear update_config.h con SECRET_VERSION_URL"
#endif

/**
 * @def VERSION_CHECK_TIMEOUT_DEFAULT
 * @brief Timeout por defecto para verificación de versión (ms)
 */
#define VERSION_CHECK_TIMEOUT_DEFAULT 15000

/**
 * @def DOWNLOAD_TIMEOUT_DEFAULT
 * @brief Timeout por defecto para descarga de firmware (ms)
 */
#define DOWNLOAD_TIMEOUT_DEFAULT 300000

/**
 * @struct update_config_t
 * @brief Estructura de configuración para el módulo de actualización
 */
typedef struct {
    const char *firmware_url;    ///< URL del firmware más reciente
    const char *version_url;     ///< URL del archivo de versión
    int version_check_timeout;   ///< Timeout para verificación de versión (ms)
    int download_timeout;        ///< Timeout para descarga de firmware (ms)
} update_config_t;



/**
 * @brief Inicializa el módulo de actualización OTA.
 *
 * Establece el estado inicial del sistema de actualización. Debe ser llamado una sola vez
 * al iniciar el sistema, generalmente desde `app_main()`.
 *
 * @return
 * - ESP_OK: Inicialización exitosa.
 * - ESP_FAIL: Fallo interno.
 */
esp_err_t update_init(void);

/**
 * @brief Verifica si hay una actualización de firmware disponible.
 *
 * Compara la versión local con la remota contenida en el archivo `version.json` en GitHub.
 *
 * @param[out] update_available Puntero que será true si hay nueva versión.
 * @return
 * - ESP_OK: Verificación exitosa.
 * - ESP_FAIL: Error de red, formato inválido o no se pudo acceder.
 * - ESP_ERR_INVALID_ARG: Puntero nulo.
 */
esp_err_t update_check(bool *update_available);

/**
 * @brief Descarga el firmware más reciente desde GitHub a la microSD.
 *
 * La descarga se realiza desde una URL fija (definida en `FIRMWARE_URL`) y se guarda
 * en la ruta especificada en `local_path`. La SD se monta automáticamente si no lo está.
 *
 * @param[in] local_path Ruta completa en la microSD donde se guardará el firmware (.bin).
 * @return
 * - ESP_OK: Descarga exitosa.
 * - ESP_FAIL: Error de red, montaje o escritura.
 * - ESP_ERR_INVALID_ARG: Ruta nula.
 */
esp_err_t update_download_firmware(const char *local_path);

/**
 * @brief Flashea el firmware desde un archivo en la microSD.
 *
 * Intenta flashear el firmware desde `firmware_path`. Si falla, intenta restaurar
 * automáticamente el firmware original desde `fallback_path`. En caso de éxito,
 * reinicia el dispositivo para aplicar los cambios.
 *
 * Flujo de ejecución:
 * 1. Intenta flashear `firmware_path`.
 * 2. Si falla, intenta flashear `fallback_path`.
 * 3. Si ambos fallan, devuelve ESP_FAIL.
 * 4. Si el flasheo es exitoso, reinicia el dispositivo.
 *
 * @param[in] firmware_path Ruta del nuevo firmware descargado.
 * @param[in] fallback_path Ruta de la copia de respaldo del firmware original.
 * @return
 * - ESP_OK: Flasheo exitoso y reinicio iniciado.
 * - ESP_FAIL: Fallo al flashear tanto el firmware nuevo como el respaldo.
 * - ESP_ERR_INVALID_ARG: Rutas inválidas o nulas.
 */
esp_err_t update_perform(const char *firmware_path, const char *fallback_path);

/**
 * @brief Consulta si hay una actualización pendiente detectada.
 *
 * Este valor se establece por la última llamada a update_check().
 * Debe llamarse después de update_check() para obtener resultados válidos.
 *
 * @return true si hay una actualización disponible, false si no.
 */
bool update_there_is_update(void);

/**
 * @brief Limpia la bandera de actualización pendiente.
 *
 * Se puede usar si el usuario cancela el proceso o tras una actualización manual.
 */
void update_clear_flag(void);

/**
 * @brief Genera hash SHA256 del firmware actual y lo almacena para verificación de integridad.
 *
 * Esta función se debe llamar después de una actualización exitosa para que
 * el bootloader pueda verificar la integridad en el próximo arranque.
 *
 * @return
 * - ESP_OK: Hash generado y almacenado exitosamente.
 * - ESP_FAIL: Error generando o almacenando el hash.
 */
esp_err_t update_generate_integrity_hash(void);

/**
 * @brief Prepara archivos de recovery en la SD para el bootloader.
 *
 * Copia el firmware actual como base_firmware.bin y genera su hash SHA256
 * para que el bootloader pueda usarlo como firmware de recuperación.
 *
 * @return
 * - ESP_OK: Archivos de recovery preparados exitosamente.
 * - ESP_FAIL: Error preparando archivos de recovery.
 */
esp_err_t update_prepare_recovery_files(void);

/**
 * @brief Verifica la integridad del firmware actual usando hash SHA256.
 *
 * @param[out] integrity_ok Puntero que será true si la integridad es correcta.
 * @return
 * - ESP_OK: Verificación completada.
 * - ESP_FAIL: Error durante la verificación.
 * - ESP_ERR_INVALID_ARG: Puntero nulo.
 */
esp_err_t update_verify_firmware_integrity(bool *integrity_ok);

#ifdef __cplusplus
}
#endif

#endif // UPDATE_H