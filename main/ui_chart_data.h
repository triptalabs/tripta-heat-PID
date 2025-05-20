/**
 * @file ui_chart_data.h
 * @brief Declaraciones de datos y objetos asociados a la gráfica de temperatura en LVGL.
 *
 * Este archivo contiene las variables necesarias para actualizar y visualizar
 * la gráfica de temperatura en la interfaz gráfica del horno de vacío.
 * Las variables son compartidas entre `ui.c`, `sensor.c` y `ui_events.c`.
 *
 * @version 1.0
 * @date 2024-01-27
 */

#ifndef UI_CHART_DATA_H
#define UI_CHART_DATA_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Arreglo circular que almacena los valores de temperatura suavizados (EMA) para la serie 1.
 * 
 * Este buffer tiene una longitud fija de 240 puntos, y se actualiza periódicamente desde la tarea
 * de lectura de temperatura. Se utiliza junto con `lv_chart_set_ext_y_array`.
 */
extern lv_coord_t ui_Chart_series_1_array[240];

/**
 * @brief Puntero a la serie principal del gráfico (`lv_chart_series_t`).
 * 
 * Esta serie representa la temperatura en grados Celsius. Es creada y asociada a `ui_Chart` desde
 * la función `ui_init()` generada por SquareLine Studio.
 */
extern lv_chart_series_t *ui_Chart_series_1;

/**
 * @brief Objeto LVGL que representa la gráfica de temperatura en pantalla.
 * 
 * Este puntero se asigna al objeto tipo `lv_chart` creado por la UI (`ui_Chart` en SquareLine Studio).
 */
extern lv_obj_t *ui_Chart;

#ifdef __cplusplus
}
#endif

#endif // UI_CHART_DATA_H
