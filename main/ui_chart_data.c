/**
 * @file ui_chart_data.c
 * @brief Declaración e inicialización de datos de la gráfica de temperatura en LVGL.
 *
 * Este archivo define los objetos y buffers asociados a la gráfica de temperatura en
 * la interfaz gráfica del horno de vacío. Estos datos son accedidos y modificados desde
 * otras tareas, como la de lectura de temperatura (`sensor.c`) y visualización (`ui_events.c`).
 *
 * @version 1.0
 * @date 2024-01-27
 */

#include "ui_chart_data.h"

// ───────────────────────────────────────────────────────
// Variables públicas utilizadas por LVGL

lv_coord_t ui_Chart_series_1_array[240];   /**< Buffer de datos para la serie 1 (temperatura) */
lv_chart_series_t *ui_Chart_series_1;      /**< Puntero a la serie en la gráfica */
lv_obj_t *ui_Chart;                         /**< Objeto de gráfica en la interfaz LVGL */
