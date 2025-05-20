/**
 * @file ui_comp.c
 * @brief Implementación de funciones para manejar componentes generados por SquareLine Studio.
 * 
 * Este archivo contiene funciones para obtener y eliminar componentes secundarios de un componente padre
 * generado por SquareLine Studio. También incluye callbacks para manejar eventos relacionados con estos
 * componentes. Fue generado automáticamente por SquareLine Studio versión 1.5.1 y está diseñado para ser
 * utilizado con LVGL versión 8.3.11 en el proyecto "UI_draft".
 * 
 * SPDX-FileCopyrightText: Generado por SquareLine Studio
 * SPDX-License-Identifier: Propietario (default)
 */

#include "../ui.h"
#include "../ui_helpers.h"
#include "ui_comp.h"

/**
 * @brief Evento personalizado para obtener un componente secundario.
 */
uint32_t LV_EVENT_GET_COMP_CHILD;

/**
 * @brief Estructura para almacenar información sobre un componente secundario.
 */
typedef struct {
    uint32_t child_idx; ///< Índice del componente secundario.
    lv_obj_t *child;    ///< Puntero al componente secundario.
} ui_comp_get_child_t;

/**
 * @brief Obtiene un componente secundario de un componente padre.
 * 
 * Esta función envía un evento personalizado `LV_EVENT_GET_COMP_CHILD` al componente padre para obtener
 * un puntero al componente secundario especificado por su índice.
 * 
 * @param comp Puntero al componente padre.
 * @param child_idx Índice del componente secundario que se desea obtener.
 * @return lv_obj_t* Puntero al componente secundario si se encuentra, o NULL si no existe.
 */
lv_obj_t *ui_comp_get_child(lv_obj_t *comp, uint32_t child_idx)
{
    ui_comp_get_child_t info;
    info.child = NULL;       // Inicializar el puntero del componente secundario como NULL
    info.child_idx = child_idx; // Establecer el índice del componente secundario
    lv_event_send(comp, LV_EVENT_GET_COMP_CHILD, &info); // Enviar el evento para obtener el componente
    return info.child; // Devolver el puntero al componente secundario
}

/**
 * @brief Callback para manejar el evento de obtención de componentes secundarios.
 * 
 * Esta función se llama cuando se recibe el evento `LV_EVENT_GET_COMP_CHILD`. Obtiene el índice del
 * componente secundario solicitado y devuelve el puntero correspondiente.
 * 
 * @param e Puntero al objeto de evento de LVGL.
 */
void get_component_child_event_cb(lv_event_t *e)
{
    lv_obj_t **c = lv_event_get_user_data(e); // Obtener los datos del usuario (arreglo de punteros a componentes)
    ui_comp_get_child_t *info = lv_event_get_param(e); // Obtener los parámetros del evento
    info->child = c[info->child_idx]; // Asignar el puntero al componente secundario correspondiente
}

/**
 * @brief Callback para manejar el evento de eliminación de componentes secundarios.
 * 
 * Esta función se llama cuando se recibe el evento de eliminación. Libera la memoria asignada para el
 * arreglo de punteros a componentes secundarios.
 * 
 * @param e Puntero al objeto de evento de LVGL.
 */
void del_component_child_event_cb(lv_event_t *e)
{
    lv_obj_t **c = lv_event_get_user_data(e); // Obtener los datos del usuario (arreglo de punteros a componentes)
    lv_mem_free(c); // Liberar la memoria asignada para el arreglo
}