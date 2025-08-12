# Auditoría de Código Exhaustiva – TriptaLabs Heat Controller

> **Ámbito**: Solo artefactos de software presentes en el repositorio (`main/`, `drivers/`, `ui/`, `docs/`). Se excluyen fabricación HW, trazabilidad de commits y aspectos eléctricos, salvo referencias a Seguridad Funcional (SIL) y Ciberseguridad.
>
> **Objetivo**: Determinar si la versión actual es apta para despliegue industrial.

---

## 1. Visión General del Sistema

* **Plataforma**: ESP32-S3, FreeRTOS, LVGL.
* **Componentes clave**:
  1. Lógica de control de temperatura (PID + Autotuning Åström-Hägglund y Ziegler-Nichols).
  2. Servidor WebSocket para monitoreo y control remoto.
  3. Drivers HW (Modbus UART, I²C LCD, IO CH422G).
  4. OTA a microSD + flasheo.
  5. UI táctil LVGL.
  6. Estadísticas y módulo de pruebas de sistema.

---

## 2. Arquitectura de Software

| Capa | Rutas principales | Descripción | Hallazgos |
|------|-------------------|-------------|-----------|
| **Core** | `main/core/` | Lógica de negocio (PID, OTA, Wi-Fi, BT, tiempo, estadísticas) | Modular, pero dependencias directas entre submódulos.
| **Drivers** | `main/drivers/` | Sensor, LCD, IO expansor | Interfaz directa a HW; no mocks.
| **Autotuning** | `main/core/autotuning/` | ZN & ÅH, tareas FreeRTOS | Implementado y activo.
| **WebSocket** | `main/core/ws_server/` | Servidor WS para clientes remotos | Sin autenticación ni cifrado.
| **UI** | `main/ui/` | Pantallas y eventos LVGL | Falta feedback en varios flujos.

---

## 3. Inspección Detallada de Módulos

### 3.1 WebSocket Server (`main/core/ws_server/ws_server.c`)
* Broadcast JSON de estado cada segundo.
* Soporta múltiples clientes sobre `esp_http_server`.
* Sin autenticación ni cifrado TLS.

### 3.2 OTA (`main/core/update.c`)
* Descarga firmware con `esp_http_client` por HTTP/HTTPS **sin** `cert_pem` ⇒ MITM.
* No firma de firmware. Sólo hash SHA-256, insuficiente.
* Manejo de memoria: usa `malloc` 4 KB, chequea nulo y libera (`update_prepare_recovery_files`).
* Timeouts configurables, pero no hay reintentos ni validación de tamaño.

### 3.3 Wi-Fi (`main/core/wifi_manager.c`)
```73:81:main/core/wifi_manager.c
wifi_credentials_t creds;
if (wifi_prov_get_credentials(&creds) != ESP_OK) {
    ESP_LOGW(TAG, "Sin credenciales en NVS; iniciando provisioning BLE");
    wifi_prov_start_ble_provisioning();
    strcpy(creds.ssid, "DemoSSID");
    strcpy(creds.password, "password");
}
```
* Credenciales hard-coded como respaldo.
* Función monolítica (~120 líneas) inicializa Wi-Fi, escaneo y SNTP.
* Reintentos y mDNS activan `ws_server_start`.

### 3.4 PID (`main/core/pid_controller.c`)
* Controlador incremental con salida PWM SSR.
* Límites de Kp/Ki/Kd y persistencia NVS.
* Watchdog de proceso inexistente (no esp_task_wdt).

### 3.5 Autotuning
* Ziegler-Nichols: `ziegler_nichols.c` crea tarea ⇒ parámetros aplicados al PID.
* Åström-Hägglund: `astrom_hagglund.c` similar.
* UI aún no expone selector de método.

### 3.6 Sensor Modbus (`drivers/sensor/sensor.c`)
* UART RS485 half-duplex @9600.
* CRC16 implementado manualmente.
* Filtro EMA con arranque suave ✅.
* No se desechan outliers ni se recalibra.

### 3.7 UI & LVGL
* `lvgl_port.c` pinneado opcionalmente a core; utiliza buffers en PSRAM con `heap_caps_malloc`.
* Sin medidor FPS; numerosos `TODO` para mensajes de error.

### 3.8 Estadísticas y Test de Sistema
* `statistics.c` almacena sesiones en NVS; no limita crecimiento.
* `system_test.c` ejecuta pruebas de sensor + relay, llamado desde UI.

---

## 4. Seguridad y Ciberseguridad (IEC 62443-4-1)

| Riesgo | Evidencia | Impacto |
|--------|-----------|---------|
| **Credenciales en código** | 73:81 `wifi_manager.c` | Acceso no autorizado a red.
| **OTA sin TLS** | 197:204 `update.c` | Inyección de firmware malicioso.
| **Sin verificación de firma** | update.c | Compromiso de dispositivo.
| **Logs verbosos en producción** | múltiples `ESP_LOGI` | Revelación de información.
| **Bluetooth nombre editable sin autenticación** | UI | Spoofing de dispositivo.
| **WebSocket sin autenticación** | `ws_server.c` | Control remoto no autorizado.

Mitigaciones: usar `esp_https_ota`, implementar aprovisionamiento, desactivar logs, BLE pairing seguro, quemar EFUSE secure boot + flash encryption.

---

## 5. Seguridad Funcional (SIL-orientada)

| Elemento | Estado | Comentario |
|----------|--------|------------|
| Watchdog HW/SW | ❌ | No `esp_task_wdt` ni WDT HW alimentado.
| Sensor redundante | ❌ | Un único termopar; si falla → sin mecanismo.
| Paro seguro SSR | ⚠ | `desactivar_ssr()` se llama al deshabilitar PID, pero no ante error sensor.
| Pruebas de autodiagnóstico al arranque | ✔ | Bootloader calcula hash.
| Autotuning activo | ✔ | Mejora control; requiere validación térmica.

Recomendación: habilitar watchdog, umbrales de sobretemperatura con corte HW, doble sensor o diagnóstico, evaluación SIL (probabilidad de falla).

---

## 6. Calidad del Código

### 6.1 Estilo y Normas
* Mezcla idioma. Adoptar inglés para código y español sólo en docs.
* clang-format inexistente; agregar workflow.

### 6.2 Tests y CI
* No carpeta `tests/`, ni mocks de UART/LCD.
* Sin workflow GitHub Actions → riesgo de regresiones.

### 6.3 Complejidad y Mantenimiento
* 12 funciones >100 LOC.
* Duplicación de lógica entre ZN y AH; extraer base común.

---

## 7. Rendimiento y Recursos

| Recurso | Observación |
|---------|-------------|
| Heap PSRAM | LVGL buffers ~320 KB, OK con PSRAM habilitada.
| CPU | Tareas: PID(5), Temp(5), LVGL(core-pinned, prio 4) – riesgo de _starvation_ mínima.
| Persistencia NVS | Uso de namespaces `pid_params` y `statistics`; sin GC.

---

## 8. Tabla de Cumplimiento (listo ↔ pendiente)

| Categoría | ✔ Listo | ⚠ Parcial | ❌ Pendiente |
|-----------|---------|-----------|--------------|
| Integridad de arranque (bootloader IDF) | ✔ | | |
| Secure Boot + Encrypted Flash | | | ❌ |
| OTA TLS + firma | | | ❌ |
| Credentials provisioning | | | ❌ |
| Watchdog HW/SW | | | ❌ |
| Fail-safe over-temp | | ⚠ | |
| Autotuning validado | | ⚠ | |
| Unit Tests + CI | | | ❌ |
| Lint / clang-format | | | ❌ |
| Style Consistency | | ⚠ | |

---

## 9. Recomendaciones Priorizadas

1. **Seguridad Alta**
   * Eliminar credenciales hard-coded; usar NVS + provisioning sobre BLE/Wi-Fi.
   * Migrar OTA a `esp_https_ota()` con `cert_pem` y verificación de firma (RSA-2048).
   * Activar Secure Boot v2 y Flash Encryption (quemar EFUSEs en producción).

2. **Seguridad Funcional**
   * Incorporar `esp_task_wdt` y reinicio seguro.
   * Implementar rutina de paro de emergencia cuando `read_temperature_raw()==-1` o > Tmax.

3. **Calidad y Mantenimiento**
   * Introducir **Unity/CMock** + simulador UART para tests.
   * Añadir GitHub Action: build booleana, cppcheck, clang-format.
   * Refactorizar `wifi_manager_init`, `update_perform`, unir código común de autotuning.

4. **UI/UX**
   * Completar `TODO` de mensajes de error.
   * Añadir iconos de estado crítico y progreso OTA.

5. **Documentación**
   * Manual de operación y guía rápida de aprovisionamiento.
   * Registro de riesgos SIL preliminar.

---

## 10. Conclusión

El proyecto presenta una **base sólida** — arquitectura modular y PID con autotuning — pero **no satisface** los requisitos industriales de ciberseguridad y seguridad funcional en su estado actual:

* OTA vulnerable a MITM.
* Credenciales expuestas.
* Carece de watchdog y mecanismos de paro seguro.
* Sin pruebas automatizadas ni CI.

**Veredicto**: **No apto** para producción industrial hasta que se implementen las contramedidas listadas en las prioridades 1 y 2 y se establezca un pipeline de pruebas robusto.

---

*Informe generado automáticamente – Revisado por auditor humano recomendado antes de certificación.* 