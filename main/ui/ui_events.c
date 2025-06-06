// Este archivo fue generado por SquareLine Studio
// Versión: 1.5.1 - LVGL 8.3.11
// Proyecto: UI_draft

#include "ui.h"
#include "ui_helpers.h"
#include "ui_comp.h"
#include "ui_comp_hook.h"
#include "ui_events.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_err.h"
#include "update.h"
#include <math.h>
#include "driver/gpio.h"
#include "CH422G.h"
#include <sys/time.h>
#include <time.h>
#include "pid_controller.h"

#define CH422_CMD_SET_PARAM   0x48
#define CH422_PUSH_PULL_MODE  0x00  // IOs como entrada, OCx como push-pull


#define CHART_POINT_COUNT 240

extern lv_obj_t * ui_LabelEditWifiStatus;
extern lv_obj_t * ui_Dropdown1;
extern lv_obj_t * ui_LabelWifiPass;
extern lv_obj_t * ui_nombrebt;
extern lv_obj_t * ui_TextAreaKp;
extern lv_obj_t * ui_TextAreaKi;
extern lv_obj_t * ui_TextAreaKd;
extern lv_obj_t * ui_ArcSetTime;
extern lv_obj_t * ui_ArcSetTemp;
extern lv_obj_t * ui_RollerAnio;
extern lv_obj_t * ui_RollerMes;
extern lv_obj_t * ui_RollerDia;
extern lv_obj_t * ui_RollerHora;
extern lv_obj_t * ui_RollerMinuto;
extern lv_obj_t * cui_datetime1;

static const char *EVENTS_TAG = "Events";

static int anio = 2024;
static int mes = 1;
static int dia = 1;
static int hora = 0;
static int minuto = 0;


// Timer LVGL
static lv_timer_t *timer_minutos = NULL;
static int minutos_restantes = 0;



void EncenderWifi(lv_event_t * e)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
    if (mode == WIFI_MODE_STA) {
        ESP_LOGI(EVENTS_TAG, "Wi-Fi ya está encendido en modo estación.");
        lv_label_set_text(ui_LabelEditWifiStatus, "Wi-Fi ya está encendido en modo estación.");
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(EVENTS_TAG, "Wi-Fi encendido y en modo estación.");
    lv_label_set_text(ui_LabelEditWifiStatus, "Wi-Fi encendido y en modo estación.");
}

void ApagarWifi(lv_event_t * e)
{
    wifi_mode_t mode;
    esp_err_t ret = esp_wifi_get_mode(&mode);
    if (ret != ESP_OK || mode == WIFI_MODE_NULL) {
        ESP_LOGI(EVENTS_TAG, "Wi-Fi ya está apagado.");
        lv_label_set_text(ui_LabelEditWifiStatus, "Wi-Fi ya está apagado.");
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    ESP_LOGI(EVENTS_TAG, "Wi-Fi apagado.");
    lv_label_set_text(ui_LabelEditWifiStatus, "Wi-Fi apagado.");
}

void EncenderBt(lv_event_t * e) {
}

void ApagarBt(lv_event_t * e) {
    // TODO: Implementar
}


void EncenderPID(lv_event_t *e) {
    float setpoint = lv_arc_get_value(ui_ArcSetTemp);  // Obtiene el setpoint desde la UI
    pid_set_setpoint(setpoint);    
    pid_enable();                    // Lo pasa al controlador                                     // Activa el PID
    printf("PID habilitado desde GUI (Setpoint = %.2f°C)\n", setpoint);
}

void ApagarPID(lv_event_t *e) {
    pid_disable();          // Desactiva la lógica PID
    desactivar_ssr();       // 💥 Apaga físicamente el relé (¡clave!)
    printf("PID deshabilitado desde GUI\n");
}


// Callback del temporizador LVGL
static void timer_callback(lv_timer_t *t) {
    if (--minutos_restantes <= 0) {
        ApagarPID(NULL);
        lv_arc_set_value(ui_ArcSetTime, 0);
        lv_timer_del(timer_minutos);
        timer_minutos = NULL;
        ESP_LOGI(EVENTS_TAG, "Temporizador completado.");
    } else {
        lv_arc_set_value(ui_ArcSetTime, minutos_restantes);
        ESP_LOGI(EVENTS_TAG, "Minutos restantes: %d", minutos_restantes);
    }
}

void EncenderTimer(lv_event_t *e) {
    if (timer_minutos != NULL) {
        ESP_LOGW(EVENTS_TAG, "Temporizador ya en ejecución.");
        return;
    }

    minutos_restantes = lv_arc_get_value(ui_ArcSetTime);
    if (minutos_restantes <= 0) {
        ESP_LOGW(EVENTS_TAG, "El temporizador no puede iniciarse con un valor de 0 o menor.");
        return;
    }

    timer_minutos = lv_timer_create(timer_callback, 60000, NULL);  // 60,000 ms = 1 minuto
    ESP_LOGI(EVENTS_TAG, "Temporizador iniciado con %d minutos.", minutos_restantes);
}

void ApagarTimer(lv_event_t *e) {
    if (timer_minutos != NULL) {
        lv_timer_del(timer_minutos);
        timer_minutos = NULL;
        ESP_LOGI(EVENTS_TAG, "Temporizador detenido manualmente.");
    }

    minutos_restantes = 0;
    lv_arc_set_value(ui_ArcSetTime, 0);
}

void CambiarNombreBT(lv_event_t *e)
{
    const char *new_bt_name = lv_textarea_get_text(ui_nombrebt);
    if (!new_bt_name || strlen(new_bt_name) == 0) {
        ESP_LOGW(EVENTS_TAG, "No se proporcionó un nombre válido para Bluetooth.");
        return;
    }

    // Actualizar el nombre del dispositivo Bluetooth
    esp_err_t ret = esp_bt_dev_set_device_name(new_bt_name);
    if (ret == ESP_OK) {
        ESP_LOGI(EVENTS_TAG, "Nombre Bluetooth actualizado a: %s", new_bt_name);
    } else {
        ESP_LOGE(EVENTS_TAG, "Error al cambiar el nombre Bluetooth: %s", esp_err_to_name(ret));
    }
}

void CambiarFechaHora(lv_event_t * e) {
    int anio   = lv_roller_get_selected(ui_RollerAnio) + 2025;
    int mes    = lv_roller_get_selected(ui_RollerMes);     // 0–11
    int dia    = lv_roller_get_selected(ui_RollerDia) + 1;
    int hora   = lv_roller_get_selected(ui_RollerHora);
    int minuto = lv_roller_get_selected(ui_RollerMinuto);

    // Log para consola
    ESP_LOGI("FECHA_HORA", "Nueva fecha y hora: %04d-%02d-%02d %02d:%02d", anio, mes + 1, dia, hora, minuto);

    // Crear estructura de tiempo
    struct tm t = {
        .tm_year = anio - 1900,
        .tm_mon  = mes,       // 0–11
        .tm_mday = dia,
        .tm_hour = hora,
        .tm_min  = minuto,
        .tm_sec  = 0,
    };

    time_t epoch = mktime(&t);
    struct timeval now = { .tv_sec = epoch };
    settimeofday(&now, NULL);  // 🕐 Aquí se actualiza el reloj del sistema

    // También actualiza la barra de estado
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d  %H:%M", &t);
    lv_label_set_text(cui_datetime1, buffer);
}


void UpdateFirmware(lv_event_t * e) {
    if (update_there_is_update()) {
        ESP_LOGI(EVENTS_TAG, "Actualización pendiente: iniciando actualización OTA...");
        esp_err_t ret = update_perform("/sdcard/update.bin", "/sdcard/firmware_backup.bin");
        if (ret != ESP_OK) {
            ESP_LOGE(EVENTS_TAG, "Error en la actualización OTA: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGI(EVENTS_TAG, "No hay actualizaciones pendientes.");
    }
}

void TryWifiConn(lv_event_t * e) {
    char ssid[64] = {0};
    lv_dropdown_get_selected_str(ui_Dropdown1, ssid, sizeof(ssid));
    const char *password = lv_textarea_get_text(ui_LabelWifiPass);

    ESP_LOGI(EVENTS_TAG, "Intentando conectar a WiFi: %s", ssid);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void ActualizarK(lv_event_t * e) {
    const char *kp_str = lv_textarea_get_text(ui_TextAreaKp);
    const char *ki_str = lv_textarea_get_text(ui_TextAreaKi);
    const char *kd_str = lv_textarea_get_text(ui_TextAreaKd);

    float kp = atof(kp_str);
    float ki = atof(ki_str);
    float kd = atof(kd_str);

    pid_set_params(kp, ki, kd);
}

void ui_actualizar_estado_pid(float temperatura, bool heating_on) {
    static char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.1f°C\nTemperatura", temperatura);
    lv_label_set_text(ui_editLabelGetStatus, buffer);
}

void actualizar_hora_cb(lv_timer_t *timer) {
    time_t now;
    struct tm timeinfo;
    char buffer[32];

    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year > 70) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d  %H:%M", &timeinfo);
        lv_label_set_text(cui_datetime1, buffer);
    } else {
        lv_label_set_text(cui_datetime1, "Sin hora");
    }
}

