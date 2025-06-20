#include "system_time.h"
#include "esp_sntp.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "ui.h"
#include "ui_helpers.h"
#include "statusbar_manager.h"

static const char *TAG = "SYSTEM_TIME";

// Variables globales
system_datetime_t g_system_datetime = {2025, 6, 25, 12, 0, 0};
esp_timer_handle_t datetime_update_timer = NULL;
bool timer_active = false;

void system_time_init(void) {
    ESP_LOGI(TAG, "Inicializando sistema de tiempo");
    
    // Configurar SNTP para sincronización automática
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.nist.gov");
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    
    // Configurar zona horaria (ajustar según necesidad)
    setenv("TZ", "UTC-0", 1);
    tzset();
    
    ESP_LOGI(TAG, "Sistema de tiempo inicializado");
}

void system_time_set(system_datetime_t* datetime) {
    if (datetime == NULL) return;
    
    // Actualizar variable global
    g_system_datetime = *datetime;
    
    // Convertir a timestamp y actualizar el RTC del sistema
    time_t timestamp = system_datetime_to_timestamp(datetime);
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    
    // Actualizar displays de UI
    system_time_update_ui_displays();
    
    ESP_LOGI(TAG, "Fecha y hora actualizada: %04d-%02d-%02d %02d:%02d:%02d", 
             datetime->year, datetime->month, datetime->day,
             datetime->hour, datetime->minute, datetime->second);
}

void system_time_get(system_datetime_t* datetime) {
    if (datetime == NULL) return;
    
    time_t now;
    time(&now);
    timestamp_to_system_datetime(now, datetime);
    
    // También actualizar la variable global
    g_system_datetime = *datetime;
}

void system_time_update_from_network(void) {
    ESP_LOGI(TAG, "Actualizando tiempo desde red...");
    
    // Inicializar SNTP si no está inicializado
    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        esp_sntp_init();
    }
    
    // Esperar sincronización (hasta 10 segundos)
    int retry = 0;
    const int retry_count = 100; // 10 segundos con delays de 100ms
    
    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Esperando sincronización SNTP... (%d/%d)", retry, retry_count);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        // Obtener tiempo actualizado del sistema
        system_time_get(&g_system_datetime);
        system_time_update_ui_displays();
        ESP_LOGI(TAG, "Tiempo sincronizado exitosamente desde red");
    } else {
        ESP_LOGW(TAG, "No se pudo sincronizar tiempo desde red");
    }
}

void system_time_start_auto_update(void) {
    if (timer_active) return;
    
    const esp_timer_create_args_t timer_args = {
        .callback = &datetime_timer_callback,
        .arg = NULL,
        .name = "datetime_timer"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &datetime_update_timer));
    
    // Configurar para ejecutar cada minuto (60 segundos)
    ESP_ERROR_CHECK(esp_timer_start_periodic(datetime_update_timer, 60 * 1000000)); // 60 segundos en microsegundos
    
    timer_active = true;
    ESP_LOGI(TAG, "Timer de actualización automática iniciado");
}

void system_time_stop_auto_update(void) {
    if (!timer_active || datetime_update_timer == NULL) return;
    
    ESP_ERROR_CHECK(esp_timer_stop(datetime_update_timer));
    ESP_ERROR_CHECK(esp_timer_delete(datetime_update_timer));
    datetime_update_timer = NULL;
    timer_active = false;
    
    ESP_LOGI(TAG, "Timer de actualización automática detenido");
}

void datetime_timer_callback(void* arg) {
    // Obtener tiempo actual del sistema
    system_time_get(&g_system_datetime);
    
    // Actualizar todas las pantallas que muestren fecha/hora
    system_time_update_ui_displays();
    
    ESP_LOGD(TAG, "Timer: Fecha y hora actualizada automáticamente");
}

time_t system_datetime_to_timestamp(system_datetime_t* datetime) {
    if (datetime == NULL) return 0;
    
    struct tm timeinfo;
    timeinfo.tm_year = datetime->year - 1900;  // años desde 1900
    timeinfo.tm_mon = datetime->month - 1;     // meses desde enero (0-11)
    timeinfo.tm_mday = datetime->day;
    timeinfo.tm_hour = datetime->hour;
    timeinfo.tm_min = datetime->minute;
    timeinfo.tm_sec = datetime->second;
    timeinfo.tm_isdst = -1;  // dejar que el sistema determine DST
    
    return mktime(&timeinfo);
}

void timestamp_to_system_datetime(time_t timestamp, system_datetime_t* datetime) {
    if (datetime == NULL) return;
    
    struct tm *timeinfo = localtime(&timestamp);
    
    datetime->year = timeinfo->tm_year + 1900;
    datetime->month = timeinfo->tm_mon + 1;
    datetime->day = timeinfo->tm_mday;
    datetime->hour = timeinfo->tm_hour;
    datetime->minute = timeinfo->tm_min;
    datetime->second = timeinfo->tm_sec;
}

void system_time_update_ui_displays(void) {
    // Actualizar la barra de estado forzando la actualización
    statusbar_update_time(true);
    
    // Aquí se pueden agregar más actualizaciones de UI según sea necesario
    // Por ejemplo, si hay otros widgets que muestren fecha/hora en otras pantallas
    
    ESP_LOGD(TAG, "Displays de UI actualizados con nueva fecha/hora");
} 