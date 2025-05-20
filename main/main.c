/**
 * @file main.c
 * @brief Punto de entrada principal del firmware para el horno de vacío con ESP32-S3.
 * 
 * Esta aplicación inicializa los periféricos principales del sistema, como la pantalla RGB,
 * la interfaz gráfica LVGL, el stack WiFi y la comunicación Modbus para lectura de temperatura.
 * También configura el reloj del sistema a través de SNTP y muestra la hora en la interfaz.
 * 
 * @version 1.0
 * @date 2024-01-27
 * 
 * @note La interfaz fue generada con SquareLine Studio.
 */

#include "waveshare_rgb_lcd_port.h"
#include "ui.h"
#include "sensor.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "DEV_Config.h"
#include "CH422G.h"
#include "pid_controller.h"
#include <string.h>
#include <time.h>
#include "esp_sntp.h"
#include "lvgl.h"
#include <sys/time.h>

#ifndef TAG
#define TAG "main" ///< Etiqueta de log para este módulo
#endif

/**
 * @brief Inicializa el stack de WiFi, escanea redes y sincroniza la hora vía SNTP.
 * 
 * Esta función:
 * - Inicializa NVS, WiFi y TCP/IP.
 * - Se conecta automáticamente a la red "Yahel2023".
 * - Escanea redes disponibles y las carga en un dropdown de LVGL.
 * - Configura la zona horaria y sincroniza la hora con servidores NTP.
 * - Muestra la hora local en la interfaz gráfica.
 */
void wifi_stack_init(void) {
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

    lv_dropdown_set_options(ui_Dropdown1, options);
    ESP_LOGI(TAG, "Redes actualizadas en el dropdown.");

    // Configura zona horaria manualmente (Colombia UTC-5)
    setenv("TZ", "COT5", 1);
    tzset();

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
        lv_label_set_text(cui_datetime1, datetime_str);
        ESP_LOGI(TAG, "Hora actualizada: %s", datetime_str);
    } else {
        lv_label_set_text(cui_datetime1, "Sin hora");
        ESP_LOGW(TAG, "No se pudo sincronizar la hora.");
    }

    ESP_LOGI(TAG, "WiFi y hora configurados correctamente.");
}

/**
 * @brief Función principal del firmware.
 * 
 * Realiza la inicialización de hardware y software:
 * - Configura el bus I2C.
 * - Inicializa la pantalla RGB y el backend de LVGL.
 * - Carga la interfaz gráfica exportada desde SquareLine Studio.
 * - Inicia el stack WiFi, sincronización de hora y actualización periódica del reloj.
 * - Inicia tareas de lectura de temperatura por Modbus y el controlador PID.
 */
void app_main(void)
{
    DEV_Module_Init();  // Inicializa I2C

    // Inicializa pantalla RGB y mutex de LVGL
    waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();

    if (lvgl_port_lock(-1)) {
        // Carga interfaz gráfica
        ui_init();

        // Configura WiFi y hora
        wifi_stack_init();

        // Crea temporizador para actualizar hora cada minuto
        lv_timer_create(actualizar_hora_cb, 60000, NULL);

        // Inicia tareas principales
        start_temperature_task();
        pid_controller_init(0.0f);

        lvgl_port_unlock();
    }

    // Nota: no se necesita un bucle explícito; LVGL corre en background.
}
