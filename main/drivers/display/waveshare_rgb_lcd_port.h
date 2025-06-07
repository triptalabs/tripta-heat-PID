/**
 * @file rgb_lcd.h
 * @brief Header file for Waveshare RGB LCD and touch controller integration.
 * 
 * This file contains the configuration and declarations for initializing and controlling
 * the Waveshare RGB LCD panel, including backlight control, touch functionality, and LVGL integration.
 * 
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: CC0-1.0
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
 * @brief GPIO pin number used for I2C master clock.
 */
#define I2C_MASTER_SCL_IO           9

/**
 * @brief GPIO pin number used for I2C master data.
 */
#define I2C_MASTER_SDA_IO           8

/**
 * @brief I2C master port number.
 */
#define I2C_MASTER_NUM              0

/**
 * @brief I2C master clock frequency in Hz.
 */
#define I2C_MASTER_FREQ_HZ          400000

/**
 * @brief Disable I2C master transmit buffer.
 */
#define I2C_MASTER_TX_BUF_DISABLE   0

/**
 * @brief Disable I2C master receive buffer.
 */
#define I2C_MASTER_RX_BUF_DISABLE   0

/**
 * @brief Timeout for I2C master operations in milliseconds.
 */
#define I2C_MASTER_TIMEOUT_MS       1000

/**
 * @brief GPIO pin number used for input IO 4.
 */
#define GPIO_INPUT_IO_4             4

/**
 * @brief Bit mask for GPIO input pins.
 */
#define GPIO_INPUT_PIN_SEL          (1ULL << GPIO_INPUT_IO_4)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Horizontal resolution of the LCD panel.
 */
#define EXAMPLE_LCD_H_RES               (LVGL_PORT_H_RES)

/**
 * @brief Vertical resolution of the LCD panel.
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
 * @brief Locks the LVGL library for thread-safe operations.
 * 
 * @param timeout_ms Timeout in milliseconds to acquire the lock.
 * @return bool Returns true if the lock was acquired successfully, false otherwise.
 */
bool example_lvgl_lock(int timeout_ms);

/**
 * @brief Unlocks the LVGL library after thread-safe operations.
 */
void example_lvgl_unlock(void);

/**
 * @brief Initializes the RGB LCD panel and touch controller.
 * 
 * Sets up the RGB LCD panel with the specified configuration, initializes the LVGL library,
 * and configures the touch controller (if enabled).
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code otherwise.
 */
esp_err_t waveshare_esp32_s3_rgb_lcd_init();

/**
 * @brief Turns on the backlight of the RGB LCD.
 * 
 * Configures the CH422G chip to enable the backlight by setting the appropriate I2C commands.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code otherwise.
 */
esp_err_t waveshare_rgb_lcd_bl_on();

/**
 * @brief Turns off the backlight of the RGB LCD.
 * 
 * Configures the CH422G chip to disable the backlight by setting the appropriate I2C commands.
 * 
 * @return esp_err_t Returns ESP_OK on success, or an error code otherwise.
 */
esp_err_t wavesahre_rgb_lcd_bl_off();

#endif // _RGB_LCD_H_