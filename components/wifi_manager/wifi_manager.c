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

    // Configura zona horaria manualmente (Colombia UTC-5)
    setenv("TZ", "COT5", 1);
    tzset();

    // Configura SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "time.nist.gov");
    sntp_init();

    vTaskDelay(pdMS_TO_TICKS(2000));  // Espera para sincronizar

    // Obtiene hora local y la muestra
    time_t now;
    struct tm timeinfo;
    char datetime_str[64];

    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year > 70) {
        strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M", &timeinfo);
        if (datetime_label != NULL) {
            lv_label_set_text(datetime_label, datetime_str);
        }
        ESP_LOGI(TAG, "Hora actualizada: %s", datetime_str);
    } else {
        if (datetime_label != NULL) {
            lv_label_set_text(datetime_label, "Sin hora");
        }
        ESP_LOGW(TAG, "No se pudo sincronizar la hora.");
    }

    ESP_LOGI(TAG, "WiFi y hora configurados correctamente.");
    return ESP_OK;
} 