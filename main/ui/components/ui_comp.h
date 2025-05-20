/**
 * @file ui_comp.h
 * @brief Archivo de cabecera para funciones y eventos relacionados con componentes generados por SquareLine Studio.
 * 
 * Este archivo contiene las declaraciones de funciones y eventos necesarios para manejar componentes generados
 * automáticamente por SquareLine Studio. Fue generado para LVGL versión 8.3.11 en el proyecto "UI_draft".
 * 
 * SPDX-FileCopyrightText: Generado por SquareLine Studio
 * SPDX-License-Identifier: Propietario (default)
 */

#ifndef _UI_COMP__H
#define _UI_COMP__H

#include "../ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback para manejar el evento de obtención de componentes secundarios.
 * 
 * Esta función se llama cuando se recibe un evento para obtener un componente secundario.
 * Utiliza los datos del usuario y los parámetros del evento para proporcionar acceso a los componentes.
 * 
 * @param e Puntero al objeto de evento de LVGL.
 */
void get_component_child_event_cb(lv_event_t *e);

/**
 * @brief Callback para manejar el evento de eliminación de componentes secundarios.
 * 
 * Esta función se llama cuando se recibe un evento para eliminar componentes secundarios.
 * Libera la memoria asignada para los datos asociados a los componentes.
 * 
 * @param e Puntero al objeto de evento de LVGL.
 */
void del_component_child_event_cb(lv_event_t *e);

/**
 * @brief Obtiene un componente secundario de un componente padre.
 * 
 * Esta función permite obtener un puntero a un componente secundario específico utilizando su índice.
 * Envía un evento personalizado `LV_EVENT_GET_COMP_CHILD` al componente padre para obtener el puntero.
 * 
 * @param comp Puntero al componente padre.
 * @param child_idx Índice del componente secundario que se desea obtener.
 * @return lv_obj_t* Puntero al componente secundario si se encuentra, o NULL si no existe.
 */
lv_obj_t *ui_comp_get_child(lv_obj_t *comp, uint32_t child_idx);

/**
 * @brief Evento personalizado para obtener un componente secundario.
 * 
 * Este identificador representa un evento personalizado utilizado para solicitar un componente secundario
 * específico de un componente padre.
 */
extern uint32_t LV_EVENT_GET_COMP_CHILD;

// Incluye el archivo de cabecera para el componente STATUSBAR
#include "ui_comp_statusbar.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _UI_COMP__H