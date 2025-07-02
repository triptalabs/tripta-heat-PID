#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estructura que contiene las credenciales Wi-Fi.
 */
typedef struct {
    char ssid[32];
    char password[64];
} wifi_credentials_t;

/**
 * @brief Obtiene las credenciales Wi-Fi almacenadas en NVS.
 *
 * @param[out] cred  Estructura donde se copiarán las credenciales.
 * @return ESP_OK si se encontraron credenciales, ESP_ERR_NOT_FOUND si no existen.
 */
esp_err_t wifi_prov_get_credentials(wifi_credentials_t *cred);

/**
 * @brief Inicia el proceso de provisioning BLE para capturar SSID/clave.
 *        Implementación placeholder.
 */
esp_err_t wifi_prov_start_ble_provisioning(void);

#ifdef __cplusplus
}
#endif 