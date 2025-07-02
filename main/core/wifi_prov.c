#include "wifi_prov.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "wifi_prov";
#define NVS_NAMESPACE "wifi_cfg"

esp_err_t wifi_prov_get_credentials(wifi_credentials_t *cred)
{
    if (!cred) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No se pudo abrir NVS: %s", esp_err_to_name(err));
        return err;
    }

    size_t ssid_len = sizeof(cred->ssid);
    err = nvs_get_str(handle, "ssid", cred->ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return ESP_ERR_NOT_FOUND;
    }

    size_t pass_len = sizeof(cred->password);
    err = nvs_get_str(handle, "pass", cred->password, &pass_len);
    nvs_close(handle);
    if (err != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Credenciales leídas: SSID=%s", cred->ssid);
    return ESP_OK;
}

esp_err_t wifi_prov_start_ble_provisioning(void)
{
    // Placeholder: en implementación completa se integrará wifi_provisioning.
    ESP_LOGW(TAG, "Provisioning BLE no implementado (stub)");
    return ESP_ERR_NOT_SUPPORTED;
} 