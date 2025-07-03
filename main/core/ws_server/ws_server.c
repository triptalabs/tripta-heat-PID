#include "ws_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor.h"
#include "pid_controller.h"
#include "cJSON.h"

static const char *TAG = "ws_server";
static httpd_handle_t s_server = NULL;
static TaskHandle_t s_broadcast_task = NULL;

/************** Helpers JSON **************/
static char *build_status_json(void)
{
    float temp = read_ema_temp();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "status");
    cJSON_AddNumberToObject(root, "temp", temp);
    cJSON_AddNumberToObject(root, "setpoint", 0.0f); // TODO: obtener setpoint real
    cJSON_AddBoolToObject(root, "pid_enabled", true); // TODO
    cJSON_AddBoolToObject(root, "ssr", pid_ssr_status());
    cJSON_AddBoolToObject(root, "alarm", false);
    char *str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str;
}

/************** Broadcast Task **************/
static void broadcast_task(void *arg)
{
    while (s_server) {
        size_t clients = CONFIG_LWIP_MAX_SOCKETS;
        int client_fds[CONFIG_LWIP_MAX_SOCKETS];
        httpd_get_client_list(s_server, &clients, client_fds);
        httpd_ws_frame_t frame = {
            .type = HTTPD_WS_TYPE_TEXT,
            .payload = NULL,
            .len = 0
        };
        char *payload = build_status_json();
        frame.payload = (uint8_t *)payload;
        frame.len = strlen(payload);

        for (size_t i = 0; i < clients; ++i) {
            httpd_ws_client_info_t info = httpd_ws_get_fd_info(s_server, client_fds[i]);
            if (info == HTTPD_WS_CLIENT_WEBSOCKET) {
                httpd_ws_send_frame_async(s_server, client_fds[i], &frame);
            }
        }
        free(payload);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

/************** WebSocket Handler **************/
static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done");
        return ESP_OK;
    }
    httpd_ws_frame_t frame = {0};
    frame.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ws recv frame failed: %s", esp_err_to_name(ret));
        return ret;
    }
    uint8_t *buf = calloc(1, frame.len + 1);
    frame.payload = buf;
    ret = httpd_ws_recv_frame(req, &frame, frame.len);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Received WS message: %s", (char *)frame.payload);
        // TODO: parse comando y ejecutar
    }
    free(buf);
    return ret;
}

/************** Server Start/Stop **************/
esp_err_t ws_server_start(void)
{
    if (s_server) return ESP_OK;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WS_SERVER_PORT;
    config.ctrl_port = 0; // deshabilitar control socket para ahorrar

    ESP_LOGI(TAG, "Iniciando servidor WS en puerto %d", config.server_port);
    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando httpd: %s", esp_err_to_name(ret));
        s_server = NULL;
        return ret;
    }

    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true
    };
    httpd_register_uri_handler(s_server, &ws_uri);

    xTaskCreate(broadcast_task, "ws_broadcast", 4096, NULL, 4, &s_broadcast_task);
    return ESP_OK;
}

esp_err_t ws_server_stop(void)
{
    if (!s_server) return ESP_OK;
    if (s_broadcast_task) {
        TaskHandle_t h = s_broadcast_task;
        s_broadcast_task = NULL;
        vTaskDelete(h);
    }
    esp_err_t r = httpd_stop(s_server);
    s_server = NULL;
    return r;
} 