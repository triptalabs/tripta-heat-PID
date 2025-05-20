/**
 * @file ui_comp_hook.h
 * @brief Archivo de cabecera para hooks personalizados de componentes generados por SquareLine Studio.
 * 
 * Este archivo contiene las declaraciones de funciones de hook para personalizar componentes generados
 * automáticamente por SquareLine Studio. Los hooks permiten agregar código personalizado después de la
 * creación de un componente en LVGL.
 * 
 * @note Este archivo fue generado automáticamente por SquareLine Studio versión 1.5.1.
 * Está diseñado para ser utilizado con LVGL versión 8.3.11 en el proyecto "UI_draft".
 * 
 * SPDX-FileCopyrightText: Generado por SquareLine Studio
 * SPDX-License-Identifier: Propietario (default)
 */

#ifndef _UI_DRAFT_UI_COMP_HOOK_H
#define _UI_DRAFT_UI_COMP_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Función de hook para el componente STATUSBAR.
 * 
 * Esta función se llama automáticamente después de la creación del componente STATUSBAR.
 * Se puede utilizar para agregar inicializaciones personalizadas, configurar estilos,
 * adjuntar eventos o modificar propiedades del componente.
 * 
 * @param comp Puntero al objeto STATUSBAR (lv_obj_t).
 * 
 * @note La implementación de esta función debe proporcionarse en el archivo de código fuente correspondiente.
 */
void ui_comp_STATUSBAR_create_hook(lv_obj_t *comp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _UI_DRAFT_UI_COMP_HOOK_H