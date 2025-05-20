/**
 * @file DEV_Config.c
 * @brief Configuración e inicialización de hardware para interfaces I2C y temporización.
 * 
 * Este módulo proporciona funciones de bajo nivel para inicializar el bus I2C,
 * realizar lecturas y escrituras sobre el mismo, y generar retardos temporales.
 * Es utilizado como backend para módulos como CH422G o sensores conectados por I2C.
 * 
 * @version 1.0
 * @date 2024-01-27
 * 
 * @note Este archivo fue reutilizado y adaptado para el proyecto del horno de vacío con ESP32.
 * 
 * @copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */

#include "DEV_Config.h"

/**
 * @brief Inicializa el bus I2C en modo maestro.
 * 
 * Configura los pines SDA y SCL, habilita resistencias de pull-up internas
 * y establece la velocidad de reloj definida en `I2C_MASTER_FREQ_HZ`.
 * 
 * @return esp_err_t ESP_OK si la inicialización fue exitosa, o un código de error.
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * @brief Escribe un byte en un registro específico de un dispositivo I2C.
 * 
 * @param addr Dirección I2C del dispositivo esclavo.
 * @param reg Registro interno a escribir.
 * @param Value Valor que se escribirá.
 * @return esp_err_t ESP_OK si fue exitoso, o un código de error.
 */
esp_err_t DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t Value)
{
    uint8_t write_buf[2] = {reg, Value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, write_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Escribe múltiples bytes en un dispositivo I2C (modo directo).
 * 
 * @param addr Dirección I2C del dispositivo esclavo.
 * @param pData Puntero al buffer con los datos a enviar.
 * @param Len Longitud del buffer.
 * @return esp_err_t ESP_OK si fue exitoso, o un código de error.
 */
esp_err_t DEV_I2C_Write_nByte(uint8_t addr, uint8_t *pData, uint32_t Len)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, pData, Len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Lee un byte desde un registro específico de un dispositivo I2C.
 * 
 * @param addr Dirección I2C del dispositivo esclavo.
 * @param reg Registro interno desde donde se desea leer.
 * @param data Puntero a la variable donde se almacenará el valor leído.
 * @return esp_err_t ESP_OK si fue exitoso, o un código de error.
 */
esp_err_t DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg, uint8_t *data)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Lee múltiples bytes desde un registro específico de un dispositivo I2C.
 * 
 * @param addr Dirección I2C del dispositivo esclavo.
 * @param reg Registro desde donde se comenzará a leer.
 * @param pData Puntero al buffer donde se almacenarán los datos leídos.
 * @param Len Número de bytes a leer.
 * @return esp_err_t ESP_OK si fue exitoso, o un código de error.
 */
esp_err_t DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t Len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, pData, Len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Realiza una pausa o retardo por el número de milisegundos indicado.
 * 
 * @param xms Milisegundos de espera.
 */
void DEV_Delay_ms(uint32_t xms)
{
    vTaskDelay(xms / portTICK_PERIOD_MS);
}

/**
 * @brief Inicializa el módulo de configuración de hardware (I2C).
 * 
 * Esta función encapsula la inicialización del bus I2C y puede extenderse
 * para inicializar otros periféricos en el futuro.
 * 
 * @return uint8_t Siempre retorna 0 si no hay error. 
 */
uint8_t DEV_Module_Init(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    return 0;
}
