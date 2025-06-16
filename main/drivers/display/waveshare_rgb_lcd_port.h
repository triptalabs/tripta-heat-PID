/**
 * @file waveshare_rgb_lcd_port.h
 * @brief Controlador para pantalla LCD RGB Waveshare con soporte táctil
 * 
 * @details Este archivo proporciona la configuración y declaraciones necesarias para inicializar
 * y controlar una pantalla LCD RGB Waveshare. Incluye funcionalidades para:
 * - Control del backlight
 * - Integración con LVGL
 * - Soporte táctil
 * - Configuración de pines GPIO
 * - Configuración I2C
 * 
 * @author Espressif Systems
 * @version 1.0
 * @date 2022
 * 
 * @copyright Copyright (c) 2022 Espressif Systems (Shanghai) CO LTD
 * @license CC0-1.0
 */

#ifndef _RGB_LCD_H_
#define _RGB_LCD_H_

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "lvgl_port.h"

/**
 * @brief Configuración del bus I2C para comunicación con periféricos
 * @{
 */

/**
 * @brief Pin GPIO para la señal de reloj I2C (SCL)
 * @note Pin 9 en el ESP32-S3
 */
#define I2C_MASTER_SCL_IO           9

/**
 * @brief Pin GPIO para la señal de datos I2C (SDA)
 * @note Pin 8 en el ESP32-S3
 */
#define I2C_MASTER_SDA_IO           8

/**
 * @brief Número del puerto I2C maestro
 * @note 0 para el primer puerto I2C
 */
#define I2C_MASTER_NUM              0

/**
 * @brief Frecuencia del reloj I2C en Hz
 * @note 400kHz es una frecuencia estándar para I2C
 */
#define I2C_MASTER_FREQ_HZ          400000

/**
 * @brief Deshabilitación del buffer de transmisión I2C
 * @note 0 indica que el buffer está deshabilitado
 */
#define I2C_MASTER_TX_BUF_DISABLE   0

/**
 * @brief Deshabilitación del buffer de recepción I2C
 * @note 0 indica que el buffer está deshabilitado
 */
#define I2C_MASTER_RX_BUF_DISABLE   0

/**
 * @brief Tiempo de espera para operaciones I2C en milisegundos
 * @note 1000ms = 1 segundo
 */
#define I2C_MASTER_TIMEOUT_MS       1000

/** @} */

/**
 * @brief Configuración de pines GPIO
 * @{
 */

/**
 * @brief Pin GPIO para entrada IO 4
 * @note Utilizado para entradas digitales
 */
#define GPIO_INPUT_IO_4             4

/**
 * @brief Máscara de bits para pines de entrada GPIO
 * @note Configura el pin GPIO_INPUT_IO_4 como entrada
 */
#define GPIO_INPUT_PIN_SEL          (1ULL << GPIO_INPUT_IO_4)

/** @} */

/**
 * @brief Configuración de la pantalla LCD
 * @{
 */

/**
 * @brief Resolución horizontal de la pantalla LCD
 * @note Definida por LVGL_PORT_H_RES
 */
#define EXAMPLE_LCD_H_RES               (LVGL_PORT_H_RES)

/**
 * @brief Resolución vertical de la pantalla LCD
 * @note Definida por LVGL_PORT_V_RES
 */
#define EXAMPLE_LCD_V_RES               (LVGL_PORT_V_RES)

#if ESP_PANEL_USE_1024_600_LCD
    /**
     * @brief Pixel clock frequency for 1024x600 LCD panel in Hz.
     */
    #define EXAMPLE_LCD_PIXEL_CLOCK_HZ      (21 * 1000 * 1000)
#else
    /**
     * @brief Pixel clock frequency for other LCD panels in Hz.
     */
    #define EXAMPLE_LCD_PIXEL_CLOCK_HZ      (16 * 1000 * 1000)
#endif

/**
 * @brief Number of bits per pixel for the LCD panel.
 */
#define EXAMPLE_LCD_BIT_PER_PIXEL       (16)

/**
 * @brief Number of bits per pixel for RGB interface.
 */
#define EXAMPLE_RGB_BIT_PER_PIXEL       (16)

/**
 * @brief Data width for RGB interface in bits.
 */
#define EXAMPLE_RGB_DATA_WIDTH          (16)

/**
 * @brief Size of the bounce buffer for RGB interface in pixels.
 */
#define EXAMPLE_RGB_BOUNCE_BUFFER_SIZE  (EXAMPLE_LCD_H_RES * CONFIG_EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT)

/**
 * @brief GPIO pin number for display enable signal (-1 if not used).
 */
#define EXAMPLE_LCD_IO_RGB_DISP         (-1)

/**
 * @brief GPIO pin number for vertical sync signal.
 */
#define EXAMPLE_LCD_IO_RGB_VSYNC        (GPIO_NUM_3)

/**
 * @brief GPIO pin number for horizontal sync signal.
 */
#define EXAMPLE_LCD_IO_RGB_HSYNC        (GPIO_NUM_46)

/**
 * @brief GPIO pin number for data enable signal.
 */
#define EXAMPLE_LCD_IO_RGB_DE           (GPIO_NUM_5)

/**
 * @brief GPIO pin number for pixel clock signal.
 */
#define EXAMPLE_LCD_IO_RGB_PCLK         (GPIO_NUM_7)

/**
 * @brief GPIO pin numbers for RGB data signals.
 */
#define EXAMPLE_LCD_IO_RGB_DATA0        (GPIO_NUM_14)
#define EXAMPLE_LCD_IO_RGB_DATA1        (GPIO_NUM_38)
#define EXAMPLE_LCD_IO_RGB_DATA2        (GPIO_NUM_18)
#define EXAMPLE_LCD_IO_RGB_DATA3        (GPIO_NUM_17)
#define EXAMPLE_LCD_IO_RGB_DATA4        (GPIO_NUM_10)
#define EXAMPLE_LCD_IO_RGB_DATA5        (GPIO_NUM_39)
#define EXAMPLE_LCD_IO_RGB_DATA6        (GPIO_NUM_0)
#define EXAMPLE_LCD_IO_RGB_DATA7        (GPIO_NUM_45)
#define EXAMPLE_LCD_IO_RGB_DATA8        (GPIO_NUM_48)
#define EXAMPLE_LCD_IO_RGB_DATA9        (GPIO_NUM_47)
#define EXAMPLE_LCD_IO_RGB_DATA10       (GPIO_NUM_21)
#define EXAMPLE_LCD_IO_RGB_DATA11       (GPIO_NUM_1)
#define EXAMPLE_LCD_IO_RGB_DATA12       (GPIO_NUM_2)
#define EXAMPLE_LCD_IO_RGB_DATA13       (GPIO_NUM_42)
#define EXAMPLE_LCD_IO_RGB_DATA14       (GPIO_NUM_41)
#define EXAMPLE_LCD_IO_RGB_DATA15       (GPIO_NUM_40)

/**
 * @brief GPIO pin number for reset signal (-1 if not used).
 */
#define EXAMPLE_LCD_IO_RST              (-1)

/**
 * @brief GPIO pin number for backlight control (-1 if not used).
 */
#define EXAMPLE_PIN_NUM_BK_LIGHT        (-1)

/**
 * @brief Backlight on level.
 */
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL   (1)

/**
 * @brief Backlight off level.
 */
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL  (!EXAMPLE_LCD_BK_LIGHT_ON_LEVEL)

/**
 * @brief GPIO pin number for touch reset signal (-1 if not used).
 */
#define EXAMPLE_PIN_NUM_TOUCH_RST       (-1)

/**
 * @brief GPIO pin number for touch interrupt signal (-1 if not used).
 */
#define EXAMPLE_PIN_NUM_TOUCH_INT       (-1)

/**
 * @brief Tag used for logging.
 */
static const char *TAG = "example";

/**
 * @brief Funciones de control de la pantalla LCD
 * @{
 */

/**
 * @brief Bloquea la biblioteca LVGL para operaciones seguras en hilos
 * 
 * @param timeout_ms Tiempo máximo de espera en milisegundos para adquirir el bloqueo
 * @return true Si se adquirió el bloqueo exitosamente
 * @return false Si no se pudo adquirir el bloqueo en el tiempo especificado
 */
bool example_lvgl_lock(int timeout_ms);

/**
 * @brief Libera el bloqueo de la biblioteca LVGL
 * 
 * @note Debe llamarse después de example_lvgl_lock() cuando se complete la operación
 */
void example_lvgl_unlock(void);

/**
 * @brief Inicializa la pantalla LCD RGB y el controlador táctil
 * 
 * @details Esta función realiza las siguientes tareas:
 * 1. Configura la pantalla LCD RGB con los parámetros especificados
 * 2. Inicializa la biblioteca LVGL
 * 3. Configura el controlador táctil (si está habilitado)
 * 
 * @return esp_err_t ESP_OK si la inicialización fue exitosa, código de error en caso contrario
 */
esp_err_t waveshare_esp32_s3_rgb_lcd_init();

/**
 * @brief Enciende el backlight de la pantalla LCD RGB
 * 
 * @details Configura el chip CH422G para habilitar el backlight mediante comandos I2C
 * 
 * @return esp_err_t ESP_OK si la operación fue exitosa, código de error en caso contrario
 */
esp_err_t waveshare_rgb_lcd_bl_on();

/**
 * @brief Apaga el backlight de la pantalla LCD RGB
 * 
 * @details Configura el chip CH422G para deshabilitar el backlight mediante comandos I2C
 * 
 * @return esp_err_t ESP_OK si la operación fue exitosa, código de error en caso contrario
 */
esp_err_t wavesahre_rgb_lcd_bl_off();

/** @} */

#endif // _RGB_LCD_H_