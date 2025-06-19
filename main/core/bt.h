/**
 * @file bt.h
 * @brief Módulo de gestión Bluetooth BLE para TriptaLabs Heat Controller
 * 
 * Este módulo proporciona una interfaz simplificada para la gestión del stack
 * Bluetooth Low Energy (BLE) del ESP32. Incluye funcionalidades para:
 * - Inicialización y control del stack BLE
 * - Gestión del estado del módulo BT
 * - Configuración del nombre del dispositivo
 * - Monitoreo de conexiones BLE
 * 
 * @note Esta es una implementación simplificada que puede expandirse en el futuro
 *       para incluir funcionalidades BLE más avanzadas como GATT services,
 *       advertising customizado, y manejo de múltiples conexiones.
 * 
 * @author TriptaLabs
 * @version 1.0
 * @date 2024
 * 
 * @see Issue #35: Módulo Bluetooth BLE
 */

#ifndef BT_H
#define BT_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estados del módulo Bluetooth
 * 
 * Define los diferentes estados por los que puede pasar el módulo BT durante
 * su ciclo de vida. El estado se mantiene internamente y puede consultarse
 * mediante bt_get_state().
 */
typedef enum {
    BT_STATE_UNINITIALIZED = 0,  ///< Módulo no inicializado, stack BT deshabilitado
    BT_STATE_INITIALIZED,        ///< Stack BT inicializado pero servicio no iniciado
    BT_STATE_STARTED,           ///< Servicio BLE activo y disponible para conexiones
    BT_STATE_STOPPED            ///< Servicio BLE detenido pero stack aún inicializado
} bt_state_t;

/**
 * @brief Información de conexión BLE
 * 
 * Estructura que almacena información sobre la conexión BLE actual.
 * Se actualiza automáticamente cuando se establece o termina una conexión.
 * 
 * @note En la implementación actual, solo se soporta una conexión simultánea
 */
typedef struct {
    bool is_connected;     ///< Indica si hay una conexión BLE activa
    uint16_t conn_id;      ///< ID de la conexión (0 si no hay conexión)
    char remote_addr[18];  ///< Dirección MAC del dispositivo remoto (formato XX:XX:XX:XX:XX:XX)
} bt_connection_info_t;

/**
 * @brief Inicializa el módulo Bluetooth BLE
 * 
 * Realiza la inicialización completa del stack Bluetooth del ESP32:
 * 1. Libera memoria del controlador BT clásico (no utilizado)
 * 2. Inicializa el controlador BT en modo BLE
 * 3. Habilita el controlador BT
 * 4. Inicializa y habilita el stack Bluedroid
 * 
 * Esta función debe llamarse antes de usar cualquier otra función del módulo.
 * Es seguro llamarla múltiples veces (no-op si ya está inicializado).
 * 
 * @return ESP_OK si la inicialización fue exitosa
 * @return ESP_ERR_* códigos de error específicos del ESP-IDF si falla
 * 
 * @note Consume aproximadamente 64KB de RAM cuando está activo
 * @warning No llamar desde interrupciones
 */
esp_err_t bt_init(void);

/**
 * @brief Inicia el servicio Bluetooth BLE
 * 
 * Activa el servicio BLE haciendo que el dispositivo esté disponible para
 * conexiones entrantes. Si el módulo no está inicializado, lo inicializa
 * automáticamente.
 * 
 * En implementaciones futuras, esta función también:
 * - Configurará y iniciará la publicidad BLE
 * - Registrará servicios GATT
 * - Habilitará notificaciones
 * 
 * @return ESP_OK si el inicio fue exitoso
 * @return ESP_ERR_* códigos de error específicos si falla la inicialización automática
 * 
 * @see bt_init() para detalles de inicialización
 * @note Es seguro llamarla múltiples veces (no-op si ya está iniciado)
 */
esp_err_t bt_start(void);

/**
 * @brief Detiene el servicio Bluetooth BLE
 * 
 * Desactiva el servicio BLE y cierra todas las conexiones activas.
 * El stack BT permanece inicializado, permitiendo un reinicio rápido
 * con bt_start().
 * 
 * Acciones realizadas:
 * - Detiene la publicidad BLE (implementación futura)
 * - Cierra conexiones BLE activas
 * - Limpia información de conexión interna
 * - Cambia estado a BT_STATE_STOPPED
 * 
 * @return ESP_OK si se detuvo correctamente
 * @return ESP_OK si ya estaba detenido (no-op)
 * 
 * @note No desinicializa el stack completo, usar bt_deinit() para eso
 */
esp_err_t bt_stop(void);

/**
 * @brief Desinicializa el módulo Bluetooth BLE
 * 
 * Realiza una desinicialización completa del stack Bluetooth:
 * 1. Detiene el servicio BLE si está activo
 * 2. Deshabilita y desinicializa el stack Bluedroid
 * 3. Deshabilita y desinicializa el controlador BT
 * 4. Libera toda la memoria asociada
 * 
 * Después de esta llamada, es necesario llamar bt_init() nuevamente
 * para usar el módulo.
 * 
 * @return ESP_OK si la desinicialización fue exitosa
 * @return ESP_OK si ya estaba desinicializado (no-op)
 * 
 * @note Libera aproximadamente 64KB de RAM
 * @warning No llamar desde interrupciones
 */
esp_err_t bt_deinit(void);

/**
 * @brief Cambia el nombre del dispositivo Bluetooth
 * 
 * Establece un nuevo nombre para el dispositivo BT que será visible
 * durante el proceso de descubrimiento (advertising). El nombre se
 * guarda internamente y se aplica al stack BT si está inicializado.
 * 
 * @param name Nuevo nombre del dispositivo (no puede ser NULL o vacío)
 *             Máximo 32 caracteres (ESP_BT_GAP_MAX_BDNAME_LEN)
 * 
 * @return ESP_OK si el cambio fue exitoso
 * @return ESP_ERR_INVALID_ARG si el nombre es NULL, vacío o muy largo
 * @return ESP_ERR_* códigos de error del stack BT si falla la actualización
 * 
 * @note Si el módulo no está inicializado, el nombre se guarda y se aplicará
 *       cuando se llame a bt_init()
 * @note El nombre por defecto es "TriptaLabs-Heat"
 * 
 * @example
 * ```c
 * bt_set_device_name("Mi Calentador");
 * ```
 */
esp_err_t bt_set_device_name(const char* name);

/**
 * @brief Verifica si el módulo Bluetooth está habilitado
 * 
 * Consulta el estado interno del módulo para determinar si está
 * habilitado y funcionando (inicializado o en estado superior).
 * 
 * @return true si el módulo está habilitado (estado >= BT_STATE_INITIALIZED)
 * @return false si el módulo está desinicializado
 * 
 * @note Esta función no consulta directamente el hardware, sino el estado interno
 * @note Es seguro llamarla desde cualquier contexto
 */
bool bt_is_enabled(void);

/**
 * @brief Verifica si hay algún dispositivo conectado vía BLE
 * 
 * Consulta el estado interno de conexiones para determinar si
 * hay al menos un dispositivo BLE conectado actualmente.
 * 
 * @return true si hay al menos una conexión BLE activa
 * @return false si no hay conexiones activas
 * 
 * @note En la implementación actual solo se soporta una conexión simultánea
 * @note Es seguro llamarla desde cualquier contexto
 * 
 * @see bt_get_connection_info() para obtener detalles de la conexión
 */
bool bt_is_connected(void);

/**
 * @brief Obtiene el estado actual del módulo BT
 * 
 * Retorna el estado interno actual del módulo, útil para debugging
 * y lógica de aplicación que necesite conocer el estado exacto.
 * 
 * @return bt_state_t Estado actual del módulo
 * 
 * @see bt_state_t para descripción de los estados posibles
 * @note Es seguro llamarla desde cualquier contexto
 */
bt_state_t bt_get_state(void);

/**
 * @brief Obtiene información de la conexión actual
 * 
 * Copia la información de la conexión BLE activa al buffer proporcionado.
 * Incluye ID de conexión y dirección MAC del dispositivo remoto.
 * 
 * @param info Puntero a estructura donde se almacenará la información
 *             No puede ser NULL
 * 
 * @return ESP_OK si se obtuvo la información correctamente
 * @return ESP_FAIL si no hay conexión activa
 * @return ESP_ERR_INVALID_ARG si el puntero info es NULL
 * 
 * @note Solo válido cuando bt_is_connected() retorna true
 * 
 * @example
 * ```c
 * bt_connection_info_t conn_info;
 * if (bt_get_connection_info(&conn_info) == ESP_OK) {
 *     ESP_LOGI("APP", "Conectado a: %s", conn_info.remote_addr);
 * }
 * ```
 */
esp_err_t bt_get_connection_info(bt_connection_info_t* info);

/**
 * @brief Obtiene el nombre actual del dispositivo BT
 * 
 * Copia el nombre actual del dispositivo al buffer proporcionado.
 * El nombre retornado es el que se aplicó con bt_set_device_name()
 * o el nombre por defecto si no se ha cambiado.
 * 
 * @param name Buffer donde se almacenará el nombre (no puede ser NULL)
 * @param max_len Tamaño máximo del buffer incluyendo terminador nulo
 * 
 * @return ESP_OK si se obtuvo el nombre correctamente
 * @return ESP_ERR_INVALID_ARG si name es NULL o max_len es 0
 * 
 * @note El string resultante siempre incluye terminador nulo
 * @note Se recomienda un buffer de al menos 33 caracteres (32 + nulo)
 * 
 * @example
 * ```c
 * char device_name[33];
 * if (bt_get_device_name(device_name, sizeof(device_name)) == ESP_OK) {
 *     ESP_LOGI("APP", "Nombre del dispositivo: %s", device_name);
 * }
 * ```
 */
esp_err_t bt_get_device_name(char* name, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // BT_H 