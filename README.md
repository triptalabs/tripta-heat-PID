![Project banner](banner.png)

# 🔥 TriptaLabs Heat Controller

**Controlador inteligente industrial para horno de vacío**, desarrollado por TriptaLabs, basado en ESP32-S3 con **bootloader personalizado**, interfaz táctil avanzada y sistema de actualización OTA innovador. 

> 🎯 **Proyecto de nivel industrial** con arquitectura robusta, verificación de integridad SHA256, recuperación automática y control PID profesional.

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4.1-blue)](https://docs.espressif.com/projects/esp-idf/)
[![LVGL](https://img.shields.io/badge/LVGL-8.x-green)](https://lvgl.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## 🧪 Descripción General

Este sistema implementa un **controlador profesional de horno de vacío** que combina hardware industrial con software de nivel producción. A diferencia de proyectos típicos de ESP32, incluye características avanzadas como:

### ✨ Características Principales

* 🛡️ **Bootloader personalizado** con verificación SHA256 y recuperación automática
* 🎛️ **Control PID avanzado** con parámetros configurables y anti-windup
* 🌡️ **Lectura precisa PT100** vía **Modbus RTU** implementado manualmente
* 📡 **Actualización OTA innovadora** directa sobre partición única (sin OTA estándar)
* 🖼️ **Interfaz táctil profesional** con LVGL y modo desarrollador secreto  
* 📊 **Sistema de estadísticas** y monitoreo de sesiones
* 🔧 **Modo recovery** con interfaz UART para mantenimiento
* 🌐 **Conectividad dual** WiFi + Bluetooth con sincronización NTP

---

## 🏗️ Arquitectura del Sistema

El sistema implementa una arquitectura modular robusta con separación clara de responsabilidades:

```mermaid
---
config:
  theme: neo-dark
  layout: elk
  flowchart:
    curve: linear
    nodeSpacing: 40
    rankSpacing: 50
    padding: 10
    htmlLabels: false
    useMaxWidth: false
    diagramPadding: 10
---

flowchart TB

%% =====================
%% 🎨 CONVENCIONES
%% =====================
subgraph CONVENCIONES["🎨 CONVENCIONES"]
    C1["🟢 Hardware"]
    C2["🔵 Inicialización"]
    C3["🟣 Funcional"]
    C4["🟡 Memoria"]
    C5["🔴 UI Eventos"]
    C6["🟠 Comunicación"]
    C1 --> C2 --> C3 --> C4 --> C5 --> C6
end

style C1 fill:#6DC36D,stroke:#6DC36D,color:#000000
style C2 fill:#4FB0C6,stroke:#4FB0C6,color:#000000
style C3 fill:#B58DB6,stroke:#B58DB6,color:#000000
style C4 fill:#C9AF5D,stroke:#C9AF5D,color:#000000
style C5 fill:#C86D6D,stroke:#C86D6D,color:#000000
style C6 fill:#D38F64,stroke:#D38F64,color:#000000

%% =====================
%% 🧪 TRIPTALABS-HEAT-CONTROLLER
%% =====================
subgraph SYSTEM["🧪 Triptalabs-Heat-Controller"]
    subgraph HW["🧱 HARDWARE"]
        ESP32["ESP32-S3<br/>WiFi + BT<br/>Dual Core"]
        DISPLAY["Pantalla Táctil<br/>Waveshare 5''<br/>ST7701 + GT911"]
        PT100["Sensor RTD<br/>PT100"]
        RS485["Módulo RS485<br/>PT21A01-B-MODBUS"]
        SSR["Relé SSR<br/>40A, 110V AC"]
        HEATER["Resistencia<br/>Calefactora"]
        SDCARD["MicroSD<br/>Firmware / Backup"]
    end

    subgraph RTOS["⚙️ ESP-IDF + RTOS"]
        FREERTOS["FreeRTOS"]
        WIFI_STACK["WiFi Stack"]
        BT_STACK["Bluetooth Stack"]
        NVS["NVS Storage"]
    end

    subgraph DRIVERS["🧰 DRIVERS"]
        CONFIG_DRV["I2C Init<br/>CH422G Config"]
        DISPLAY_DRV["Display Driver<br/>CH422G + RGB"]
        SENSOR_DRV["Sensor Driver<br/>Modbus RTU (UART)"]
        IO_DRV["GPIO Driver<br/>CH422G IO Expander"]
    end

    subgraph CORE["🧠 APLICACIÓN"]
        MAIN["main.c<br/>Inicialización"]
        PID["PID Control<br/> NVS + SSR"]
        UPDATE["Actualización OTA<br/>GitHub + SD"]
        CHART["Charting<br/>Historial Temp"]
    end

    subgraph SCREENS["🧩 PANTALLAS"]
        BOOT["Boot<br/>Logo animado"]
        HOME["Inicio<br/>PID, Temp, Timer"]
        SETTINGS["Ajustes<br/>WiFi, BT, Fecha"]
        STATS["Estadísticas<br/>Histórico"]
        DEV["Modo Dev<br/>Kp, Ki, Kd"]
        WIFI_SCREEN["WiFi Setup"]
        BT_SCREEN["BT Setup"]
    end

    subgraph COMPONENTS["⚙️ COMPONENTES"]
        STATUSBAR["Status Bar<br/>Fecha + Conexión"]
        HOOKS["Hooks<br/>Manejadores Eventos"]
        EVENTS["Eventos UI<br/>Botones, Transiciones"]
    end

    subgraph GUI["🖼️ LVGL INTERFAZ"]
        LVGL_PORT["LVGL Backend<br/>Render + Touch"]
        SCREENS
        COMPONENTS
    end

    subgraph NET["🌐 RED"]
        WIFI_MGR["Gestor WiFi<br/>Scan, Connect, SNTP"]
        SNTP["Servidor SNTP"]
    end

    subgraph PROTO["📡 PROTOCOLOS"]
        MODBUS["Modbus RTU<br/>Manual"]
        HTTP["HTTP Client"]
        JSON["JSON Parser"]
    end

    subgraph EXT["🔗 SERVICIOS EXTERNOS"]
        GITHUB["GitHub Releases"]
    end
end

%% =====================
%% 🔌 CONEXIONES Y ESTILO
%% =====================

%% 🟢 Hardware
ESP32 --> DISPLAY
ESP32 --> RS485
ESP32 --> SDCARD
ESP32 --> SSR
RS485 --> PT100
SSR --> HEATER

%% 🔵 Inicialización
MAIN --> CONFIG_DRV
MAIN --> DISPLAY_DRV
MAIN --> SENSOR_DRV
MAIN --> PID
MAIN --> LVGL_PORT
MAIN --> WIFI_MGR
MAIN --> FREERTOS

%% 🟣 Funcional
CONFIG_DRV --> ESP32
DISPLAY_DRV --> DISPLAY
SENSOR_DRV --> RS485
SENSOR_DRV --> MODBUS
SENSOR_DRV --> FREERTOS
IO_DRV --> ESP32
PID --> SSR
PID --> SENSOR_DRV
PID --> FREERTOS
CHART --> SENSOR_DRV
LVGL_PORT --> DISPLAY_DRV
LVGL_PORT --> SCREENS
LVGL_PORT --> COMPONENTS

%% 🟡 Memoria
PID --> NVS
UPDATE --> SDCARD

%% 🟠 Comunicación
UPDATE --> HTTP
UPDATE --> JSON
WIFI_MGR --> WIFI_STACK
WIFI_MGR --> SNTP
HTTP --> GITHUB

%% 🔴 UI Eventos
HOOKS --> EVENTS
EVENTS --> PID
EVENTS --> UPDATE
STATUSBAR --> WIFI_MGR
```

---

## 🛡️ Bootloader Personalizado y Sistema de Recuperación

Una de las características más innovadoras del proyecto es su **bootloader personalizado** que proporciona verificación de integridad y recuperación automática:

### 🔐 Características del Bootloader

* **Verificación SHA256** del firmware en cada arranque
* **Contadores de fallos** con límite de 3 intentos
* **Recuperación automática** desde microSD
* **Modo recovery manual** con interfaz UART
* **Estadísticas persistentes** en NVS

```mermaid
---
config:
  layout: fixed
---
flowchart TD
 subgraph subGraph0["💾 Variables NVS Críticas"]
        AU["🔐 firmware_hash<br/>(SHA256 del firmware)"]
        AV["🔢 boot_attempts<br/>(0-3, resetea en boot exitoso)"]
        AW["🚨 recovery_flag<br/>(true/false)"]
        AX["⏰ last_boot_time<br/>(timestamp último boot)"]
  end
 subgraph subGraph1["🔍 Criterios Razón de Arranque"]
        AY["🆕 PRIMER ARRANQUE:<br/>• firmware_hash no existe en NVS<br/>• O NVS vacío/corrupto"]
        AZ["🔐 ARRANQUE NORMAL:<br/>• firmware_hash existe<br/>• recovery_flag = false<br/>• boot_attempts &lt; 3"]
        BA["🚨 MODO RECUPERACIÓN:<br/>• recovery_flag = true<br/>• O boot_attempts &gt;= 3<br/>• O activación manual"]
  end
 subgraph subGraph2["💾 Distribución Memoria"]
        BB["📦 Particiones (13MB Total)"]
        BC["🔧 app0: 10MB<br/>(Aplicación Principal)"]
        BD["💾 nvs: 1MB<br/>(Config + Variables Boot)"]
        BE["📁 spiffs: 2MB<br/>(Datos Usuario)"]
  end
    A["🔌 Encendido ESP32-S3"] --> B["🚀 Bootloader ESP-IDF"]
    B --> C["📱 Inicio App (main/core/main.c)"]
    C --> D["🛡️ Inicialización Bootloader<br/>(bootloader_main.c)"]
    D --> E["🔍 Determinar Razón de Arranque"]
    E --> F{"📊 Leer NVS"}
    F -- Hash no existe --> G["🆕 PRIMER ARRANQUE<br/>• Sin hash previo<br/>• Firmware nuevo"]
    F -- Hash existe --> H["🔐 ARRANQUE NORMAL<br/>• Hash encontrado<br/>• Verificar integridad"]
    F -- "Flag recovery=true" --> I["🚨 MODO RECUPERACIÓN<br/>• Flag manual activado<br/>• O falla crítica previa"]
    G --> J["📝 Generar Hash SHA256<br/>(integrity_checker.c)"]
    J --> K["💾 Guardar Hash en NVS"]
    K --> L["✅ Continuar a Aplicación"]
    H --> M["🔐 Verificar Integridad Firmware<br/>(Comparar SHA256 actual vs NVS)"]
    M --> N{"✅ Hash Coincide?"}
    N -- Sí --> O["📊 boot_attempts = 0"]
    N -- No --> P["⚠️ boot_attempts++"]
    O --> L
    P --> Q{"🔢 boot_attempts >= 3?"}
    Q -- No --> R["💾 recovery_flag = false<br/>📝 Guardar en NVS"]
    Q -- Sí --> S["🆘 recovery_flag = true<br/>💾 Guardar en NVS<br/>🔄 esp_restart()"]
    R --> T["🔄 Reiniciar ESP32<br/>(Reintentar arranque)"]
    T --> B
    S --> U["🔄 Reiniciar → Modo Recuperación"]
    U --> B
    I --> V["📺 Mostrar Menú Recuperación<br/>(recovery_mode.c)"]
    V --> W{"👤 Elección Usuario"}
    W -- Recuperación SD --> X["🗂️ Proceso Recuperación SD"]
    W -- Recuperación UART --> Y["💻 Interfaz UART"]
    W -- Info Sistema --> Z["📋 Estado del Sistema"]
    W -- Reiniciar Normal --> AA["🔄 recovery_flag = false<br/>Reiniciar"]
    X --> AB["🔍 Montar Tarjeta SD"]
    AB --> AC{"📁 SD Encontrada?"}
    AC -- No --> AD["❌ Error: SD no encontrada"]
    AC -- Sí --> AE["🔎 Buscar Archivos Firmware"]
    AE --> AF["📂 Verificar /sdcard/recovery/<br/>1️⃣ update.bin (prioridad)<br/>2️⃣ base_firmware.bin"]
    AF --> AG{"📄 Archivos Encontrados?"}
    AG -- No --> AH["❌ Sin archivos de recuperación"]
    AG -- Sí --> AI["🔐 Verificar SHA256 archivo"]
    AI --> AJ{"✅ Archivo Válido?"}
    AJ -- No --> AK["❌ Archivo corrupto"]
    AJ -- Sí --> AL["⚡ Flashear a partición app0"]
    AL --> AM["📝 Actualizar hash en NVS"]
    AM --> AN["💾 recovery_flag = false"]
    AN --> AO["🔄 Reiniciar ESP32"]
    AO --> B
    Y --> AP["💬 Comandos UART disponibles:<br/>• info - Información sistema<br/>• recovery - Iniciar recuperación SD<br/>• reboot - Reiniciar normal<br/>• factory - Reset de fábrica"]
    Z --> AQ["📊 Información del Sistema:<br/>• Hash actual firmware<br/>• Intentos arranque: {boot_attempts}<br/>• Flag recuperación: {recovery_flag}<br/>• Último arranque exitoso<br/>• Espacio libre particiones"]
    AD --> AR["🔄 Volver al Menú"]
    AH --> AR
    AK --> AR
    AR --> V
    AA --> AS["🔄 Reiniciar con recovery_flag=false"]
    AS --> B
    L --> AT["🎯 TriptaLabs Heat Controller<br/>Aplicación Principal"]
    BB --> BC & BD & BE

    style G fill:#51cf66
    style H fill:#74c0fc
    style I fill:#ff8787
    style L fill:#51cf66
    style X fill:#ffd43b
    style AT fill:#51cf66
```

### 🚨 Modo Recovery

El sistema incluye un **modo recovery avanzado** accesible mediante:
- **Automático**: Después de 3 fallos de arranque consecutivos
- **Manual**: Mediante comando UART o flag en NVS
- **Interfaz UART**: Comandos `info`, `recovery`, `reboot`, `factory`

---

## 💾 Sistema de Actualización OTA Innovador

A diferencia de las implementaciones OTA estándar de ESP-IDF, este proyecto utiliza un **enfoque único**:

### 🎯 Características del OTA

* **Partición única** (app0) - Sin particiones OTA dedicadas
* **Actualización directa** flasheando sobre app0 desde bootloader
* **Descarga desde GitHub** con verificación JSON
* **Backup automático** en microSD antes de actualización
* **Rollback seguro** en caso de fallo

### 📋 Tabla de Particiones

```csv
# Name,        Type, SubType, Offset,  Size,     Notes
nvs,           data, nvs,     0x9000,  0x100000, # Config + Boot vars
app0,          app,  factory, 0x110000, 0xA00000, # Main application  
spiffs,        data, spiffs,  ,        0x200000, # User data
```

### 🔄 Flujo de Actualización

```mermaid
flowchart TD
    A[App descarga update.bin] --> B[Guarda en /sdcard/update.bin]
    B --> C[pending_update = true en NVS]
    C --> D[esp_restart()]
    D --> E[Bootloader detecta flag]
    E --> F[Verifica SHA256 del archivo]
    F --> G[Flashea app0 completo]
    G --> H[Actualiza hash en NVS]
    H --> I[pending_update = false]
    I --> J[Reinicia a nueva versión]
    
    style A fill:#e1f5fe
    style G fill:#ff8a65
    style J fill:#81c784
```

---

## 🎛️ Control PID Profesional

El sistema implementa un **controlador PID avanzado** con características de nivel industrial:

### ⚙️ Características del PID

* **Anti-windup** en término integral
* **Parámetros configurables** (Kp=1.5, Ki=0.03, Kd=25.0 por defecto)
* **Límites configurables** (0-100% output)
* **Tiempo de muestreo** ajustable (5 segundos por defecto)
* **Persistencia en NVS** para mantener configuración
* **Control directo de SSR** vía CH422G IO expander

### 📊 Parámetros de Control

| Parámetro | Valor por Defecto | Descripción |
|-----------|-------------------|-------------|
| **Kp** | 1.5 | Ganancia proporcional |
| **Ki** | 0.03 | Ganancia integral |
| **Kd** | 25.0 | Ganancia derivativa |
| **Sample Time** | 5000ms | Tiempo entre muestras |
| **Output Range** | 0-100% | Rango de salida SSR |
| **Stable Threshold** | ±0.5°C | Umbral de estabilidad |

---

## 🖼️ Interfaz de Usuario Avanzada

La interfaz utiliza **LVGL** con un diseño profesional y flujo UX cuidadosamente planificado:

### 🎨 Flujo de Usuario

```mermaid
---
config:
  layout: elk
---
flowchart TD
 subgraph INICIO["🚀 INICIO"]
        ENCENDIDO["🔌 Usuario enciende<br/>el dispositivo"]
        BOOT_LOGO["📱 Logo TriptaLabs<br/>Animación de bienvenida"]
  end
 subgraph PANTALLA_PRINCIPAL["🏠 PANTALLA PRINCIPAL"]
        HOME["🏠 Pantalla Home<br/>Centro de control"]
        TEMP_ACTUAL["🌡️ Temperatura Actual<br/>42°C"]
        GRAFICO["📈 Gráfico en tiempo real<br/>Últimos valores"]
        AJUSTAR_TEMP["🌡️ Ajustar Temperatura<br/>Botones + / -"]
        AJUSTAR_TIMER["⏱️ Ajustar Timer<br/>Botones + / -"]
        ACTIVAR["🔥 Activar Calentamiento<br/>Switch ON/OFF"]
        BOTON_CONFIG["⚙️ Ir a Configuración"]
        BOTON_STATS["📊 Ver Estadísticas"]
        LOGO_SECRETO["🤫 Logo (5 toques)<br/>→ Modo Desarrollador"]
  end
 subgraph CONFIGURACION["⚙️ CONFIGURACIÓN"]
        MENU_CONFIG["📋 Menú de Configuración"]
        CONFIG_WIFI["📶 Configurar WiFi<br/>• Ver redes<br/>• Escribir contraseña<br/>• Conectar"]
        CONFIG_BT["📱 Configurar Bluetooth<br/>• Cambiar nombre<br/>• Activar/Desactivar"]
        CONFIG_FECHA["📅 Ajustar Fecha y Hora<br/>Ruedas día/mes/año"]
        VER_INFO["ℹ️ Ver Información<br/>Versión del sistema"]
        ACTUALIZAR["🔄 Actualizar Sistema<br/>Buscar actualizaciones"]
  end
 subgraph ESTADISTICAS["📊 ESTADÍSTICAS"]
        VER_STATS["📈 Ver Estadísticas<br/>Historial de uso"]
        GRAFICOS_HIST["📊 Gráficos Históricos<br/>Datos anteriores"]
  end
 subgraph MODO_DEV["🔧 MODO DESARROLLADOR"]
        PANTALLA_DEV["🔧 Configuración Avanzada<br/>Solo para técnicos"]
        CONFIG_PID["🎛️ Ajustar Parámetros PID<br/>Kp, Ki, Kd"]
  end
 subgraph PROCESO["🔥 PROCESO ACTIVO"]
        CALENTANDO["🔥 Sistema Calentando<br/>• Temperatura subiendo<br/>• Gráfico actualizándose"]
        TIMER_ACTIVO["⏱️ Timer Corriendo<br/>Cuenta regresiva"]
        PARADA_AUTO["⏹️ Parada Automática<br/>Timer completado"]
  end
    ENCENDIDO --> BOOT_LOGO
    BOOT_LOGO -- 3 segundos --> HOME
    BOTON_CONFIG -- Toque --> MENU_CONFIG
    BOTON_STATS -- Toque --> VER_STATS
    LOGO_SECRETO -- 5 toques rápidos --> PANTALLA_DEV
    MENU_CONFIG --> CONFIG_WIFI & CONFIG_BT & CONFIG_FECHA & VER_INFO & ACTUALIZAR
    VER_STATS --> GRAFICOS_HIST
    PANTALLA_DEV --> CONFIG_PID
    AJUSTAR_TEMP --> ACTIVAR
    ACTIVAR -- Switch ON --> CALENTANDO
    CALENTANDO --> TIMER_ACTIVO
    TIMER_ACTIVO -- Tiempo completado --> PARADA_AUTO
    CONFIG_WIFI -. Volver .-> HOME
    CONFIG_BT -. Volver .-> HOME
    VER_STATS -. Home .-> HOME
    PANTALLA_DEV -. Home .-> HOME
    PARADA_AUTO -. Continuar .-> HOME
    ACTUALIZAR -- Reinicio --> INICIO
     ENCENDIDO:::start
     BOOT_LOGO:::start
     HOME:::main
     ACTIVAR:::important
     CALENTANDO:::process
     INICIO:::startStyle
     PANTALLA_PRINCIPAL:::mainStyle
     CONFIGURACION:::configStyle
     ESTADISTICAS:::statsStyle
     MODO_DEV:::devStyle
     PROCESO:::processStyle
    classDef startStyle fill:#FEE2E2,stroke:#DC2626,color:#7F1D1D,stroke-width:3px
    classDef mainStyle fill:#DBEAFE,stroke:#1E40AF,color:#1E3A8A,stroke-width:3px
    classDef configStyle fill:#D1FAE5,stroke:#059669,color:#064E3B,stroke-width:3px
    classDef statsStyle fill:#EDE9FE,stroke:#7C3AED,color:#4C1D95,stroke-width:3px
    classDef devStyle fill:#FEF3C7,stroke:#F59E0B,color:#92400E,stroke-width:3px
    classDef processStyle fill:#FECACA,stroke:#EF4444,color:#7F1D1D,stroke-width:3px
    classDef start fill:#DC2626,stroke:#FFFFFF,color:#FFFFFF,stroke-width:4px,font-weight:bold
    classDef main fill:#1E40AF,stroke:#FFFFFF,color:#FFFFFF,stroke-width:5px,font-weight:bold
    classDef process fill:#DC2626,stroke:#FFFFFF,color:#FFFFFF,stroke-width:4px,font-weight:bold
    classDef important fill:#EF4444,stroke:#FFFFFF,color:#FFFFFF,stroke-width:4px,font-weight:bold
```

### 🎯 Características de la UI

* **Pantallas especializadas**: Home, Configuración, Estadísticas, Modo Dev
* **Modo Desarrollador secreto**: Acceso mediante 5 toques en el logo
* **Gráfico en tiempo real**: Buffer circular de 240 puntos
* **Barra de estado inteligente**: Fecha/hora + conectividad  
* **Componentes reutilizables**: StatusBar, Hooks, Eventos centralizados

---

## 🎯 Hardware Requerido

| Componente | Especificación | Función |
|------------|---------------|---------|
| **MCU** | ESP32-S3 (WiFi + BT, Dual Core) | Procesamiento principal |
| **Display** | Waveshare 5" 1024x600 (ST7701 + GT911) | Interfaz táctil |
| **Sensor** | PT100 RTD | Medición de temperatura |
| **Interfaz RS485** | PT21A01-B-MODBUS | Comunicación con PT100 |
| **Control de potencia** | SSR 40A, 110V AC | Activación de resistencia |
| **Expansor I/O** | CH422G | Control SSR y GPIO adicional |
| **Almacenamiento** | MicroSD | Firmware backup y OTA |
| **Resistencia** | Elemento calefactor industrial | Generación de calor |

---

## 🧩 Tecnologías y Librerías

### 🔧 Stack Principal

* **[ESP-IDF v5.4.1](https://docs.espressif.com/projects/esp-idf/)** - Framework de desarrollo
* **[LVGL 8.x](https://lvgl.io/)** - Biblioteca de interfaz gráfica  
* **FreeRTOS** - Sistema operativo en tiempo real
* **NVS** - Almacenamiento no volátil para configuración
* **FATFS + SDMMC** - Sistema de archivos para microSD

### 📡 Protocolos

* **Modbus RTU** - Implementación manual para PT100
* **HTTP/HTTPS Client** - Descarga de actualizaciones
* **JSON Parser** - Procesamiento de metadatos de versión
* **SNTP** - Sincronización de tiempo
* **WiFi + Bluetooth** - Conectividad dual

---

## 🖥️ Estructura del Proyecto

```
triptalabs-heat-controller/
├── main/
│   ├── core/                     # 🧠 Lógica principal
│   │   ├── main.c               # Punto de entrada
│   │   ├── pid_controller.c/.h  # Control PID avanzado
│   │   ├── update.c/.h          # Sistema OTA
│   │   ├── wifi_manager.c/.h    # Gestión WiFi + SNTP
│   │   ├── statistics.c/.h      # Monitoreo de sesiones
│   │   ├── system_time.c/.h     # Gestión de tiempo
│   │   └── bt.c/.h              # Bluetooth BLE
│   ├── bootloader/              # 🛡️ Bootloader personalizado
│   │   ├── bootloader_main.c/.h # Lógica principal
│   │   ├── integrity_checker.c/.h # Verificación SHA256
│   │   ├── sd_recovery.c/.h     # Recuperación desde SD
│   │   └── recovery_mode.c/.h   # Interfaz recovery
│   ├── drivers/                 # 🔧 Drivers de hardware
│   │   ├── config/              # Configuración I2C
│   │   ├── display/             # Driver display + CH422G
│   │   ├── sensor/              # Modbus RTU manual
│   │   └── io/                  # GPIO y expansor I/O
│   ├── ui/                      # 🎨 Interfaz LVGL
│   │   ├── screens/             # Pantallas principales
│   │   ├── components/          # Componentes reutilizables
│   │   ├── images/              # Recursos gráficos
│   │   ├── ui_events.c/.h       # Manejadores de eventos
│   │   └── ui.c/.h              # Inicialización UI
│   └── CMakeLists.txt           # Configuración de compilación
├── docs/                        # 📚 Documentación
│   ├── summary.mmd              # Diagrama arquitectura
│   ├── bootloader.mmd           # Diagrama bootloader
│   └── audit.md                 # Auditoría de código
├── .tmp/                        # 🗂️ Archivos temporales
│   ├── bootloader/              # Docs adicionales bootloader
│   ├── ux/                      # Diagramas UX
│   └── arq/                     # Diagramas arquitectura
├── partitions.csv               # 💾 Tabla de particiones
├── sdkconfig.defaults           # ⚙️ Configuración ESP-IDF
└── README.md                    # 📖 Este archivo
```

---

## 🚀 Instalación y Configuración

### 📋 Prerrequisitos

1. **ESP-IDF v5.4.1** instalado y configurado
2. **Python 3.8+** con pip
3. **Git** para clonado del repositorio
4. **Hardware específico** (ver tabla anterior)

### 🔧 Configuración Inicial

1. **Clonar el repositorio**:
   ```bash
   git clone https://github.com/triptalabs/triptalabs-heat-controller.git
   cd triptalabs-heat-controller
   ```

2. **⚠️ OBLIGATORIO - Configurar URLs OTA secretas**:
   ```bash
   cp main/core/update_config.h.example main/core/update_config.h
   ```
   
   Editar `main/core/update_config.h`:
   ```c
   #define SECRET_FIRMWARE_URL "https://tu-servidor.com/firmware.bin"
   #define SECRET_VERSION_URL "https://tu-servidor.com/version.json"
   ```

3. **Configurar target y build**:
   ```bash
   idf.py set-target esp32s3
   idf.py menuconfig  # Opcional: ajustar configuración
   idf.py build
   ```

4. **Flashear firmware inicial**:
   ```bash
   idf.py flash monitor
   ```

### 🔐 Configuración de Seguridad

* **update_config.h** está en `.gitignore` para proteger URLs
* Sin este archivo, la compilación **fallará intencionalmente**
* Usar HTTPS con certificados válidos para descarga OTA
* Verificar SHA256 de archivos de actualización

---

## 📊 Uso del Sistema

### 🏠 Pantalla Principal

* **Temperatura actual**: Display grande con valor PT100
* **Gráfico en tiempo real**: Últimos 240 puntos de temperatura
* **Control setpoint**: Botones +/- para ajustar objetivo
* **Timer de proceso**: Configuración de tiempo de calentamiento
* **Estado PID**: ON/OFF del controlador
* **Estado SSR**: Indicador visual de activación

### ⚙️ Configuración

* **WiFi**: Escaneo automático de redes, conexión con credenciales
* **Bluetooth**: Activación/desactivación, configuración de nombre
* **Fecha/Hora**: Configuración manual o automática vía SNTP
* **Información**: Versión firmware, estadísticas del sistema
* **Actualización**: Verificación manual de nuevas versiones

### 🔧 Modo Desarrollador

**Acceso secreto**: 5 toques rápidos en el logo de TriptaLabs

* **Parámetros PID**: Ajuste fino de Kp, Ki, Kd
* **Configuración avanzada**: Límites, tiempos de muestreo
* **Diagnósticos**: Estado interno del controlador
* **Calibración**: Ajustes de sensor y actuadores

### 📈 Estadísticas

* **Historial de temperatura**: Gráficos de sesiones anteriores
* **Tiempo de operación**: Registro de tiempo de calentamiento
* **Estadísticas de uso**: Número de ciclos, tiempo total
* **Estado del sistema**: Salud general del controlador

---

## 🛠️ Troubleshooting y Recovery

### 🚨 Modo Recovery Automático

El sistema entra automáticamente en recovery si:
- **3 fallos de arranque** consecutivos
- **Hash SHA256 inválido** del firmware
- **Flag recovery** activado manualmente

### 💻 Comandos UART (115200 baud)

```bash
info        # Información del sistema
recovery    # Iniciar recuperación desde SD
reboot      # Reiniciar normal
factory     # Reset de fábrica
```

### 🗂️ Estructura SD para Recovery

```
/sdcard/
├── recovery/
│   ├── update.bin          # Firmware de actualización (prioridad)
│   └── base_firmware.bin   # Firmware base de respaldo
└── backups/                # Backups automáticos
    └── previous_firmware.bin
```

### 🔄 Recuperación Manual

1. **Preparar microSD** con firmware válido en `/recovery/`
2. **Conectar UART** y abrir terminal serie
3. **Activar recovery**: 
   - Comando `recovery` por UART, o
   - Reiniciar 3 veces consecutivas
4. **Seguir menú** de recuperación en pantalla

---

## ⚠️ Estado Actual del Proyecto

### ✅ Funcionalidades Implementadas

* ✅ **Bootloader personalizado** con verificación SHA256
* ✅ **Control PID avanzado** con parámetros configurables
* ✅ **Lectura PT100** vía Modbus RTU manual
* ✅ **Sistema OTA completo** con backup automático
* ✅ **Interfaz LVGL completa** con múltiples pantallas
* ✅ **Modo desarrollador** con acceso secreto
* ✅ **Gestión WiFi/Bluetooth** con configuración UI
* ✅ **Sistema de estadísticas** y monitoreo
* ✅ **Sincronización NTP** automática
* ✅ **Gráfico en tiempo real** con buffer circular

### 🔄 Próximas Mejoras

* 📱 **App móvil** para control remoto
* 📊 **Base de datos** para históricos extendidos
* 🎛️ **Autotuning PID** automático
* 🔐 **Firma digital** para actualizaciones OTA
* 📡 **API REST** para integración IoT
* 🧪 **Perfiles de proceso** predefinidos

---

## 🔒 Consideraciones de Seguridad

### ⚠️ Áreas Identificadas para Mejora

* **Credenciales WiFi** hardcodeadas (requiere configuración dinámica)
* **Verificación HTTPS** en descargas OTA (añadir certificados)
* **Validación de entrada** en manipulación de strings
* **Manejo de memoria** robusto en todas las operaciones malloc

### 🛡️ Medidas de Seguridad Implementadas

* URLs OTA ocultas del código público
* Verificación SHA256 de firmware
* Límites de intentos de arranque
* Recovery automático ante fallos
* Almacenamiento seguro en NVS

---

## 📜 Licencia

Este proyecto es propiedad intelectual de **TriptaLabs S.A.S.**

**Licencia**: MIT - Uso libre bajo atribución

```
Copyright (c) 2024 TriptaLabs S.A.S.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

---

## 👨‍💻 Autores y Contribuidores

**TriptaLabs Team**
**Contacto**: [https://github.com/triptalabs](https://github.com/triptalabs)

---

## 📚 Recursos Adicionales

* 📖 [Documentación ESP-IDF](https://docs.espressif.com/projects/esp-idf/)
* 🎨 [LVGL Documentation](https://docs.lvgl.io/)
* 🔧 [SquareLine Studio](https://squareline.io/)
* 📡 [Modbus RTU Specification](https://modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf)
* 🛡️ [ESP32 Security Best Practices](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/security.html)

---

**🔥 TriptaLabs Heat Controller - Controlador industrial de nivel profesional para ESP32-S3**

---
