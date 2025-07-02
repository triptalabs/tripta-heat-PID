#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa y arranca el servidor WebSocket.
 */
esp_err_t ws_server_start(void);

#ifdef __cplusplus
}
#endif 