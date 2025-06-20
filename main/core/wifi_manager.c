/**
 * @file wifi_manager.c
 * @brief Implementación del gestor de WiFi
 */

#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "lwip/apps/sntp.h"
#include "lvgl.h"
#include "system_time.h"
#include <string.h>
#include <time.h>

static const char *TAG = "wifi_manager";

esp_err_t wifi_manager_init(lv_obj_t *dropdown, lv_obj_t *datetime_label) {
    ESP_LOGI(TAG, "Inicializando WiFi...");

    // Inicializa el almacenamiento no volátil
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Configura red a conectar
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Yahel2023",
            .password = "Yahel2023",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Conectado a Yahel2023");

    // Espera para permitir escaneo de redes
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Escaneo de redes y construcción de lista para el dropdown
    uint16_t ap_num = 0;
    wifi_ap_record_t ap_records[20];

    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

    static char options[512] = "";
    options[0] = '\0';

    for (int i = 0; i < ap_num && i < 20; i++) {
        strcat(options, (const char *)ap_records[i].ssid);
        if (i < ap_num - 1) strcat(options, "\n");
    }

    if (dropdown != NULL) {
        lv_dropdown_set_options(dropdown, options);
        ESP_LOGI(TAG, "Redes actualizadas en el dropdown.");
    }

    // Usar el nuevo sistema de tiempo para sincronización automática
    ESP_LOGI(TAG, "Sincronizando tiempo con la red usando el nuevo sistema...");
    system_time_update_from_network();
    
    // Iniciar timer de actualización automática
    system_time_start_auto_update();
    
    ESP_LOGI(TAG, "Sistema de tiempo integrado con WiFi completado");

    ESP_LOGI(TAG, "WiFi y hora configurados correctamente.");
    return ESP_OK;
} 