#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <time.h>
#include <sys/time.h>
#include "esp_timer.h"
#include "esp_log.h"

// Estructura para manejar fecha y hora del sistema
typedef struct {
    int year;
    int month;    // 1-12
    int day;      // 1-31
    int hour;     // 0-23
    int minute;   // 0-59
    int second;   // 0-59
} system_datetime_t;

// Variable global del sistema
extern system_datetime_t g_system_datetime;

// Timer para actualización automática
extern esp_timer_handle_t datetime_update_timer;

// Funciones principales
void system_time_init(void);
void system_time_set(system_datetime_t* datetime);
void system_time_get(system_datetime_t* datetime);
void system_time_update_from_network(void);
void system_time_start_auto_update(void);
void system_time_stop_auto_update(void);

// Callback del timer (actualización cada minuto)
void datetime_timer_callback(void* arg);

// Funciones de conversión
time_t system_datetime_to_timestamp(system_datetime_t* datetime);
void timestamp_to_system_datetime(time_t timestamp, system_datetime_t* datetime);

// Funciones para la UI
void system_time_update_ui_displays(void);

#endif // SYSTEM_TIME_H 