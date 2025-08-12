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
#include "statistics.h"
#include "../ui/components/statusbar_manager.h"
#include "update.h"
#include "system_time.h"
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
    ESP_LOGI(TAG, "=== INICIANDO TRIPTABS HEAT CONTROLLER ===");
    ESP_LOGI(TAG, "Firmware Version: 1.0.0");
    ESP_LOGI(TAG, "ESP32-S3 Vacuum Oven Controller");

    // Inicializar módulo de actualización
    update_init();

    // ========================================
    // INICIALIZACIÓN PRINCIPAL
    // ========================================
    
    DEV_Module_Init();  // Inicializa I2C

    // Inicializa pantalla RGB y mutex de LVGL
    waveshare_esp32_s3_rgb_lcd_init();
    waveshare_rgb_lcd_bl_on();

    /* Inicialización de la interfaz gráfica y servicios principales
     * Se realiza dentro de un bloque protegido por mutex para evitar
     * conflictos con otras tareas que acceden a LVGL
     */
    if (lvgl_port_lock(-1)) {
        // Inicializar sistema de tiempo antes de cargar la interfaz
        system_time_init();
        
        // Carga interfaz gráfica
        ui_init();

        // Inicializar el módulo de gestión de barra de estado
        statusbar_config_t statusbar_config = statusbar_get_default_config();
        statusbar_config.time_format = "%d %b %Y   |   %H:%M";
        statusbar_config.time_update_interval_ms = 60000;  // 1 minuto
        
        if (!statusbar_manager_init(ui_STATUSBAR, &statusbar_config)) {
            ESP_LOGE(TAG, "Error al inicializar el módulo de barra de estado");
        } else {
            ESP_LOGI(TAG, "Módulo de barra de estado inicializado correctamente");
        }

        // Configura WiFi (sin pasar cui_datetime1 ya que ahora lo maneja statusbar_manager)
        wifi_manager_init(ui_Dropdown1, NULL);

        // Inicia tareas principales
        start_temperature_task();
        pid_controller_init(0.0f);
        
        // Inicializar módulo de estadísticas
        if (statistics_init() != ESP_OK) {
            ESP_LOGE(TAG, "Error al inicializar el módulo de estadísticas");
        } else {
            ESP_LOGI(TAG, "Módulo de estadísticas inicializado correctamente");
        }

        lvgl_port_unlock();
    }
    
    // Todas las inicializaciones completadas exitosamente
    ESP_LOGI(TAG, "🎉 Sistema iniciado completamente");

    // Nota: no se necesita un bucle explícito; LVGL corre en background.
}
