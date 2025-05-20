/**
 * @file ui_comp_statusbar.h
 * @brief Archivo de cabecera para el componente STATUSBAR generado por SquareLine Studio.
 * 
 * Este archivo contiene las declaraciones y definiciones necesarias para el componente STATUSBAR,
 * que es una barra de estado diseñada para mostrar información como la fecha/hora y varios íconos.
 * Fue generado automáticamente por SquareLine Studio versión 1.5.1 y está diseñado para ser utilizado
 * con LVGL versión 8.3.11 en el proyecto "UI_draft".
 * 
 * SPDX-FileCopyrightText: Generado por SquareLine Studio
 * SPDX-License-Identifier: Propietario (default)
 */

#ifndef _UI_COMP_STATUSBAR_H
#define _UI_COMP_STATUSBAR_H

#include "../ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Índices para acceder a los elementos secundarios del componente STATUSBAR.
 * 
 * Estas macros definen los índices de los elementos secundarios del componente STATUSBAR,
 * como la barra principal, la fecha/hora y los íconos. Se utilizan para identificar y acceder
 * a los objetos secundarios en callbacks o funciones personalizadas.
 */
#define UI_COMP_STATUSBAR_STATUSBAR         0 ///< Índice del objeto principal STATUSBAR.
#define UI_COMP_STATUSBAR_DATETIME          1 ///< Índice del contenedor de fecha/hora.
#define UI_COMP_STATUSBAR_DATETIME_DATETIME1 2 ///< Índice de la etiqueta de fecha/hora.
#define UI_COMP_STATUSBAR_ICONS             3 ///< Índice del contenedor de íconos.
#define UI_COMP_STATUSBAR_ICONS_ICONUD      4 ///< Índice del ícono de actualización.
#define UI_COMP_STATUSBAR_ICONS_ICONHT      5 ///< Índice del ícono de calefacción.
#define UI_COMP_STATUSBAR_ICONS_ICONBT      6 ///< Índice del ícono de batería.
#define UI_COMP_STATUSBAR_ICONS_ICONWIFI    7 ///< Índice del ícono de WiFi.
#define UI_COMP_STATUSBAR_ICONS_ICONWARN    8 ///< Índice del ícono de advertencia.

/**
 * @brief Número total de elementos secundarios en el componente STATUSBAR.
 */
#define _UI_COMP_STATUSBAR_NUM              9 ///< Número total de elementos secundarios.

/**
 * @brief Crea y configura el componente STATUSBAR.
 * 
 * Esta función crea un objeto STATUSBAR y sus elementos secundarios (fecha/hora e íconos),
 * aplicando estilos y configuraciones específicas. También registra callbacks para manejar eventos.
 * 
 * @param comp_parent Puntero al objeto padre donde se creará el STATUSBAR.
 * @return lv_obj_t* Puntero al objeto STATUSBAR creado.
 */
lv_obj_t *ui_STATUSBAR_create(lv_obj_t *comp_parent);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _UI_COMP_STATUSBAR_H