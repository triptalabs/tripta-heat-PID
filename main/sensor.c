/**
 * @file sensor.c
 * @brief Lectura de temperatura vía Modbus RTU usando UART y visualización en LVGL.
 *
 * Este módulo configura el puerto UART en modo RS485 half-duplex, realiza la comunicación
 * Modbus con un esclavo, obtiene la temperatura, aplica un filtro EMA y actualiza una
 * gráfica en la interfaz táctil mediante LVGL.
 *
 * @version 1.0
 * @date 2024-01-27
 */

#include "mb.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui_events.h"
#include "ui.h"
#include "ui_chart_data.h"

// ───────────────────────────────────────────────────────
// Objetos y constantes externas

extern lv_obj_t *ui_Chart;
extern lv_chart_series_t *ui_Chart_series_1;
extern lv_coord_t ui_Chart_series_1_array[240];

#define UART_PORT       UART_NUM_1      ///< Puerto UART utilizado
#define UART_TXD        44              ///< Pin TXD (también DE/RE en RS485)
#define UART_RXD        43              ///< Pin RXD
#define TAG             "MODBUS"        ///< Etiqueta para logs
#define MODBUS_SLAVE_ID 1               ///< ID del esclavo Modbus
#define TEMPERATURE_REGISTER 0x0000     ///< Registro que contiene la temperatura

// ───────────────────────────────────────────────────────
// Variables de estado

static float ema_temperature = 0.0f;
static const float alpha = 0.15f;    ///< Factor de suavizado para filtro EMA

#define TEMP_BUFFER_SIZE 240
static float temp_buffer[TEMP_BUFFER_SIZE] = {0};  ///< Buffer circular para gráfica
static int temp_index = 0;                          ///< Índice del buffer

// ───────────────────────────────────────────────────────
// Funciones internas

/**
 * @brief Actualiza la gráfica de temperatura en la interfaz.
 *
 * Copia los datos del buffer circular `temp_buffer` al arreglo usado por `LVGL`.
 */
void actualizar_grafica_temp() {
    int i, idx;

    for (i = 0; i < TEMP_BUFFER_SIZE; i++) {
        idx = (temp_index + i) % TEMP_BUFFER_SIZE;
        ui_Chart_series_1_array[i] = (lv_coord_t) temp_buffer[idx];
    }

    lv_chart_refresh(ui_Chart);
}

/**
 * @brief Calcula el CRC16 para tramas Modbus RTU.
 *
 * @param data Puntero a los datos de entrada.
 * @param len Longitud del mensaje.
 * @return uint16_t CRC calculado.
 */
uint16_t modbus_crc(uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
        }
    }
    return crc;
}

/**
 * @brief Imprime en consola una cadena hexadecimal de bytes.
 *
 * @param tag Etiqueta de log.
 * @param data Arreglo de bytes.
 * @param len Cantidad de bytes.
 */
static void print_hex(const char *tag, const uint8_t *data, int len) {
    char buf[128];
    int pos = 0;
    for (int i = 0; i < len && pos < sizeof(buf) - 3; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%02X ", data[i]);
    }
    ESP_LOGI(tag, "%s", buf);
}

/**
 * @brief Envía una trama Modbus RTU y decodifica la respuesta como temperatura.
 *
 * @return float Temperatura en °C o -1 si hubo error.
 */
float read_temperature_raw() {
    uint8_t tx_buffer[8];
    uint8_t rx_buffer[16];
    int temperature_raw;
    float temperature;

    tx_buffer[0] = MODBUS_SLAVE_ID;
    tx_buffer[1] = 0x03;
    tx_buffer[2] = (TEMPERATURE_REGISTER >> 8) & 0xFF;
    tx_buffer[3] = TEMPERATURE_REGISTER & 0xFF;
    tx_buffer[4] = 0x00;
    tx_buffer[5] = 0x01;
    uint16_t crc = modbus_crc(tx_buffer, 6);
    tx_buffer[6] = crc & 0xFF;
    tx_buffer[7] = (crc >> 8) & 0xFF;

    uart_flush(UART_PORT);
    ESP_LOGI(TAG, "Trama enviada:");
    print_hex(TAG, tx_buffer, sizeof(tx_buffer));
    uart_write_bytes(UART_PORT, (const char *)tx_buffer, sizeof(tx_buffer));
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));

    int len = uart_read_bytes(UART_PORT, rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Bytes leídos: %d", len);

    if (len > 0) {
        ESP_LOGI(TAG, "Respuesta recibida:");
        print_hex(TAG, rx_buffer, len);
    } else {
        ESP_LOGE(TAG, "No se recibieron bytes");
        return -1;
    }

    if (len < 7 || rx_buffer[0] != MODBUS_SLAVE_ID || rx_buffer[1] != 0x03 || rx_buffer[2] != 2) {
        ESP_LOGE(TAG, "Respuesta inválida");
        return -1;
    }

    temperature_raw = (rx_buffer[3] << 8) | rx_buffer[4];
    temperature = temperature_raw / 10.0f;
    if (rx_buffer[3] & 0x80) {
        temperature = (temperature_raw - 65536) / 10.0f;
    }

    return temperature;
}

/**
 * @brief Devuelve la última temperatura EMA calculada.
 * @return float Temperatura suavizada en °C.
 */
float read_ema_temp(void) {
    return ema_temperature;
}

/**
 * @brief Inicializa UART1 en modo RS485 half-duplex.
 *
 * Configura la velocidad, pines, buffer y modo para Modbus RTU.
 */
void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TXD, UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, 256, 256, 0, NULL, 0);
    uart_set_mode(UART_PORT, UART_MODE_RS485_HALF_DUPLEX);
}

/**
 * @brief Tarea FreeRTOS que se ejecuta continuamente para leer la temperatura.
 *
 * Lee temperatura cada 5 segundos, aplica filtro EMA, actualiza gráfica y etiqueta en UI.
 */
void temperature_task(void *pvParameters) {
    while (1) {
        float raw = read_temperature_raw();
        if (raw != -1) {
            if (ema_temperature == 0.0f) {
                ema_temperature = raw;
            } else {
                ema_temperature = alpha * raw + (1 - alpha) * ema_temperature;
            }

            ESP_LOGI("Main", "Raw: %.2f°C | EMA: %.2f°C", raw, ema_temperature);

            temp_buffer[temp_index] = ema_temperature;
            temp_index = (temp_index + 1) % TEMP_BUFFER_SIZE;

            actualizar_grafica_temp();

            bool ssr_on = false;  // ← Este valor se debe obtener desde el controlador PID

            ui_actualizar_estado_pid(ema_temperature, ssr_on);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Inicializa UART y lanza la tarea de lectura de temperatura.
 */
void start_temperature_task() {
    uart_init();
    xTaskCreate(temperature_task, "temperature_task", 4096, NULL, 5, NULL);
}
