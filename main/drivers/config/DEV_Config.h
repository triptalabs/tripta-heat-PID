/**
 * @file DEV_Config.h
 * @brief Configuración de hardware y definiciones de interfaz para el controlador de temperatura
 * @author TriptaLabs
 * @version 1.0
 * @date 2024-01-27
 * 
 * @details Este archivo contiene las definiciones de hardware y configuraciones
 * necesarias para la comunicación I2C, GPIO y otras interfaces del sistema.
 * Proporciona una capa de abstracción para la interacción con el hardware
 * del ESP32, incluyendo:
 * - Configuración de pines GPIO
 * - Comunicación I2C
 * - Control PWM
 * - Temporización
 * - Lectura ADC
 * 
 * @note Este archivo es parte del sistema de control de temperatura TriptaLabs
 * y debe ser incluido en cualquier módulo que necesite interactuar con el hardware.
 * 
 * @copyright Copyright (c) 2024 TriptaLabs
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of theex Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdio.h>
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "driver/i2c.h"
#include "esp_console.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "waveshare_rgb_lcd_port.h"

/**
 * @defgroup tipos_datos Definiciones de tipos de datos
 * @brief Tipos de datos personalizados para el sistema
 * @{
 */
#define UBYTE uint8_t    /**< Tipo de dato para byte sin signo (8 bits) */
#define UWORD uint16_t   /**< Tipo de dato para palabra sin signo (16 bits) */
#define UDOUBLE uint32_t /**< Tipo de dato para doble palabra sin signo (32 bits) */
/** @} */

/**
 * @defgroup gpio_config Configuración de pines GPIO
 * @brief Definiciones para la configuración de pines GPIO
 * @{
 */
#define GPIO_INPUT_IO_4 4  /**< Pin GPIO 4 configurado como entrada digital */
// #define GPIO_INPUT_PIN_SEL 1ULL << GPIO_INPUT_IO_4  /**< Máscara para pin de entrada (comentado) */

#define GPIO_INPUT_IO_8 8  /**< Pin GPIO 8 configurado para SDA del bus I2C */
#define GPIO_SDA_PIN_SEL 1ULL << GPIO_INPUT_IO_8  /**< Máscara para pin SDA (bit 8) */

#define GPIO_OUTPUT_IO_9 9 /**< Pin GPIO 9 configurado para SCL del bus I2C */
#define GPIO_SCL_PIN_SEL 1ULL << GPIO_INPUT_IO_9  /**< Máscara para pin SCL (bit 9) */

#define ESP_INTR_FLAG_DEFAULT 0 /**< Bandera de interrupción por defecto para GPIO */
/** @} */

/**
 * @defgroup i2c_config Configuración del bus I2C
 * @brief Parámetros de configuración para la comunicación I2C
 * @{
 */
#define I2C_MASTER_NUM 0                        /**< Número de puerto I2C maestro (0) */
#define I2C_MASTER_FREQ_HZ 400000              /**< Frecuencia del reloj I2C (400 KHz) */
#define I2C_MASTER_TX_BUF_DISABLE 0            /**< Deshabilitar buffer de transmisión */
#define I2C_MASTER_RX_BUF_DISABLE 0            /**< Deshabilitar buffer de recepción */
#define I2C_MASTER_TIMEOUT_MS 1000             /**< Tiempo de espera I2C (1000 ms) */
#define WRITE_BIT I2C_MASTER_WRITE             /**< Bit de escritura I2C (0) */
#define READ_BIT I2C_MASTER_READ               /**< Bit de lectura I2C (1) */
#define ACK_CHECK_EN 0x1                       /**< Habilitar verificación ACK */
#define ACK_CHECK_DIS 0x0                      /**< Deshabilitar verificación ACK */
#define ACK_VAL 0x0                            /**< Valor de ACK (0) */
#define NACK_VAL 0x1                           /**< Valor de NACK (1) */
/** @} */

/**
 * @defgroup gpio_funcs Funciones de control GPIO
 * @brief Funciones para el control de pines GPIO
 * @{
 */
/**
 * @brief Escribe un valor digital en un pin GPIO
 * @param Pin Número del pin GPIO
 * @param Value Valor a escribir (0 o 1)
 */
void DEV_Digital_Write(uint16_t Pin, uint8_t Value);

/**
 * @brief Lee el valor digital de un pin GPIO
 * @param Pin Número del pin GPIO
 * @return Valor leído (0 o 1)
 */
uint8_t DEV_Digital_Read(uint16_t Pin);

/**
 * @brief Configura el modo de un pin GPIO
 * @param Pin Número del pin GPIO
 * @param Mode Modo de operación (entrada/salida)
 */
void DEV_GPIO_Mode(uint16_t Pin, uint16_t Mode);

/**
 * @brief Configura un pin como entrada de teclado
 * @param Pin Número del pin GPIO
 */
void DEV_KEY_Config(uint16_t Pin);

/**
 * @brief Configura una interrupción GPIO
 * @param Pin Número del pin GPIO
 * @param isr_handler Función manejadora de la interrupción
 */
void DEV_GPIO_INT(int32_t Pin, gpio_isr_t isr_handler);
/** @} */

/**
 * @defgroup adc_funcs Funciones de lectura ADC
 * @brief Funciones para la lectura de valores analógicos
 * @{
 */
/**
 * @brief Lee el valor del convertidor analógico-digital
 * @return Valor ADC de 12 bits (0-4095)
 */
uint16_t DEC_ADC_Read(void);
/** @} */

/**
 * @defgroup spi_funcs Funciones de comunicación SPI
 * @brief Funciones para la comunicación SPI
 * @{
 */
/**
 * @brief Escribe un byte por SPI
 * @param Value Byte a escribir
 */
void DEV_SPI_WriteByte(uint8_t Value);

/**
 * @brief Escribe múltiples bytes por SPI
 * @param pData Puntero al buffer de datos
 * @param Len Longitud del buffer
 */
void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len);
/** @} */

/**
 * @defgroup timing_funcs Funciones de temporización
 * @brief Funciones para el control de tiempos
 * @{
 */
/**
 * @brief Genera un retardo en milisegundos
 * @param xms Tiempo de retardo en milisegundos
 */
void DEV_Delay_ms(uint32_t xms);
/** @} */

/**
 * @defgroup i2c_funcs Funciones de comunicación I2C
 * @brief Funciones para la comunicación I2C
 * @{
 */
/**
 * @brief Escribe un byte en un registro I2C
 * @param addr Dirección del dispositivo I2C
 * @param reg Dirección del registro
 * @param Value Valor a escribir
 * @return Código de error ESP_OK si es exitoso
 */
esp_err_t DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t Value);

/**
 * @brief Escribe múltiples bytes en un registro I2C
 * @param addr Dirección del dispositivo I2C
 * @param pData Puntero al buffer de datos
 * @param Len Longitud del buffer
 * @return Código de error ESP_OK si es exitoso
 */
esp_err_t DEV_I2C_Write_nByte(uint8_t addr, uint8_t *pData, uint32_t Len);

/**
 * @brief Lee un byte de un registro I2C
 * @param addr Dirección del dispositivo I2C
 * @param reg Dirección del registro
 * @param data Puntero donde se almacenará el valor leído
 * @return Código de error ESP_OK si es exitoso
 */
esp_err_t DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg, uint8_t *data);

/**
 * @brief Lee múltiples bytes de un registro I2C
 * @param addr Dirección del dispositivo I2C
 * @param reg Dirección del registro
 * @param pData Puntero al buffer donde se almacenarán los datos
 * @param Len Longitud del buffer
 * @return Código de error ESP_OK si es exitoso
 */
esp_err_t DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t Len);

/**
 * @brief Escanea dispositivos I2C conectados
 * @note Imprime las direcciones de los dispositivos encontrados
 */
void DEV_I2C_SCAN();
/** @} */

/**
 * @defgroup pwm_funcs Funciones de control PWM
 * @brief Funciones para el control PWM
 * @{
 */
/**
 * @brief Configura el valor PWM
 * @param Value Valor PWM (0-255)
 */
void DEV_SET_PWM(uint8_t Value);
/** @} */

/**
 * @defgroup init_funcs Funciones de inicialización
 * @brief Funciones para la inicialización del sistema
 * @{
 */
/**
 * @brief Inicializa el módulo
 * @return 0 si la inicialización es exitosa, 1 en caso de error
 * @note Esta función debe ser llamada antes de usar cualquier otra función
 */
uint8_t DEV_Module_Init(void);
/** @} */

#endif
