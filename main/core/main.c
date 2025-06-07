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
#include "wifi_manager.h"
#include "DEV_Config.h"
#include "CH422G.h"
#include "pid_controller.h"
#include <string.h>
#include <time.h>
#include "lvgl.h"
#include <sys/time.h>

#ifndef TAG
#define TAG "main" ///< Etiqueta de log para este módulo
#endif

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
    waveshare_rgb_lcd_bl_on();

    /* Inicialización de la interfaz gráfica y servicios principales
     * Se realiza dentro de un bloque protegido por mutex para evitar
     * conflictos con otras tareas que acceden a LVGL
     */
    if (lvgl_port_lock(-1)) {
        // Carga interfaz gráfica
        ui_init();

        // Configura WiFi y hora
        wifi_manager_init(ui_Dropdown1, cui_datetime1);

        // Crea temporizador para actualizar hora cada minuto
        lv_timer_create(actualizar_hora_cb, 60000, NULL);

        // Inicia tareas principales
        start_temperature_task();
        pid_controller_init(0.0f);

        lvgl_port_unlock();
    }

    // Nota: no se necesita un bucle explícito; LVGL corre en background.
}
