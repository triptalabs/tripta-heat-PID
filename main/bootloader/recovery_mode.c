/**
 * @file recovery_mode.c
 * @brief Implementación del módulo de modo recovery
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#include "recovery_mode.h"
#include "sd_recovery.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

/* ================================
 * VARIABLES PRIVADAS
 * ================================ */

static bool recovery_mode_initialized = false;
static bool display_initialized = false;
static bool uart_initialized = false;

/* ================================
 * FUNCIONES PRIVADAS
 * ================================ */

static const char* boot_reason_strings[] = {
    "Boot Normal",
    "Corrupción Detectada", 
    "Actualización Fallida",
    "Modo Recovery",
    "Múltiples Fallos",
    "Recovery desde SD",
    "Modo Emergency"
};

static const char* recovery_state_strings[] = {
    "Inactivo",
    "Verificando",
    "Montando SD",
    "Verificando Firmware",
    "Flasheando",
    "Limpiando",
    "Exitoso",
    "Fallido"
};

/* ================================
 * FUNCIONES PÚBLICAS
 * ================================ */

esp_err_t recovery_mode_init(void) {
    if (recovery_mode_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Inicializando módulo de modo recovery");
    
    recovery_mode_initialized = true;
    ESP_LOGI(BOOTLOADER_TAG, "Módulo recovery mode inicializado exitosamente");
    return ESP_OK;
}

esp_err_t init_recovery_display(void) {
    if (display_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(BOOTLOADER_TAG, "Inicializando pantalla para recovery...");
    
    // Intentar inicializar el display
    esp_err_t ret = waveshare_esp32_s3_rgb_lcd_init();
    if (ret == ESP_OK) {
        ret = waveshare_rgb_lcd_bl_on();
        if (ret == ESP_OK) {
            display_initialized = true;
            ESP_LOGI(BOOTLOADER_TAG, "✅ Pantalla recovery inicializada");
        }
    }
    
    if (ret != ESP_OK) {
        ESP_LOGW(BOOTLOADER_TAG, "⚠️  No se pudo inicializar pantalla, usando solo UART");
    }

    return ESP_OK; // No fallar si pantalla no está disponible
}

esp_err_t init_recovery_uart(void) {
    if (uart_initialized) {
        return ESP_OK;
    }

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0);
    if (ret == ESP_OK) {
        ret = uart_param_config(UART_NUM_0, &uart_config);
        if (ret == ESP_OK) {
            ret = uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 
                              UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            if (ret == ESP_OK) {
                uart_initialized = true;
                ESP_LOGI(BOOTLOADER_TAG, "✅ UART recovery inicializado");
            }
        }
    }

    return ret;
}

esp_err_t show_recovery_message(boot_reason_t reason, const char* details, int progress) {
    // Siempre mostrar por UART
    char message[256];
    snprintf(message, sizeof(message), 
             "=== MODO RECOVERY ===\nRazón: %s\nDetalles: %s\nProgreso: %d%%\n",
             boot_reason_to_string(reason), 
             details ? details : "N/A",
             progress);

    printf("%s", message);

    // Intentar mostrar en pantalla si está disponible
    if (display_initialized) {
        // Para esta implementación simplificada, solo logeamos
        ESP_LOGI(BOOTLOADER_TAG, "Mostrando en pantalla: %s", boot_reason_to_string(reason));
    }

    return ESP_OK;
}

esp_err_t show_uart_recovery_message(boot_reason_t reason, const char* details, const char* technical_info) {
    printf("\n");
    printf("=====================================\n");
    printf("    TRIPTABS HEAT CONTROLLER\n");
    printf("       MODO RECOVERY ACTIVADO\n");
    printf("=====================================\n");
    printf("Razón: %s\n", boot_reason_to_string(reason));
    printf("Detalles: %s\n", details ? details : "Sin detalles");
    
    if (technical_info) {
        printf("Información técnica:\n%s\n", technical_info);
    }
    
    printf("=====================================\n");
    printf("Contacto técnico: support@triptabs.com\n");
    printf("=====================================\n\n");

    return ESP_OK;
}

esp_err_t wait_for_recovery_action(uint32_t timeout_ms) {
    ESP_LOGI(BOOTLOADER_TAG, "Esperando acción del usuario (timeout: %lu ms)", timeout_ms);
    
    if (timeout_ms == 0) {
        // Sin timeout - esperar indefinidamente
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            // En implementación completa, verificar botones/UART aquí
        }
    } else {
        // Con timeout
        uint32_t elapsed = 0;
        const uint32_t check_interval = 100;
        
        while (elapsed < timeout_ms) {
            vTaskDelay(pdMS_TO_TICKS(check_interval));
            elapsed += check_interval;
            
            // En implementación completa, verificar input aquí
        }
        
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

esp_err_t enter_recovery_mode(boot_reason_t reason, recovery_state_t* recovery_info) {
    ESP_LOGI(BOOTLOADER_TAG, "=== ENTRANDO EN MODO RECOVERY ===");
    
    // Inicializar comunicaciones
    init_recovery_uart();
    init_recovery_display();
    
    // Mostrar información inicial
    show_uart_recovery_message(reason, "Sistema en modo recovery", NULL);
    show_recovery_message(reason, "Iniciando recovery manual", -1);
    
    // Intentar recovery manual
    ESP_LOGI(BOOTLOADER_TAG, "Intentando recovery manual...");
    
    esp_err_t ret = perform_full_sd_recovery(recovery_info);
    if (ret == ESP_OK) {
        show_recovery_message(BOOT_REASON_SD_RECOVERY, "Recovery exitoso", 100);
        printf("✅ Recovery manual exitoso - sistema se reiniciará\n");
        return ESP_OK;
    } else {
        show_recovery_message(BOOT_REASON_EMERGENCY, "Recovery falló", -1);
        printf("❌ Recovery manual falló\n");
        
        // Mostrar instrucciones finales
        printf("\nINSTRUCCIONES DE RECOVERY MANUAL:\n");
        printf("1. Verificar que SD contiene firmware válido\n");
        printf("2. Copiar base_firmware.bin y .sha256 a /recovery/\n");
        printf("3. Reiniciar el sistema\n");
        printf("4. Contactar soporte técnico si persiste\n\n");
        
        return ESP_FAIL;
    }
}

esp_err_t show_recovery_welcome_screen(void) {
    printf("\n");
    printf("  ████████╗██████╗ ██╗██████╗ ████████╗ █████╗ \n");
    printf("  ╚══██╔══╝██╔══██╗██║██╔══██╗╚══██╔══╝██╔══██╗\n");
    printf("     ██║   ██████╔╝██║██████╔╝   ██║   ███████║\n");
    printf("     ██║   ██╔══██╗██║██╔═══╝    ██║   ██╔══██║\n");
    printf("     ██║   ██║  ██║██║██║        ██║   ██║  ██║\n");
    printf("     ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝        ╚═╝   ╚═╝  ╚═╝\n");
    printf("           HEAT CONTROLLER - RECOVERY MODE\n\n");
    
    return ESP_OK;
}

esp_err_t show_system_info_screen(const firmware_info_t* firmware_info, const bootloader_stats_t* bootloader_stats) {
    if (!firmware_info || !bootloader_stats) {
        return ESP_ERR_INVALID_ARG;
    }

    printf("=== INFORMACIÓN DEL SISTEMA ===\n");
    printf("Firmware válido: %s\n", firmware_info->valid ? "Sí" : "No");
    printf("Tamaño firmware: %.1f MB\n", (float)firmware_info->size / (1024 * 1024));
    printf("Hash coincide: %s\n", firmware_info->hash_match ? "Sí" : "No");
    printf("Total boots: %lu\n", bootloader_stats->total_boots);
    printf("Intentos boot: %d\n", bootloader_stats->boot_attempts);
    printf("Total recoveries: %lu\n", bootloader_stats->total_recoveries);
    printf("Último boot: %s\n", boot_reason_to_string(bootloader_stats->last_boot_reason));
    printf("===============================\n\n");

    return ESP_OK;
}

esp_err_t show_recovery_progress(const char* operation, int progress, const char* status) {
    printf("[%3d%%] %s - %s\n", progress, operation ? operation : "Operación", 
           status ? status : "En progreso");
    return ESP_OK;
}

esp_err_t show_critical_error(int error_code, const char* error_message, bool recovery_possible) {
    printf("\n");
    printf("💀💀💀 ERROR CRÍTICO 💀💀💀\n");
    printf("Código: %d\n", error_code);
    printf("Mensaje: %s\n", error_message ? error_message : "Error desconocido");
    printf("Recovery posible: %s\n", recovery_possible ? "Sí" : "No");
    
    if (!recovery_possible) {
        printf("\n⚠️  SISTEMA EN ESTADO IRRECUPERABLE\n");
        printf("Contacte inmediatamente a soporte técnico\n");
        printf("Email: support@triptabs.com\n");
    }
    
    printf("\n");
    return ESP_OK;
}

esp_err_t send_system_info_uart(const firmware_info_t* firmware_info, const bootloader_stats_t* bootloader_stats) {
    return show_system_info_screen(firmware_info, bootloader_stats);
}

esp_err_t send_recovery_log_uart(const char* log_message, uint32_t timestamp) {
    printf("[%lu] %s\n", timestamp, log_message ? log_message : "Log vacío");
    return ESP_OK;
}

esp_err_t read_user_command_uart(char* command, size_t max_len, uint32_t timeout_ms) {
    if (!command || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    printf("Ingrese comando (timeout %lu ms): ", timeout_ms);
    
    // Implementación simplificada - solo retorna comando por defecto
    strncpy(command, "recovery", max_len - 1);
    command[max_len - 1] = '\0';
    
    return ESP_OK;
}

esp_err_t check_physical_buttons(int* button_pressed) {
    if (!button_pressed) {
        return ESP_ERR_INVALID_ARG;
    }

    // Implementación simplificada - no hay botones presionados
    *button_pressed = 0;
    return ESP_OK;
}

esp_err_t process_recovery_command(const char* command, int* action) {
    if (!command || !action) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(command, "recovery") == 0) {
        *action = 1; // Acción de recovery
        return ESP_OK;
    }
    
    if (strcmp(command, "reboot") == 0) {
        *action = 2; // Acción de reinicio
        return ESP_OK;
    }

    return ESP_ERR_INVALID_ARG;
}

esp_err_t execute_manual_recovery(void) {
    printf("Ejecutando recovery manual...\n");
    
    recovery_state_t state = RECOVERY_STATE_IDLE;
    esp_err_t ret = perform_full_sd_recovery(&state);
    
    if (ret == ESP_OK) {
        printf("✅ Recovery manual exitoso\n");
    } else {
        printf("❌ Recovery manual falló\n");
    }
    
    return ret;
}

const char* boot_reason_to_string(boot_reason_t reason) {
    if (reason >= 0 && reason < sizeof(boot_reason_strings)/sizeof(boot_reason_strings[0])) {
        return boot_reason_strings[reason];
    }
    return "Razón Desconocida";
}

const char* recovery_state_to_string(recovery_state_t state) {
    if (state >= 0 && state < sizeof(recovery_state_strings)/sizeof(recovery_state_strings[0])) {
        return recovery_state_strings[state];
    }
    return "Estado Desconocido";
}

esp_err_t format_technical_info(const firmware_info_t* firmware_info, char* formatted_info, size_t max_len) {
    if (!firmware_info || !formatted_info || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    char hash_str[65];
    // Convertir hash a string hexadecimal
    for (int i = 0; i < 32; i++) {
        sprintf(hash_str + (i * 2), "%02x", firmware_info->calculated_hash[i]);
    }
    
    snprintf(formatted_info, max_len,
             "Firmware Size: %.1f MB\n"
             "Valid: %s\n"
             "Hash Match: %s\n"
             "Calculated Hash: %.16s...\n",
             (float)firmware_info->size / (1024 * 1024),
             firmware_info->valid ? "Yes" : "No",
             firmware_info->hash_match ? "Yes" : "No",
             hash_str);

    return ESP_OK;
}

esp_err_t cleanup_recovery_mode(void) {
    if (display_initialized) {
        wavesahre_rgb_lcd_bl_off();
        display_initialized = false;
    }
    
    if (uart_initialized) {
        uart_driver_delete(UART_NUM_0);
        uart_initialized = false;
    }
    
    ESP_LOGI(BOOTLOADER_TAG, "Recovery mode limpiado");
    return ESP_OK;
} 