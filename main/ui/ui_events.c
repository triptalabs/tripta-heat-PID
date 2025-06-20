/**
 * @file ui_events.c
 * @brief Implementación de los manejadores de eventos de la interfaz de usuario
 * @details Este archivo contiene la implementación de todas las funciones que manejan
 *          los eventos de la interfaz de usuario, incluyendo:
 *          - Control de WiFi y Bluetooth
 *          - Control del sistema PID
 *          - Gestión del temporizador
 *          - Configuración de fecha y hora
 *          - Actualización del firmware
 * @author SquareLine Studio
 * @version 1.5.1
 * @date 2024
 */

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
#include "../core/bt.h"
#include "update.h"
#include <math.h>
#include "driver/gpio.h"
#include "CH422G.h"
#include <sys/time.h>
#include <time.h>
#include "pid_controller.h"
#include "../core/statistics.h"
#include "system_test.h"
#include "../core/system_time.h"

/**
 * @brief Comando para establecer parámetros en el CH422G
 */
#define CH422_CMD_SET_PARAM   0x48

/**
 * @brief Modo push-pull para el CH422G
 * @details Configura los IOs como entrada y OCx como push-pull
 */
#define CH422_PUSH_PULL_MODE  0x00

/**
 * @brief Número de puntos en el gráfico de temperatura
 */
#define CHART_POINT_COUNT 240

// Declaraciones de variables externas
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
/**
 * @brief Etiqueta para los mensajes de log de eventos
 */
static const char *EVENTS_TAG = "Events";


/**
 * @brief Timer LVGL para el control de minutos
 */
static lv_timer_t *timer_minutos = NULL;
static int minutos_restantes = 0;

/**
 * @brief Activa el módulo WiFi del dispositivo
 * @details Inicializa el NVS, configura el modo estación WiFi y lo activa
 * @param e Puntero al evento que activó la función
 */
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

/**
 * @brief Desactiva el módulo WiFi del dispositivo
 * @details Detiene y desinicializa el módulo WiFi
 * @param e Puntero al evento que activó la función
 */
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

/**
 * @brief Activa el módulo Bluetooth del dispositivo
 * @param e Puntero al evento que activó la función
 */
void EncenderBt(lv_event_t * e) {
    ESP_LOGI(EVENTS_TAG, "Usuario solicitó encender Bluetooth desde UI");
    
    esp_err_t ret = bt_start();
    if (ret == ESP_OK) {
        ESP_LOGI(EVENTS_TAG, "Bluetooth iniciado exitosamente desde UI");
        // TODO: Actualizar indicador visual en UI cuando esté disponible
    } else {
        ESP_LOGE(EVENTS_TAG, "Error al iniciar Bluetooth desde UI: %s", esp_err_to_name(ret));
        // TODO: Mostrar mensaje de error en UI cuando esté disponible
    }
}

/**
 * @brief Desactiva el módulo Bluetooth del dispositivo
 * @param e Puntero al evento que activó la función
 */
void ApagarBt(lv_event_t * e) {
    ESP_LOGI(EVENTS_TAG, "Usuario solicitó apagar Bluetooth desde UI");
    
    esp_err_t ret = bt_stop();
    if (ret == ESP_OK) {
        ESP_LOGI(EVENTS_TAG, "Bluetooth detenido exitosamente desde UI");
        // TODO: Actualizar indicador visual en UI cuando esté disponible
    } else {
        ESP_LOGE(EVENTS_TAG, "Error al detener Bluetooth desde UI: %s", esp_err_to_name(ret));
        // TODO: Mostrar mensaje de error en UI cuando esté disponible
    }
}

/**
 * @brief Activa el controlador PID
 * @details Configura el setpoint y activa el controlador PID
 * @param e Puntero al evento que activó la función
 */
void EncenderPID(lv_event_t *e) {
    float setpoint = lv_arc_get_value(ui_ArcSetTemp);  // Obtiene el setpoint desde la UI
    pid_set_setpoint(setpoint);    
    enable_pid();                    // Lo pasa al controlador                                     // Activa el PID
    
    // Iniciar nueva sesión de estadísticas
    statistics_start_session();
    
    printf("PID habilitado desde GUI (Setpoint = %.2f°C)\n", setpoint);
}

/**
 * @brief Desactiva el controlador PID
 * @details Desactiva el controlador y apaga el relé SSR
 * @param e Puntero al evento que activó la función
 */
void ApagarPID(lv_event_t *e) {
    disable_pid();          // Desactiva la lógica PID
    desactivar_ssr();       // 💥 Apaga físicamente el relé (¡clave!)
    
    // Finalizar sesión de estadísticas
    statistics_end_session();
    
    printf("PID deshabilitado desde GUI\n");
}

/**
 * @brief Callback del temporizador LVGL
 * @details Actualiza el tiempo restante y apaga el PID cuando se completa
 * @param t Puntero al temporizador
 */
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

/**
 * @brief Inicia el temporizador del sistema
 * @details Configura y activa el temporizador con el valor especificado
 * @param e Puntero al evento que activó la función
 */
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

/**
 * @brief Detiene el temporizador del sistema
 * @details Cancela el temporizador y reinicia el contador
 * @param e Puntero al evento que activó la función
 */
void ApagarTimer(lv_event_t *e) {
    if (timer_minutos != NULL) {
        lv_timer_del(timer_minutos);
        timer_minutos = NULL;
        ESP_LOGI(EVENTS_TAG, "Temporizador detenido manualmente.");
    }

    minutos_restantes = 0;
    lv_arc_set_value(ui_ArcSetTime, 0);
}

/**
 * @brief Cambia el nombre del dispositivo Bluetooth
 * @details Actualiza el nombre del dispositivo Bluetooth con el valor proporcionado
 * @param e Puntero al evento que activó la función
 */
void CambiarNombreBT(lv_event_t *e)
{
    const char *new_bt_name = lv_textarea_get_text(ui_nombrebt);
    if (!new_bt_name || strlen(new_bt_name) == 0) {
        ESP_LOGW(EVENTS_TAG, "No se proporcionó un nombre válido para Bluetooth desde UI.");
        return;
    }

    ESP_LOGI(EVENTS_TAG, "Usuario solicitó cambiar nombre Bluetooth a: '%s' desde UI", new_bt_name);

    // Usar el módulo BT en lugar de código hardcodeado
    esp_err_t ret = bt_set_device_name(new_bt_name);
    if (ret == ESP_OK) {
        ESP_LOGI(EVENTS_TAG, "Nombre Bluetooth actualizado exitosamente desde UI: %s", new_bt_name);
        // TODO: Mostrar confirmación en UI cuando esté disponible
    } else {
        ESP_LOGE(EVENTS_TAG, "Error al cambiar nombre Bluetooth desde UI: %s", esp_err_to_name(ret));
        // TODO: Mostrar mensaje de error en UI cuando esté disponible
    }
}

/**
 * @brief Actualiza la fecha y hora del sistema usando el nuevo sistema
 * @details Aplica los cambios recopilados desde el calendario y rollers al sistema
 * @param e Puntero al evento que activó la función
 */
void CambiarFechaHora(lv_event_t * e) {
    // Usar la función del nuevo sistema que aplica los cambios ya recopilados
    apply_datetime_changes_to_system();
    
    ESP_LOGI(EVENTS_TAG, "Fecha y hora aplicada desde el nuevo sistema de calendario y rollers");
}

/**
 * @brief Inicia el proceso de actualización del firmware
 * @details Verifica si hay actualizaciones disponibles y las instala
 * @param e Puntero al evento que activó la función
 */
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

/**
 * @brief Intenta establecer una conexión WiFi
 * @details Configura y conecta a la red WiFi seleccionada
 * @param e Puntero al evento que activó la función
 */
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

/**
 * @brief Actualiza los parámetros del controlador PID
 * @details Configura los valores de Kp, Ki y Kd del controlador PID con validación
 *          y actualización de la interfaz visual
 * @param e Puntero al evento que activó la función
 */
void ActualizarK(lv_event_t * e) {
    const char *kp_str = lv_textarea_get_text(ui_TextAreaKp);
    const char *ki_str = lv_textarea_get_text(ui_TextAreaKi);
    const char *kd_str = lv_textarea_get_text(ui_TextAreaKd);

    // Validación de entrada - verificar que todos los campos tengan valores
    if (strlen(kp_str) == 0 || strlen(ki_str) == 0 || strlen(kd_str) == 0) {
        ESP_LOGW(EVENTS_TAG, "Todos los campos Kp, Ki, Kd deben tener valores.");
        return;
    }

    // Conversión a float
    float kp = atof(kp_str);
    float ki = atof(ki_str);
    float kd = atof(kd_str);

    // Validación de rangos razonables para parámetros PID
    if (kp < 0.0f || kp > 100.0f) {
        ESP_LOGW(EVENTS_TAG, "Kp fuera de rango válido (0.0 - 100.0): %.2f", kp);
        return;
    }
    if (ki < 0.0f || ki > 10.0f) {
        ESP_LOGW(EVENTS_TAG, "Ki fuera de rango válido (0.0 - 10.0): %.2f", ki);
        return;
    }
    if (kd < 0.0f || kd > 100.0f) {
        ESP_LOGW(EVENTS_TAG, "Kd fuera de rango válido (0.0 - 100.0): %.2f", kd);
        return;
    }

    // Aplicar nuevos parámetros al controlador PID
    pid_set_params(kp, ki, kd);

    // Actualizar etiquetas visuales con los nuevos valores
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Kp: %.2f", kp);
    lv_label_set_text(ui_LabelKp, buffer);
    
    snprintf(buffer, sizeof(buffer), "Ki: %.2f", ki);
    lv_label_set_text(ui_LabelKi, buffer);
    
    snprintf(buffer, sizeof(buffer), "Kd: %.2f", kd);
    lv_label_set_text(ui_LabelKd, buffer);

    // Limpiar campos de texto después de aplicar los cambios
    lv_textarea_set_text(ui_TextAreaKp, "");
    lv_textarea_set_text(ui_TextAreaKi, "");
    lv_textarea_set_text(ui_TextAreaKd, "");

    ESP_LOGI(EVENTS_TAG, "Parámetros PID actualizados exitosamente: Kp=%.2f, Ki=%.2f, Kd=%.2f", kp, ki, kd);
}

/**
 * @brief Actualiza el estado del PID en la interfaz
 * @details Muestra la temperatura actual y el estado del calentador
 * @param temperatura Temperatura actual del sistema
 * @param heating_on Estado del calentador
 */
void ui_actualizar_estado_pid(float temperatura, bool heating_on) {
    static char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.1f°C\nTemperatura", temperatura);
    lv_label_set_text(ui_editLabelGetStatus, buffer);
}

/**
 * @brief Ejecuta el test del sistema y actualiza la UI con los resultados
 * @details Ejecuta las pruebas de sensor y SSR, y muestra los resultados
 *          formateados en el label de la interfaz
 * @param e Puntero al evento que activó la función
 */
void RunSystemTest(lv_event_t *e) {
    ESP_LOGI(EVENTS_TAG, "Iniciando test del sistema desde UI...");
    
    // Mostrar mensaje de "Ejecutando test..." en la UI
    lv_label_set_text(ui_LabelEditTestResult, "Ejecutando test del sistema...\n\nPor favor espere.");
    
    // Forzar actualización de la UI
    lv_task_handler();
    
    // Buffer para almacenar los resultados del test
    static char test_result_buffer[SYSTEM_TEST_RESULT_MAX_LEN];
    
    // Ejecutar el test del sistema
    esp_err_t test_result = system_test_run_quick(test_result_buffer, sizeof(test_result_buffer));
    
    if (test_result == ESP_OK) {
        ESP_LOGI(EVENTS_TAG, "Test del sistema completado exitosamente");
        // Actualizar la UI con los resultados del test
        lv_label_set_text(ui_LabelEditTestResult, test_result_buffer);
    } else {
        ESP_LOGE(EVENTS_TAG, "Error ejecutando test del sistema: %s", esp_err_to_name(test_result));
        // Mostrar mensaje de error en la UI
        lv_label_set_text(ui_LabelEditTestResult, 
                          "Error ejecutando test del sistema.\n\n"
                          "Verifique las conexiones\n"
                          "e intente nuevamente.");
    }
}

/**
 * @brief Callback para actualizar la hora en la interfaz (DEPRECATED)
 * @details Esta función ha sido reemplazada por el módulo statusbar_manager.
 *          Se mantiene por compatibilidad pero no debería usarse en código nuevo.
 * @param timer Puntero al temporizador que activó la función
 * @deprecated Usar statusbar_manager en su lugar
 */
void actualizar_hora_cb(lv_timer_t *timer) {
    ESP_LOGW(EVENTS_TAG, "actualizar_hora_cb está deprecada. Use statusbar_manager en su lugar.");
    // Esta función ya no hace nada, la gestión de hora se maneja en statusbar_manager
    (void)timer; // Suprimir warning
}

