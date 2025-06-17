/**
 * @file wifi_manager.h
 * @brief Gestor de WiFi para el horno de vacío
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "lvgl.h"

/**
 * @brief Inicializa el stack de WiFi, escanea redes y sincroniza la hora vía SNTP
 * 
 * @param dropdown Elemento dropdown de LVGL para mostrar las redes WiFi
 * @param datetime_label Elemento label de LVGL para mostrar la fecha y hora (puede ser NULL)
 * @return esp_err_t ESP_OK si la inicialización fue exitosa
 * 
 * @note El parámetro datetime_label se mantiene por compatibilidad pero se recomienda
 *       usar el módulo statusbar_manager para gestión de la hora en la barra de estado
 */
esp_err_t wifi_manager_init(lv_obj_t *dropdown, lv_obj_t *datetime_label);

#endif /* WIFI_MANAGER_H */ 