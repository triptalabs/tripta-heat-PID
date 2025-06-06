![Project banner](banner.png)


# 🔥 triptalabs-heat-controller

**Controlador inteligente para horno de vacío**, desarrollado por Tripta Labs, diseñado sobre ESP32-S3 con interfaz táctil y firmware escrito en C usando **ESP-IDF**. Este sistema permite el control térmico preciso mediante PID, lectura de sensores industriales, y actualización remota del firmware.

---

## 🧪 Descripción general

Este proyecto controla un horno de vacío a través de una interfaz táctil implementada con **LVGL**, y cuenta con soporte para:

* Control de temperatura con PID.
* Lectura precisa de temperatura con sensor **PT100** conectado vía **RS485** usando módulo PT21A01.
* Salida a **relé de estado sólido (SSR)** para activar la resistencia.
* Gráfica de temperatura en tiempo real.
* Temporizador de ciclo térmico.
* Conectividad **Wi-Fi** y **Bluetooth** para futuras integraciones con apps móviles.
* Actualización OTA del firmware directamente desde GitHub (sin particiones OTA).

---

## 🖥️ Soporte de hardware (Placa de desarrollo waveshare)

* **Supported Targets:** ESP32-S3
* **Supported LCD Controller:** ST7701
* **Supported Touch Controller:** GT911

---

## 🎯 Hardware utilizado

| Componente            | Descripción                                            |
| --------------------- | ------------------------------------------------------ |
| ESP32-S3              | Módulo con Wi-Fi + Bluetooth, doble núcleo             |
| Pantalla táctil       | Waveshare 5" 1024x600 con FT5436 controlador I2C       |
| Sensor de temperatura | PT100 RTD                                              |
| Interfaz RS485        | Módulo PT21A01-B-MODBUS para lectura digital del PT100 |
| SSR                   | Relevador de estado sólido (40A, 110V AC)              |

---

## 🧩 Tecnologías y librerías

* [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/)
* [LVGL](https://lvgl.io/) (GUI táctil)
* FreeRTOS
* Protocolo **Modbus RTU** (implementado manualmente)
* NVS (Non-Volatile Storage) para persistencia de configuración PID
* SDMMC (microSD para backups y OTA)
* HTTP Client + JSON Parser para actualización desde GitHub

---

## 🖥️ Estructura del proyecto

```
triptalabs-heat-controller/
├── main/
│   ├── core/
│   │   ├── main.c
│   │   ├── pid_controller.c/.h
│   │   ├── update.c/.h
│   │   └── CMakeLists.txt
│   ├── drivers/
│   │   ├── config/
│   │   │   ├── DEV_Config.c/.h
│   │   │   └── Kconfig.projbuild
│   │   ├── display/
│   │   │   ├── CH422G.c/.h
│   │   │   └── waveshare_rgb_lcd_port.c
│   │   ├── io/
│   │   └── sensor/
│   │       └── sensor.c/.h
│   ├── ui/
│   │   ├── components/
│   │   │   ├── ui_comp.c/.h
│   │   │   ├── ui_comp_hook.c/.h
│   │   │   └── ui_comp_statusbar.c/.h
│   │   ├── fonts/
│   │   ├── images/
│   │   ├── screens/
│   │   │   ├── ui_DevMode.c
│   │   │   ├── ui_ScreenAjustes.c
│   │   │   ├── ui_ScreenBootlogo.c
│   │   │   ├── ui_ScreenBT.c
│   │   │   ├── ui_ScreenEstadisticas.c
│   │   │   ├── ui_ScreenHome.c
│   │   │   └── ui_ScreenWifi.c
│   │   ├── filelist.txt
│   │   ├── ui_events.c/.h
│   │   ├── ui_helpers.c/.h
│   │   └── ui.c/.h
│   ├── ui_chart_data.c/.h
│   ├── lvgl_port.c/.h
│   ├── CMakeLists.txt
│   └── idf_component.yml
├── components/
│   ├── lvgl__lvgl/
│   └── espressif__esp_lcd_touch/
├── sdkconfig.defaults
├── partitions.csv
├── .gitignore
├── CMakeLists.txt
└── README.md
```

---

## 🚀 Cómo compilar

1. Instala el entorno de desarrollo ESP-IDF:
   [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

2. Clona este repositorio:

   ```bash
   git clone https://github.com/triptalabs/triptalabs-heat-controller.git
   cd triptalabs-heat-controller
   ```

3. Inicializa el entorno:

   ```bash
   idf.py set-target esp32s3
   idf.py menuconfig   # Solo si necesitas modificar la configuración
   ```

4. Compila y flashea:

   ```bash
   idf.py build
   idf.py flash monitor
   ```

---

## 📡 Actualización OTA

El dispositivo verifica periódicamente si hay una versión nueva disponible en GitHub. Si la encuentra, descarga el nuevo binario a la microSD y lo flashea automáticamente. En caso de fallo, restaura desde un backup local.

---

## 📊 Interfaz táctil

* Diseñada en SquareLine Studio.
* Control táctil de setpoint, estado del PID, temporizador.
* Visualización de temperatura actual y gráfica en tiempo real.
* Configuración accesible de parametros de optimizacion (Kp, Kd, Ki) desde panel DevMode.
* Barra de estado con información de conectividad.
* Pantallas de configuración para WiFi y Bluetooth.
* Modo de desarrollo para ajustes avanzados.

---

## ⚠️ Estado del proyecto

> ✅ Funcionalidades implementadas:
>
> * Controlador PID con ajuste de parámetros
> * Lectura PT100 vía RS485 con Modbus RTU
> * OTA desde GitHub con sistema de backup
> * Interfaz gráfica completa con múltiples pantallas
> * Integración lectura sensor + control PID
> * Watchdog y sistema de recuperación
> * Configuración de WiFi y Bluetooth
> * Gráfica de temperatura en tiempo real
> * Temporizador de ciclo térmico
> * Modo de desarrollo para ajustes avanzados

Próximas mejoras:

* Integración con app móvil
* Guardado de datos históricos
* Mejoras en la interfaz de usuario
* Optimización del control PID

---

## 🧠 Licencia

Este proyecto es propiedad intelectual de **Tripta Labs S.A.S.**
Licencia: MIT – uso libre bajo atribución.

---

## ✨ Autor

**Tripta**
[https://github.com/triptalabs](https://github.com/triptalabs)

---
