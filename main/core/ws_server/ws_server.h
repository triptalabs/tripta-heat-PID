#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa y arranca el servidor WebSocket.
 */
esp_err_t ws_server_start(void);

/**
 * Puerto configurado del servidor WebSocket (definido en network_config.h)
 */
#include "network_config.h"

/** Detiene el servidor WebSocket */
esp_err_t ws_server_stop(void);

#ifdef __cplusplus
}
#endif 