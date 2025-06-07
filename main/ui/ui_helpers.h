/**
 * @file ui_helpers.h
 * @brief Archivo de utilidades para la interfaz de usuario generado por SquareLine Studio
 * @details Este archivo contiene funciones y macros auxiliares para la manipulación de elementos de la UI
 * @version SquareLine Studio 1.5.1
 * @version LVGL 8.3.11
 * @project UI_draft
 */

#ifndef _UI_DRAFT_UI_HELPERS_H
#define _UI_DRAFT_UI_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui.h"

/** @brief Tamaño del buffer temporal para strings */
#define _UI_TEMPORARY_STRING_BUFFER_SIZE 32

/** @brief Propiedades de la barra de progreso */
#define _UI_BAR_PROPERTY_VALUE 0
#define _UI_BAR_PROPERTY_VALUE_WITH_ANIM 1

/**
 * @brief Establece una propiedad de la barra de progreso
 * @param target Objeto barra de progreso
 * @param id ID de la propiedad
 * @param val Valor a establecer
 */
void _ui_bar_set_property(lv_obj_t * target, int id, int val);

/** @brief Propiedades básicas de los objetos */
#define _UI_BASIC_PROPERTY_POSITION_X 0
#define _UI_BASIC_PROPERTY_POSITION_Y 1
#define _UI_BASIC_PROPERTY_WIDTH 2
#define _UI_BASIC_PROPERTY_HEIGHT 3

/**
 * @brief Establece una propiedad básica de un objeto
 * @param target Objeto objetivo
 * @param id ID de la propiedad
 * @param val Valor a establecer
 */
void _ui_basic_set_property(lv_obj_t * target, int id, int val);

/** @brief Propiedades del dropdown */
#define _UI_DROPDOWN_PROPERTY_SELECTED 0

/**
 * @brief Establece una propiedad del dropdown
 * @param target Objeto dropdown
 * @param id ID de la propiedad
 * @param val Valor a establecer
 */
void _ui_dropdown_set_property(lv_obj_t * target, int id, int val);

/** @brief Propiedades de la imagen */
#define _UI_IMAGE_PROPERTY_IMAGE 0

/**
 * @brief Establece una propiedad de la imagen
 * @param target Objeto imagen
 * @param id ID de la propiedad
 * @param val Valor a establecer
 */
void _ui_image_set_property(lv_obj_t * target, int id, uint8_t * val);

/** @brief Propiedades de la etiqueta */
#define _UI_LABEL_PROPERTY_TEXT 0

/**
 * @brief Establece una propiedad de la etiqueta
 * @param target Objeto etiqueta
 * @param id ID de la propiedad
 * @param val Valor a establecer
 */
void _ui_label_set_property(lv_obj_t * target, int id, const char * val);

/** @brief Propiedades del roller */
#define _UI_ROLLER_PROPERTY_SELECTED 0
#define _UI_ROLLER_PROPERTY_SELECTED_WITH_ANIM 1

/**
 * @brief Establece una propiedad del roller
 * @param target Objeto roller
 * @param id ID de la propiedad
 * @param val Valor a establecer
 */
void _ui_roller_set_property(lv_obj_t * target, int id, int val);

/** @brief Propiedades del slider */
#define _UI_SLIDER_PROPERTY_VALUE 0
#define _UI_SLIDER_PROPERTY_VALUE_WITH_ANIM 1

/**
 * @brief Establece una propiedad del slider
 * @param target Objeto slider
 * @param id ID de la propiedad
 * @param val Valor a establecer
 */
void _ui_slider_set_property(lv_obj_t * target, int id, int val);

/**
 * @brief Cambia la pantalla actual
 * @param target Puntero al objeto pantalla
 * @param fademode Modo de transición
 * @param spd Velocidad de la transición
 * @param delay Retardo antes de la transición
 * @param target_init Función de inicialización de la pantalla objetivo
 */
void _ui_screen_change(lv_obj_t ** target, lv_scr_load_anim_t fademode, int spd, int delay, void (*target_init)(void));

/**
 * @brief Elimina una pantalla
 * @param target Puntero al objeto pantalla a eliminar
 */
void _ui_screen_delete(lv_obj_t ** target);

/**
 * @brief Incrementa el valor de un arco
 * @param target Objeto arco
 * @param val Valor a incrementar
 */
void _ui_arc_increment(lv_obj_t * target, int val);

/**
 * @brief Incrementa el valor de una barra
 * @param target Objeto barra
 * @param val Valor a incrementar
 * @param anm Flag de animación
 */
void _ui_bar_increment(lv_obj_t * target, int val, int anm);

/**
 * @brief Incrementa el valor de un slider
 * @param target Objeto slider
 * @param val Valor a incrementar
 * @param anm Flag de animación
 */
void _ui_slider_increment(lv_obj_t * target, int val, int anm);

/**
 * @brief Establece el objetivo del teclado
 * @param keyboard Objeto teclado
 * @param textarea Objeto textarea objetivo
 */
void _ui_keyboard_set_target(lv_obj_t * keyboard, lv_obj_t * textarea);

/** @brief Modificadores de flags */
#define _UI_MODIFY_FLAG_ADD 0
#define _UI_MODIFY_FLAG_REMOVE 1
#define _UI_MODIFY_FLAG_TOGGLE 2

/**
 * @brief Modifica los flags de un objeto
 * @param target Objeto objetivo
 * @param flag Flag a modificar
 * @param value Valor de modificación
 */
void _ui_flag_modify(lv_obj_t * target, int32_t flag, int value);

/** @brief Modificadores de estado */
#define _UI_MODIFY_STATE_ADD 0
#define _UI_MODIFY_STATE_REMOVE 1
#define _UI_MODIFY_STATE_TOGGLE 2

/**
 * @brief Modifica el estado de un objeto
 * @param target Objeto objetivo
 * @param state Estado a modificar
 * @param value Valor de modificación
 */
void _ui_state_modify(lv_obj_t * target, int32_t state, int value);

/** @brief Direcciones de movimiento del cursor */
#define UI_MOVE_CURSOR_UP 0
#define UI_MOVE_CURSOR_RIGHT 1
#define UI_MOVE_CURSOR_DOWN 2
#define UI_MOVE_CURSOR_LEFT 3

/**
 * @brief Mueve el cursor en un textarea
 * @param target Objeto textarea
 * @param val Dirección del movimiento
 */
void _ui_textarea_move_cursor(lv_obj_t * target, int val);

/**
 * @brief Callback para eliminar pantallas no cargadas
 * @param e Evento que disparó el callback
 */
void scr_unloaded_delete_cb(lv_event_t * e);

/**
 * @brief Establece la opacidad de un objeto
 * @param target Objeto objetivo
 * @param val Valor de opacidad
 */
void _ui_opacity_set(lv_obj_t * target, int val);

/**
 * @brief Estructura para datos de usuario en animaciones
 */
typedef struct _ui_anim_user_data_t {
    lv_obj_t * target;          /**< Objeto objetivo de la animación */
    lv_img_dsc_t ** imgset;     /**< Conjunto de imágenes */
    int32_t imgset_size;        /**< Tamaño del conjunto de imágenes */
    int32_t val;                /**< Valor de la animación */
} ui_anim_user_data_t;

/**
 * @brief Libera los datos de usuario de una animación
 * @param a Animación
 */
void _ui_anim_callback_free_user_data(lv_anim_t * a);

/**
 * @brief Callback para establecer la coordenada X en una animación
 * @param a Animación
 * @param v Valor de la coordenada X
 */
void _ui_anim_callback_set_x(lv_anim_t * a, int32_t v);

/**
 * @brief Callback para establecer la coordenada Y en una animación
 * @param a Animación
 * @param v Valor de la coordenada Y
 */
void _ui_anim_callback_set_y(lv_anim_t * a, int32_t v);

/**
 * @brief Callback para establecer el ancho en una animación
 * @param a Animación
 * @param v Valor del ancho
 */
void _ui_anim_callback_set_width(lv_anim_t * a, int32_t v);

/**
 * @brief Callback para establecer el alto en una animación
 * @param a Animación
 * @param v Valor del alto
 */
void _ui_anim_callback_set_height(lv_anim_t * a, int32_t v);

/**
 * @brief Callback para establecer la opacidad en una animación
 * @param a Animación
 * @param v Valor de la opacidad
 */
void _ui_anim_callback_set_opacity(lv_anim_t * a, int32_t v);

/**
 * @brief Callback para establecer el zoom de imagen en una animación
 * @param a Animación
 * @param v Valor del zoom
 */
void _ui_anim_callback_set_image_zoom(lv_anim_t * a, int32_t v);

/**
 * @brief Callback para establecer el ángulo de imagen en una animación
 * @param a Animación
 * @param v Valor del ángulo
 */
void _ui_anim_callback_set_image_angle(lv_anim_t * a, int32_t v);

/**
 * @brief Callback para establecer el frame de imagen en una animación
 * @param a Animación
 * @param v Valor del frame
 */
void _ui_anim_callback_set_image_frame(lv_anim_t * a, int32_t v);

/**
 * @brief Obtiene la coordenada X de una animación
 * @param a Animación
 * @return Valor de la coordenada X
 */
int32_t _ui_anim_callback_get_x(lv_anim_t * a);

/**
 * @brief Obtiene la coordenada Y de una animación
 * @param a Animación
 * @return Valor de la coordenada Y
 */
int32_t _ui_anim_callback_get_y(lv_anim_t * a);

/**
 * @brief Obtiene el ancho de una animación
 * @param a Animación
 * @return Valor del ancho
 */
int32_t _ui_anim_callback_get_width(lv_anim_t * a);

/**
 * @brief Obtiene el alto de una animación
 * @param a Animación
 * @return Valor del alto
 */
int32_t _ui_anim_callback_get_height(lv_anim_t * a);

/**
 * @brief Obtiene la opacidad de una animación
 * @param a Animación
 * @return Valor de la opacidad
 */
int32_t _ui_anim_callback_get_opacity(lv_anim_t * a);

/**
 * @brief Obtiene el zoom de imagen de una animación
 * @param a Animación
 * @return Valor del zoom
 */
int32_t _ui_anim_callback_get_image_zoom(lv_anim_t * a);

/**
 * @brief Obtiene el ángulo de imagen de una animación
 * @param a Animación
 * @return Valor del ángulo
 */
int32_t _ui_anim_callback_get_image_angle(lv_anim_t * a);

/**
 * @brief Obtiene el frame de imagen de una animación
 * @param a Animación
 * @return Valor del frame
 */
int32_t _ui_anim_callback_get_image_frame(lv_anim_t * a);

/**
 * @brief Establece el valor de texto de un arco
 * @param trg Objeto objetivo
 * @param src Objeto fuente
 * @param prefix Prefijo del texto
 * @param postfix Sufijo del texto
 */
void _ui_arc_set_text_value(lv_obj_t * trg, lv_obj_t * src, const char * prefix, const char * postfix);

/**
 * @brief Establece el valor de texto de un slider
 * @param trg Objeto objetivo
 * @param src Objeto fuente
 * @param prefix Prefijo del texto
 * @param postfix Sufijo del texto
 */
void _ui_slider_set_text_value(lv_obj_t * trg, lv_obj_t * src, const char * prefix, const char * postfix);

/**
 * @brief Establece el valor de texto de un checkbox
 * @param trg Objeto objetivo
 * @param src Objeto fuente
 * @param txt_on Texto cuando está activado
 * @param txt_off Texto cuando está desactivado
 */
void _ui_checked_set_text_value(lv_obj_t * trg, lv_obj_t * src, const char * txt_on, const char * txt_off);

/**
 * @brief Incrementa el valor de un spinbox
 * @param target Objeto spinbox
 * @param val Valor a incrementar
 */
void _ui_spinbox_step(lv_obj_t * target, int val);

/**
 * @brief Cambia el tema de la interfaz
 * @param val Valor del tema
 */
void _ui_switch_theme(int val);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
