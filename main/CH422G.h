/**
 * @file CH422G.h
 * @brief Definiciones y prototipos para el manejo del expansor de IO CH422G.
 * 
 * Este archivo contiene las definiciones de registros, máscaras de bits y funciones
 * necesarias para utilizar el chip CH422G como expansor de entradas/salidas digitales
 * mediante interfaz I2C. Se emplea en el proyecto del horno de vacío basado en ESP32.
 * 
 * @version 1.0
 * @date 2024-02-07
 * @ingroup CH422G
 * 
 * @note Este archivo fue reutilizado y adaptado para el proyecto actual.
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

#ifndef _CH422G_H_
#define _CH422G_H_

#include "DEV_Config.h"

/**
 * @note El chip CH422G no tiene una dirección I2C fija.
 * En su lugar, cada operación (modo, entrada, salida) se realiza usando una "dirección de función"
 * que se utiliza como dirección esclava en la comunicación I2C.
 */

/** @name Direcciones de operación del CH422G */
/**@{*/
#define CH422G_Mode          0x24  /**< Dirección para configurar el modo de funcionamiento */
#define CH422G_OD_OUT        0x23  /**< Dirección para salida open-drain (OCx) */
#define CH422G_IO_OUT        0x38  /**< Dirección para salida push-pull (IOx) */
#define CH422G_IO_IN         0x26  /**< Dirección para lectura de entradas digitales */
/**@}*/

/** @name Máscaras de configuración de modo */
/**@{*/
#define CH422G_Mode_IO_OE    0x01 /**< Habilita las salidas push-pull en pines IO */
#define CH422G_Mode_A_SCAN   0x02 /**< Habilita el escaneo automático dinámico */
#define CH422G_Mode_OD_EN    0x04 /**< Habilita salida open-drain en pines OC0~OC3 */
#define CH422G_Mode_SLEEP    0x08 /**< Activa modo de bajo consumo */
/**@}*/

/** @name Pines OC (open-drain outputs) */
/**@{*/
#define CH422G_OD_OUT_0      0x01 /**< OC0 salida nivel alto */
#define CH422G_OD_OUT_1      0x02 /**< OC1 salida nivel alto */
#define CH422G_OD_OUT_2      0x04 /**< OC2 salida nivel alto */
#define CH422G_OD_OUT_3      0x08 /**< OC3 salida nivel alto */
/**@}*/

/** @name Pines IO (push-pull outputs) */
/**@{*/
#define CH422G_IO_OUT_0      0x01 /**< IO0 salida nivel alto */
#define CH422G_IO_OUT_1      0x02 /**< IO1 salida nivel alto */
#define CH422G_IO_OUT_2      0x04 /**< IO2 salida nivel alto */
#define CH422G_IO_OUT_3      0x08 /**< IO3 salida nivel alto */
#define CH422G_IO_OUT_4      0x10 /**< IO4 salida nivel alto */
#define CH422G_IO_OUT_5      0x20 /**< IO5 salida nivel alto */
#define CH422G_IO_OUT_6      0x40 /**< IO6 salida nivel alto */
#define CH422G_IO_OUT_7      0x80 /**< IO7 salida nivel alto */
/**@}*/

/** @name Pines IO con asignación de función sugerida */
/**@{*/
#define CH422G_IO_0      0x01 /**< IO0 - Entrada digital (DI0) */
#define CH422G_IO_1      0x02 /**< IO1 - Reset táctil */
#define CH422G_IO_2      0x04 /**< IO2 - Control de retroiluminación */
#define CH422G_IO_3      0x08 /**< IO3 - Reset del LCD */
#define CH422G_IO_4      0x10 /**< IO4 - CS de tarjeta SD */
#define CH422G_IO_5      0x20 /**< IO5 - Entrada digital (DI1) */
#define CH422G_IO_6      0x40 /**< IO6 - Disponible para uso general */
#define CH422G_IO_7      0x80 /**< IO7 - Disponible para uso general */
/**@}*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Escribe un valor en un registro de salida del CH422G.
 * 
 * @param addr Dirección del registro de función.
 * @param data Valor a escribir.
 * @return esp_err_t ESP_OK si fue exitoso, o un error de I2C.
 */
esp_err_t write_output_reg(uint8_t addr, uint8_t data);

/**
 * @brief Configura el CH422G en modo push-pull y establece el estado de los pines IO.
 * 
 * @param pin Máscara de bits para los pines a controlar.
 * @return esp_err_t Resultado de la operación I2C.
 */
esp_err_t CH422G_io_output(uint8_t pin);

/**
 * @brief Lee el estado de uno o más pines IO configurados como entrada.
 * 
 * @param pin Máscara con los pines a consultar.
 * @return uint8_t Valor lógico leído (bit a bit).
 */
uint8_t CH422G_io_input(uint8_t pin);

/**
 * @brief Configura el CH422G en modo open-drain (OD) y escribe el estado de los pines OC.
 * 
 * @param pin Máscara con los pines a nivel alto.
 * @return esp_err_t Resultado de la operación I2C.
 */
esp_err_t CH422G_od_output(uint8_t pin);

/**
 * @brief Inicializa el CH422G en modo push-pull si aún no ha sido configurado.
 * 
 * Esta función debe llamarse una vez al iniciar el sistema, antes de utilizar los pines IO.
 */
void CH422G_EnsurePushPullMode(void);

#ifdef __cplusplus
}
#endif

#endif /* _CH422G_H_ */
