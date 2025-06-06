/**
 * @file CH422G.c
 * @brief Implementación del manejo del expansor de IO CH422G vía I2C.
 * 
 * Este módulo proporciona funciones para configurar y controlar pines digitales
 * de entrada y salida utilizando el chip CH422G. Está diseñado para su uso
 * en sistemas embebidos con ESP32 y el protocolo I2C.
 * 
 * @version 1.0
 * @date 2024-02-03
 * 
 * @note Este archivo fue reutilizado y adaptado para el proyecto del horno de vacío.
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

#include "CH422G.h"

static bool ch422g_mode_initialized = false;

/**
 * @brief Lee un registro de entrada del CH422G.
 * 
 * @param addr Dirección del registro a leer.
 * @param data Puntero donde se almacenará el dato leído.
 * @return esp_err_t ESP_OK si la operación fue exitosa, o un código de error.
 */
esp_err_t read_input_reg(uint8_t addr, uint8_t *data)
{
    return i2c_master_read_from_device(I2C_MASTER_NUM, addr, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Escribe un valor en un registro de salida del CH422G.
 * 
 * @param addr Dirección del registro a escribir.
 * @param data Dato que se escribirá en el registro.
 * @return esp_err_t ESP_OK si la operación fue exitosa, o un código de error.
 */
esp_err_t write_output_reg(uint8_t addr, uint8_t data)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, &data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Configura el CH422G en modo salida push-pull y escribe un valor en el puerto IO.
 * 
 * @param pin Máscara de 8 bits que representa los pines a nivel alto (1) o bajo (0).
 * @return esp_err_t ESP_OK si la operación fue exitosa, o un código de error.
 */
esp_err_t CH422G_io_output(uint8_t pin)
{
    write_output_reg(CH422G_Mode, CH422G_Mode_IO_OE); // Configura IO como salida push-pull
    return write_output_reg(CH422G_IO_OUT, pin);
}

/**
 * @brief Configura el CH422G en modo salida open-drain (OD) y escribe un valor.
 * 
 * @param pin Máscara de 8 bits que representa los pines a nivel alto (1) o bajo (0).
 * @return esp_err_t ESP_OK si la operación fue exitosa, o un código de error.
 */
esp_err_t CH422G_od_output(uint8_t pin)
{
    write_output_reg(CH422G_Mode, CH422G_Mode_OD_EN & 0x00); // Configura IO como salida OD
    return write_output_reg(CH422G_OD_OUT, pin);
}

/**
 * @brief Lee el estado de los pines configurados como entrada en el CH422G.
 * 
 * @param pin Máscara del pin o pines a leer (ej. (1 << 3) para el pin 3).
 * @return uint8_t Valor lógico leído (0 o distinto de 0).
 */
uint8_t CH422G_io_input(uint8_t pin)
{
    uint8_t value = 0;
    write_output_reg(CH422G_Mode, 0); // Configura IO como entrada
    read_input_reg(CH422G_IO_IN, &value);
    return (value & pin);
}

/**
 * @brief Asegura que el CH422G esté configurado en modo push-pull solo una vez.
 * 
 * Esta función es útil al inicializar el sistema para evitar reconfiguraciones
 * innecesarias del chip.
 */
void CH422G_EnsurePushPullMode(void)
{
    if (!ch422g_mode_initialized) {
        write_output_reg(0x48, 0x00);  // Configura OCx como push-pull
        DEV_Delay_ms(1);
        ch422g_mode_initialized = true;
        printf("CH422G configurado en modo push-pull\n");
    }
}
