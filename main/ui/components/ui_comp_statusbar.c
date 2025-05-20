/**
 * @file ui_STATUSBAR.c
 * @brief Implementación del componente STATUSBAR generado por SquareLine Studio.
 * 
 * Este archivo contiene la implementación del componente STATUSBAR, que es una barra de estado
 * diseñada para mostrar información como la fecha/hora y varios íconos (WiFi, advertencias, etc.).
 * Fue generado automáticamente por SquareLine Studio versión 1.5.1 y está diseñado para ser utilizado
 * con LVGL versión 8.3.11 en el proyecto "UI_draft".
 * 
 * SPDX-FileCopyrightText: Generado por SquareLine Studio
 * SPDX-License-Identifier: Propietario (default)
 */

#include "../ui.h"

/**
 * @brief Crea y configura el componente STATUSBAR.
 * 
 * Esta función crea un objeto STATUSBAR y sus elementos secundarios (fecha/hora e íconos),
 * aplicando estilos y configuraciones específicas. También registra callbacks para manejar eventos.
 * 
 * @param comp_parent Puntero al objeto padre donde se creará el STATUSBAR.
 * @return lv_obj_t* Puntero al objeto STATUSBAR creado.
 */
lv_obj_t *ui_STATUSBAR_create(lv_obj_t *comp_parent)
{
    // Crear el objeto principal STATUSBAR
    lv_obj_t *cui_STATUSBAR;
    cui_STATUSBAR = lv_obj_create(comp_parent);
    lv_obj_remove_style_all(cui_STATUSBAR); // Eliminar todos los estilos predeterminados
    lv_obj_set_width(cui_STATUSBAR, 1024);  // Ancho del STATUSBAR
    lv_obj_set_height(cui_STATUSBAR, 50);   // Altura del STATUSBAR
    lv_obj_set_x(cui_STATUSBAR, 0);         // Posición X
    lv_obj_set_y(cui_STATUSBAR, -275);      // Posición Y
    lv_obj_set_align(cui_STATUSBAR, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_set_flex_flow(cui_STATUSBAR, LV_FLEX_FLOW_COLUMN_WRAP); // Flujo de diseño flexible
    lv_obj_set_flex_align(cui_STATUSBAR, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); // Alineación flexible
    lv_obj_clear_flag(cui_STATUSBAR, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); // Desactivar clics y desplazamiento
    lv_obj_set_style_bg_color(cui_STATUSBAR, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT); // Color de fondo
    lv_obj_set_style_bg_opa(cui_STATUSBAR, 255, LV_PART_MAIN | LV_STATE_DEFAULT); // Opacidad del fondo

    // Crear el contenedor para la fecha/hora
    lv_obj_t *cui_Datetime;
    cui_Datetime = lv_obj_create(cui_STATUSBAR);
    lv_obj_remove_style_all(cui_Datetime); // Eliminar todos los estilos predeterminados
    lv_obj_set_height(cui_Datetime, 50);   // Altura del contenedor
    lv_obj_set_width(cui_Datetime, lv_pct(50)); // Ancho relativo (50%)
    lv_obj_set_x(cui_Datetime, 29);        // Posición X
    lv_obj_set_y(cui_Datetime, 2);         // Posición Y
    lv_obj_set_align(cui_Datetime, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_set_flex_flow(cui_Datetime, LV_FLEX_FLOW_ROW); // Flujo de diseño flexible
    lv_obj_set_flex_align(cui_Datetime, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); // Alineación flexible
    lv_obj_clear_flag(cui_Datetime, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); // Desactivar clics y desplazamiento

    // Crear la etiqueta para la fecha/hora
    cui_datetime1 = lv_label_create(cui_Datetime); // Usa la variable global directamente
    lv_obj_set_width(cui_datetime1, LV_SIZE_CONTENT); // Ancho ajustado al contenido
    lv_obj_set_height(cui_datetime1, LV_SIZE_CONTENT); // Altura ajustada al contenido
    lv_obj_set_align(cui_datetime1, LV_ALIGN_CENTER); // Alinear al centro
    lv_label_set_text(cui_datetime1, "18 mar 2025   |   10:35 AM"); // Texto inicial
    lv_obj_set_style_text_color(cui_datetime1, lv_color_hex(0xEEEEEE), LV_PART_MAIN | LV_STATE_DEFAULT); // Color del texto
    lv_obj_set_style_text_opa(cui_datetime1, 255, LV_PART_MAIN | LV_STATE_DEFAULT); // Opacidad del texto
    lv_obj_set_style_text_font(cui_datetime1, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT); // Fuente del texto

    // Crear el contenedor para los íconos
    lv_obj_t *cui_icons;
    cui_icons = lv_obj_create(cui_STATUSBAR);
    lv_obj_remove_style_all(cui_icons); // Eliminar todos los estilos predeterminados
    lv_obj_set_height(cui_icons, 50);   // Altura del contenedor
    lv_obj_set_width(cui_icons, lv_pct(50)); // Ancho relativo (50%)
    lv_obj_set_align(cui_icons, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_set_flex_flow(cui_icons, LV_FLEX_FLOW_ROW); // Flujo de diseño flexible
    lv_obj_set_flex_align(cui_icons, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); // Alineación flexible
    lv_obj_clear_flag(cui_icons, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); // Desactivar clics y desplazamiento
    lv_obj_set_style_pad_left(cui_icons, 0, LV_PART_MAIN | LV_STATE_DEFAULT); // Relleno izquierdo
    lv_obj_set_style_pad_right(cui_icons, 30, LV_PART_MAIN | LV_STATE_DEFAULT); // Relleno derecho
    lv_obj_set_style_pad_top(cui_icons, 0, LV_PART_MAIN | LV_STATE_DEFAULT); // Relleno superior
    lv_obj_set_style_pad_bottom(cui_icons, 0, LV_PART_MAIN | LV_STATE_DEFAULT); // Relleno inferior
    lv_obj_set_style_pad_row(cui_icons, 0, LV_PART_MAIN | LV_STATE_DEFAULT); // Espaciado entre filas
    lv_obj_set_style_pad_column(cui_icons, 30, LV_PART_MAIN | LV_STATE_DEFAULT); // Espaciado entre columnas

    // Crear los íconos individuales
    lv_obj_t *cui_iconud = lv_img_create(cui_icons);
    lv_img_set_src(cui_iconud, &ui_img_iconupdate_png); // Establecer imagen del ícono
    lv_obj_set_width(cui_iconud, LV_SIZE_CONTENT); // Ancho ajustado al contenido
    lv_obj_set_height(cui_iconud, LV_SIZE_CONTENT); // Altura ajustada al contenido
    lv_obj_set_align(cui_iconud, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_add_flag(cui_iconud, LV_OBJ_FLAG_ADV_HITTEST); // Activar pruebas avanzadas de clic
    lv_obj_clear_flag(cui_iconud, LV_OBJ_FLAG_SCROLLABLE); // Desactivar desplazamiento

    lv_obj_t *cui_iconht = lv_img_create(cui_icons);
    lv_img_set_src(cui_iconht, &ui_img_iconheating_png); // Establecer imagen del ícono
    lv_obj_set_width(cui_iconht, LV_SIZE_CONTENT); // Ancho ajustado al contenido
    lv_obj_set_height(cui_iconht, LV_SIZE_CONTENT); // Altura ajustada al contenido
    lv_obj_set_align(cui_iconht, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_add_flag(cui_iconht, LV_OBJ_FLAG_ADV_HITTEST); // Activar pruebas avanzadas de clic
    lv_obj_clear_flag(cui_iconht, LV_OBJ_FLAG_SCROLLABLE); // Desactivar desplazamiento

    lv_obj_t *cui_iconbt = lv_img_create(cui_icons);
    lv_img_set_src(cui_iconbt, &ui_img_iconbt_png); // Establecer imagen del ícono
    lv_obj_set_width(cui_iconbt, LV_SIZE_CONTENT); // Ancho ajustado al contenido
    lv_obj_set_height(cui_iconbt, LV_SIZE_CONTENT); // Altura ajustada al contenido
    lv_obj_set_align(cui_iconbt, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_add_flag(cui_iconbt, LV_OBJ_FLAG_ADV_HITTEST); // Activar pruebas avanzadas de clic
    lv_obj_clear_flag(cui_iconbt, LV_OBJ_FLAG_SCROLLABLE); // Desactivar desplazamiento

    lv_obj_t *cui_iconwifi = lv_img_create(cui_icons);
    lv_img_set_src(cui_iconwifi, &ui_img_iconwifi_png); // Establecer imagen del ícono
    lv_obj_set_width(cui_iconwifi, LV_SIZE_CONTENT); // Ancho ajustado al contenido
    lv_obj_set_height(cui_iconwifi, LV_SIZE_CONTENT); // Altura ajustada al contenido
    lv_obj_set_align(cui_iconwifi, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_add_flag(cui_iconwifi, LV_OBJ_FLAG_ADV_HITTEST); // Activar pruebas avanzadas de clic
    lv_obj_clear_flag(cui_iconwifi, LV_OBJ_FLAG_SCROLLABLE); // Desactivar desplazamiento

    lv_obj_t *cui_iconwarn = lv_img_create(cui_icons);
    lv_img_set_src(cui_iconwarn, &ui_img_iconwarn_png); // Establecer imagen del ícono
    lv_obj_set_width(cui_iconwarn, LV_SIZE_CONTENT); // Ancho ajustado al contenido
    lv_obj_set_height(cui_iconwarn, LV_SIZE_CONTENT); // Altura ajustada al contenido
    lv_obj_set_align(cui_iconwarn, LV_ALIGN_CENTER); // Alinear al centro
    lv_obj_add_flag(cui_iconwarn, LV_OBJ_FLAG_ADV_HITTEST); // Activar pruebas avanzadas de clic
    lv_obj_clear_flag(cui_iconwarn, LV_OBJ_FLAG_SCROLLABLE); // Desactivar desplazamiento

    // Almacenar punteros a los objetos secundarios para facilitar el acceso
    lv_obj_t **children = lv_mem_alloc(sizeof(lv_obj_t *) * _UI_COMP_STATUSBAR_NUM);
    children[UI_COMP_STATUSBAR_STATUSBAR] = cui_STATUSBAR;
    children[UI_COMP_STATUSBAR_DATETIME] = cui_Datetime;
    children[UI_COMP_STATUSBAR_DATETIME_DATETIME1] = cui_datetime1;
    children[UI_COMP_STATUSBAR_ICONS] = cui_icons;
    children[UI_COMP_STATUSBAR_ICONS_ICONUD] = cui_iconud;
    children[UI_COMP_STATUSBAR_ICONS_ICONHT] = cui_iconht;
    children[UI_COMP_STATUSBAR_ICONS_ICONBT] = cui_iconbt;
    children[UI_COMP_STATUSBAR_ICONS_ICONWIFI] = cui_iconwifi;
    children[UI_COMP_STATUSBAR_ICONS_ICONWARN] = cui_iconwarn;

    // Registrar callbacks para manejar eventos
    lv_obj_add_event_cb(cui_STATUSBAR, get_component_child_event_cb, LV_EVENT_GET_COMP_CHILD, children); // Callback para obtener hijos
    lv_obj_add_event_cb(cui_STATUSBAR, del_component_child_event_cb, LV_EVENT_DELETE, children); // Callback para eliminar hijos

    // Llamar al hook personalizado para el componente STATUSBAR
    ui_comp_STATUSBAR_create_hook(cui_STATUSBAR);

    return cui_STATUSBAR; // Devolver el objeto STATUSBAR creado
}