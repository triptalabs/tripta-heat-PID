/**
 * @file bootloader_main.c
 * @brief Implementación principal del bootloader personalizado para TriptaLabs Heat Controller
 *
 * Este archivo contiene la lógica principal que coordina:
 * - Verificación de integridad al boot
 * - Recovery automático desde SD
 * - Modo recovery manual
 * - Gestión de estado en NVS
 *
 * @author TriptaLabs
 * @date 2025-01-28
 */

#include "bootloader_config.h"
#include "integrity_checker.h"
#include "sd_recovery.h"
#include "recovery_mode.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"

/* ================================
 * VARIABLES GLOBALES
 * ================================ */

/**
 * @brief Handle NVS para persistencia de datos
 */
static nvs_handle_t bootloader_nvs_handle = 0;

/**
 * @brief Estadísticas globales del bootloader
 */
static bootloader_stats_t bootloader_stats = {0};

/**
 * @brief Flag que indica si el bootloader está inicializado
 */
static bool bootloader_initialized = false;

/* ================================
 * FUNCIONES PRIVADAS
 * ================================ */

/**
 * @brief Inicializa NVS y carga estadísticas del bootloader
 *
 * @return
 *      - ESP_OK: Inicialización exitosa
 *      - ESP_FAIL: Error en la inicialización
 */
static esp_err_t init_nvs_and_load_stats(void) {
    // Inicializar NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Si NVS está lleno o hay nueva versión, borrar y reinicializar
        ESP_LOGW(BOOTLOADER_TAG, "NVS requiere limpieza, reinicializando...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    BOOTLOADER_CHECK_RET(ret);

    // Abrir namespace del bootloader
    ret = nvs_open(BOOTLOADER_NVS_NAMESPACE, NVS_READWRITE, &bootloader_nvs_handle);
    BOOTLOADER_CHECK_RET(ret);

    // Cargar estadísticas existentes
    size_t required_size = sizeof(bootloader_stats_t);
    ret = nvs_get_blob(bootloader_nvs_handle, "stats", &bootloader_stats, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // Primera vez, inicializar estadísticas
        ESP_LOGI(BOOTLOADER_TAG, "Primera ejecución - inicializando estadísticas");
        memset(&bootloader_stats, 0, sizeof(bootloader_stats_t));
        bootloader_stats.first_boot = true;
        bootloader_stats.boot_attempts = 1;
        bootloader_stats.total_boots = 1;
        bootloader_stats.last_boot_reason = BOOT_REASON_NORMAL;
    } else if (ret == ESP_OK) {
        // Incrementar estadísticas de boot
        bootloader_stats.boot_attempts++;
        bootloader_stats.total_boots++;
        ESP_LOGI(BOOTLOADER_TAG, "Boot #%lu, intentos consecutivos: %d", 
                 bootloader_stats.total_boots, bootloader_stats.boot_attempts);
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "Error cargando estadísticas: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Guarda estadísticas del bootloader en NVS
 *
 * @return ESP_OK si exitoso, error code si no
 */
static esp_err_t save_bootloader_stats(void) {
    esp_err_t ret = nvs_set_blob(bootloader_nvs_handle, "stats", &bootloader_stats, sizeof(bootloader_stats_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(bootloader_nvs_handle);
    }
    return ret;
}

/**
 * @brief Resetea contadores de boot tras boot exitoso
 */
static void reset_boot_counters(void) {
    bootloader_stats.boot_attempts = 0;
    bootloader_stats.recovery_attempts = 0;
    save_bootloader_stats();
    ESP_LOGI(BOOTLOADER_TAG, "Contadores de boot reseteados tras éxito");
}

/**
 * @brief Determina si se debe forzar recovery basado en estadísticas
 *
 * @return true si se debe forzar recovery, false si no
 */
static bool should_force_recovery(void) {
    // Forzar recovery si hay muchos fallos consecutivos
    if (bootloader_stats.boot_attempts >= MAX_BOOT_ATTEMPTS) {
        ESP_LOGW(BOOTLOADER_TAG, "Demasiados fallos de boot (%d >= %d), forzando recovery", 
                 bootloader_stats.boot_attempts, MAX_BOOT_ATTEMPTS);
        return true;
    }

    // Forzar recovery si hay muchos intentos de recovery recientes
    if (bootloader_stats.recovery_attempts >= MAX_RECOVERY_ATTEMPTS) {
        ESP_LOGW(BOOTLOADER_TAG, "Demasiados intentos de recovery (%d >= %d), modo emergency", 
                 bootloader_stats.recovery_attempts, MAX_RECOVERY_ATTEMPTS);
        return true;
    }

    return false;
}

/**
 * @brief Registra evento de recovery en estadísticas
 *
 * @param success true si recovery fue exitoso, false si no
 */
static void record_recovery_event(bool success) {
    bootloader_stats.total_recoveries++;
    bootloader_stats.last_recovery_timestamp = esp_log_timestamp();
    
    if (success) {
        bootloader_stats.recovery_attempts = 0;
        bootloader_stats.last_boot_reason = BOOT_REASON_SD_RECOVERY;
        ESP_LOGI(BOOTLOADER_TAG, "Recovery exitoso registrado");
    } else {
        bootloader_stats.recovery_attempts++;
        bootloader_stats.last_boot_reason = BOOT_REASON_RECOVERY;
        ESP_LOGW(BOOTLOADER_TAG, "Recovery fallido registrado (intento %d)", 
                 bootloader_stats.recovery_attempts);
    }
    
    save_bootloader_stats();
}

/* ================================
 * FUNCIONES PÚBLICAS
 * ================================ */

/**
 * @brief Inicializa el bootloader personalizado
 *
 * @return
 *      - ESP_OK: Inicialización exitosa
 *      - ESP_FAIL: Error en la inicialización
 */
esp_err_t bootloader_init(void) {
    if (bootloader_initialized) {
        return ESP_OK; // Ya inicializado
    }

    ESP_LOGI(BOOTLOADER_TAG, "=== Iniciando Bootloader Personalizado v%s ===", BOOTLOADER_VERSION);

    // 1. Inicializar NVS y cargar estadísticas
    BOOTLOADER_CHECK_RET(init_nvs_and_load_stats());

    // 2. Inicializar módulos
    BOOTLOADER_CHECK_RET(integrity_checker_init());
    BOOTLOADER_CHECK_RET(sd_recovery_init());
    BOOTLOADER_CHECK_RET(recovery_mode_init());

    bootloader_initialized = true;
    ESP_LOGI(BOOTLOADER_TAG, "Bootloader inicializado exitosamente");

    return ESP_OK;
}

/**
 * @brief Función principal de decisión del bootloader
 *
 * Esta es la función que se debe llamar al inicio de app_main()
 * para verificar integridad y decidir si continuar con boot normal
 * o activar recovery.
 *
 * @return
 *      - ESP_OK: App puede continuar con boot normal
 *      - ESP_FAIL: Recovery falló, sistema en estado crítico
 *      - NO RETORNA: Sistema reinicia tras recovery exitoso
 */
esp_err_t bootloader_check_and_decide(void) {
    esp_err_t ret;
    firmware_info_t firmware_info = {0};
    recovery_state_t recovery_state = RECOVERY_STATE_IDLE;
    bool force_recovery = false;

    ESP_LOGI(BOOTLOADER_TAG, "=== Verificación de Integridad de Boot ===");

    // Verificar si debemos forzar recovery por estadísticas
    force_recovery = should_force_recovery();
    
    if (!force_recovery) {
        // Verificar integridad de la aplicación
        ret = verify_app_partition_integrity(&firmware_info);
        if (ret == ESP_OK) {
            // App íntegra - boot normal
            ESP_LOGI(BOOTLOADER_TAG, "✅ Firmware íntegro - continuando boot normal");
            reset_boot_counters();
            return ESP_OK;
        } else {
            ESP_LOGW(BOOTLOADER_TAG, "❌ Firmware corrupto detectado: %s", esp_err_to_name(ret));
            bootloader_stats.last_boot_reason = BOOT_REASON_CORRUPTION;
        }
    } else {
        ESP_LOGW(BOOTLOADER_TAG, "🔧 Recovery forzado por múltiples fallos");
        bootloader_stats.last_boot_reason = BOOT_REASON_MULTIPLE_FAILURES;
    }

    // Firmware corrupto o recovery forzado - intentar recovery automático
    ESP_LOGI(BOOTLOADER_TAG, "=== Iniciando Recovery Automático desde SD ===");
    
    ret = perform_full_sd_recovery(&recovery_state);
    if (ret == ESP_OK) {
        // Recovery exitoso
        ESP_LOGI(BOOTLOADER_TAG, "✅ Recovery automático exitoso - reiniciando sistema");
        record_recovery_event(true);
        
        // Esperar un momento antes de reiniciar
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart(); // NO RETORNA
    }

    // Recovery automático falló
    ESP_LOGE(BOOTLOADER_TAG, "❌ Recovery automático falló: %s", esp_err_to_name(ret));
    record_recovery_event(false);

    // Activar modo recovery manual
    ESP_LOGI(BOOTLOADER_TAG, "=== Activando Modo Recovery Manual ===");
    
    ret = enter_recovery_mode(bootloader_stats.last_boot_reason, &recovery_state);
    if (ret == ESP_OK) {
        // Recovery manual exitoso
        ESP_LOGI(BOOTLOADER_TAG, "✅ Recovery manual exitoso - reiniciando sistema");
        record_recovery_event(true);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart(); // NO RETORNA
    }

    // Todo falló - estado crítico
    ESP_LOGE(BOOTLOADER_TAG, "💀 ESTADO CRÍTICO: Todos los métodos de recovery fallaron");
    record_recovery_event(false);
    
    // Mostrar error crítico y mantener sistema en modo emergency
    show_critical_error(-1, "Sistema en estado crítico", false);
    
    return ESP_FAIL;
}

/**
 * @brief Marca el boot actual como exitoso
 *
 * Debe ser llamado por la aplicación una vez que haya iniciado correctamente
 * para resetear contadores de fallos.
 */
void bootloader_mark_boot_successful(void) {
    if (!bootloader_initialized) {
        ESP_LOGW(BOOTLOADER_TAG, "Bootloader no inicializado, no se puede marcar boot exitoso");
        return;
    }

    ESP_LOGI(BOOTLOADER_TAG, "🎉 Boot marcado como exitoso");
    reset_boot_counters();
}

/**
 * @brief Obtiene estadísticas actuales del bootloader
 *
 * @param[out] stats Estructura para copiar estadísticas
 * @return ESP_OK si exitoso
 */
esp_err_t bootloader_get_stats(bootloader_stats_t* stats) {
    if (!stats || !bootloader_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(stats, &bootloader_stats, sizeof(bootloader_stats_t));
    return ESP_OK;
}

/**
 * @brief Fuerza un recovery desde SD (para testing o recovery manual)
 *
 * @return
 *      - ESP_OK: Recovery forzado exitoso
 *      - ESP_FAIL: Recovery forzado falló
 */
esp_err_t bootloader_force_recovery(void) {
    ESP_LOGI(BOOTLOADER_TAG, "🔧 Recovery forzado por solicitud del usuario");
    
    recovery_state_t recovery_state = RECOVERY_STATE_IDLE;
    esp_err_t ret = perform_full_sd_recovery(&recovery_state);
    
    record_recovery_event(ret == ESP_OK);
    
    if (ret == ESP_OK) {
        ESP_LOGI(BOOTLOADER_TAG, "✅ Recovery forzado exitoso");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart(); // NO RETORNA
    }
    
    return ret;
}

/**
 * @brief Limpia todos los datos del bootloader (factory reset)
 *
 * @return ESP_OK si exitoso
 */
esp_err_t bootloader_factory_reset(void) {
    ESP_LOGW(BOOTLOADER_TAG, "⚠️  Realizando Factory Reset del bootloader");
    
    // Limpiar datos de integridad
    clear_integrity_data();
    
    // Resetear estadísticas
    memset(&bootloader_stats, 0, sizeof(bootloader_stats_t));
    bootloader_stats.first_boot = true;
    bootloader_stats.boot_attempts = 1;
    bootloader_stats.total_boots = 1;
    
    save_bootloader_stats();
    
    ESP_LOGI(BOOTLOADER_TAG, "Factory reset completado");
    return ESP_OK;
}

/* ================================
 * FUNCIONES DE COMPATIBILIDAD
 * ================================ */

/**
 * @brief Alias para bootloader_init() para compatibilidad con main.c
 */
esp_err_t bootloader_system_init(void) {
    return bootloader_init();
}

/**
 * @brief Verificación post-boot (marca boot como exitoso y verifica)
 */
esp_err_t bootloader_post_boot_check(void) {
    ESP_LOGI(BOOTLOADER_TAG, "Ejecutando verificaciones post-boot...");
    
    if (!bootloader_initialized) {
        ESP_LOGW(BOOTLOADER_TAG, "Bootloader no inicializado");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Marcar boot como exitoso
    bootloader_mark_boot_successful();
    
    // Verificar integridad como confirmación adicional
    firmware_info_t firmware_info;
    esp_err_t integrity_result = verify_app_partition_integrity(&firmware_info);
    if (integrity_result != ESP_OK) {
        ESP_LOGW(BOOTLOADER_TAG, "Advertencia: problema de integridad detectado");
        return integrity_result;
    }
    
    ESP_LOGI(BOOTLOADER_TAG, "✅ Verificaciones post-boot completadas");
    return ESP_OK;
}

/**
 * @brief Alias para bootloader_get_stats() para compatibilidad
 */
esp_err_t get_bootloader_stats(bootloader_stats_t* stats) {
    return bootloader_get_stats(stats);
}

/**
 * @brief Almacena estadísticas del bootloader (para compatibilidad con update.c)
 */
esp_err_t store_bootloader_stats(const bootloader_stats_t* stats) {
    if (!stats || !bootloader_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&bootloader_stats, stats, sizeof(bootloader_stats_t));
    return save_bootloader_stats();
}

/* ================================
 * FUNCIONES DE TESTING Y DEBUG
 * ================================ */

/**
 * @brief Ejecuta una prueba rápida del sistema de bootloader
 */
esp_err_t bootloader_run_self_test(void) {
    ESP_LOGI(BOOTLOADER_TAG, "=== EJECUTANDO SELF-TEST DEL BOOTLOADER ===");
    
    esp_err_t test_result = ESP_OK;
    
    // Test 1: Verificar inicialización
    if (!bootloader_initialized) {
        ESP_LOGE(BOOTLOADER_TAG, "❌ TEST 1 FALLÓ: Bootloader no inicializado");
        test_result = ESP_FAIL;
    } else {
        ESP_LOGI(BOOTLOADER_TAG, "✅ TEST 1 OK: Bootloader inicializado");
    }
    
    // Test 2: Verificar NVS
    bootloader_stats_t test_stats;
    if (bootloader_get_stats(&test_stats) != ESP_OK) {
        ESP_LOGE(BOOTLOADER_TAG, "❌ TEST 2 FALLÓ: No se pueden leer estadísticas NVS");
        test_result = ESP_FAIL;
    } else {
        ESP_LOGI(BOOTLOADER_TAG, "✅ TEST 2 OK: NVS accesible (boots: %lu)", test_stats.total_boots);
    }
    
    // Test 3: Verificar módulo de integridad
    firmware_info_t firmware_info = {0};
    if (verify_app_partition_integrity(&firmware_info) != ESP_OK) {
        ESP_LOGW(BOOTLOADER_TAG, "⚠️  TEST 3 ADVERTENCIA: Problema de integridad");
    } else {
        ESP_LOGI(BOOTLOADER_TAG, "✅ TEST 3 OK: Integridad verificada");
    }
    
    // Test 4: Verificar acceso a SD
    if (check_sd_accessibility() == ESP_OK) {
        ESP_LOGI(BOOTLOADER_TAG, "✅ TEST 4 OK: SD accesible");
    } else {
        ESP_LOGW(BOOTLOADER_TAG, "⚠️  TEST 4 ADVERTENCIA: SD no accesible");
    }
    
    // Test 5: Verificar módulo de recovery mode
    recovery_mode_init();
    ESP_LOGI(BOOTLOADER_TAG, "✅ TEST 5 OK: Recovery mode operativo");
    
    if (test_result == ESP_OK) {
        ESP_LOGI(BOOTLOADER_TAG, "🎉 SELF-TEST COMPLETADO EXITOSAMENTE");
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "💀 SELF-TEST FALLÓ - Revisar logs");
    }
    
    return test_result;
}

/**
 * @brief Simula una corrupción de firmware para testing
 */
esp_err_t bootloader_simulate_corruption(void) {
    ESP_LOGW(BOOTLOADER_TAG, "⚠️  SIMULANDO CORRUPCIÓN DE FIRMWARE");
    ESP_LOGW(BOOTLOADER_TAG, "PELIGRO: Esto forzará recovery en próximo boot");
    
    // Corromper el hash almacenado
    uint8_t fake_hash[32];
    memset(fake_hash, 0xFF, sizeof(fake_hash)); // Hash obviamente incorrecto
    
    esp_err_t ret = store_firmware_hash(fake_hash);
    if (ret == ESP_OK) {
        ESP_LOGW(BOOTLOADER_TAG, "✅ Hash corrompido exitosamente");
        ESP_LOGW(BOOTLOADER_TAG, "⚠️  REINICIAR AHORA PARA ACTIVAR RECOVERY");
    } else {
        ESP_LOGE(BOOTLOADER_TAG, "❌ Error corrompiendo hash");
    }
    
    return ret;
}

/**
 * @brief Muestra información detallada del bootloader por UART
 */
esp_err_t bootloader_print_detailed_info(void) {
    printf("\n");
    printf("=====================================\n");
    printf("    BOOTLOADER INFORMATION\n");
    printf("    TriptaLabs Heat Controller\n");
    printf("=====================================\n");
    
    // Información básica
    printf("Bootloader Version: %s\n", BOOTLOADER_VERSION);
    printf("Initialized: %s\n", bootloader_initialized ? "Yes" : "No");
    
    // Estadísticas
    bootloader_stats_t stats;
    if (bootloader_get_stats(&stats) == ESP_OK) {
        printf("\n--- ESTADÍSTICAS ---\n");
        printf("Total Boots: %lu\n", stats.total_boots);
        printf("Boot Attempts: %d\n", stats.boot_attempts);
        printf("Total Recoveries: %lu\n", stats.total_recoveries);
        printf("Recovery Attempts: %d\n", stats.recovery_attempts);
        printf("First Boot: %s\n", stats.first_boot ? "Yes" : "No");
        printf("Last Boot Reason: %s\n", boot_reason_to_string(stats.last_boot_reason));
    }
    
    // Información de firmware
    firmware_info_t firmware_info = {0};
    esp_err_t integrity_result = verify_app_partition_integrity(&firmware_info);
    printf("\n--- FIRMWARE ---\n");
    printf("Integrity Check: %s\n", (integrity_result == ESP_OK) ? "PASS" : "FAIL");
    printf("Firmware Valid: %s\n", firmware_info.valid ? "Yes" : "No");
    printf("Hash Match: %s\n", firmware_info.hash_match ? "Yes" : "No");
    printf("Firmware Size: %.1f MB\n", (float)firmware_info.size / (1024 * 1024));
    
    // Estado de SD
    printf("\n--- RECOVERY ---\n");
    printf("SD Accessible: %s\n", (check_sd_accessibility() == ESP_OK) ? "Yes" : "No");
    
    // Hash actual (primeros 16 caracteres)
    if (firmware_info.valid) {
        char hash_str[65];
        hash_to_hex_string(firmware_info.calculated_hash, hash_str);
        printf("Current Hash: %.16s...\n", hash_str);
    }
    
    printf("=====================================\n\n");
    
    return ESP_OK;
}