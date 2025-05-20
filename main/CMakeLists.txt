# Busca automáticamente los archivos fuente en los directorios indicados.
# Esto permite que al añadir nuevos archivos .c en esas carpetas, se incluyan automáticamente.
file(GLOB UI_SRCS "ui_chart_data.c" "CH422G.c" "ui/*.c")                   # Incluye CH422G y todos los .c en /ui
file(GLOB SCREEN_SRCS "ui/screens/*.c")                 # Pantallas exportadas por SquareLine Studio
file(GLOB IMG_SRCS "ui/images/*.c")                     # Recursos gráficos (iconos, fondos, etc.)
file(GLOB UI_COMPONENTS_SRCS "ui/components/*.c")       # Componentes reutilizables de UI

# Define la lista completa de archivos fuente del componente.
# Se combinan los archivos manuales del backend y los generados por SquareLine Studio.
set(srcs
    "main.c"
    "sensor.c"
    "waveshare_rgb_lcd_port.c"
    "lvgl_port.c"
    "pid_controller.c"
    "update.c"
    "ch422g.c"
    "DEV_Config.c"
    ${UI_SRCS}              # Archivos desde ui/
    ${SCREEN_SRCS}          # Pantallas
    ${IMG_SRCS}             # Imágenes
    ${UI_COMPONENTS_SRCS}   # Componentes de UI
)

# Registra el componente principal con ESP-IDF.
# Esto le dice al sistema de build de ESP-IDF qué archivos y librerías usar.
idf_component_register(
    SRCS ${srcs}            # Archivos fuente
    INCLUDE_DIRS
        "."                         # Incluye el directorio actual (main/)
        "ui"                        # Base de la interfaz gráfica
        "ui/screens"                # Pantallas
        "ui/images"                 # Recursos gráficos
        "ui/components"            # Componentes reutilizables
        "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__esp-modbus/freemodbus/modbus/include"  # Headers de Modbus
        "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__esp-modbus/freemodbus/port"            # Port de Modbus
    REQUIRES
        esp_wifi                    # Conectividad Wi-Fi
        nvs_flash                   # Almacenamiento no volátil
        bt                          # Bluetooth
        app_update                  # Funciones de actualización OTA
        esp_https_ota               # Cliente OTA con HTTPS
        spiffs                      # Sistema de archivos SPIFFS
        fatfs                       # Soporte adicional para microSD
        console                     # CLI con argtable3
        espressif__esp-modbus      # Soporte para protocolo Modbus RTU
)

# Añade explícitamente rutas privadas adicionales para la compilación de FreeModbus.
# Esto es necesario cuando ciertos headers no se detectan automáticamente.
target_include_directories(${COMPONENT_TARGET} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__esp-modbus/freemodbus/modbus/include"
    "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__esp-modbus/freemodbus/port"
)

# Añade ruta del archivo DEV_Config.h al target para evitar errores de inclusión.
target_include_directories(${COMPONENT_TARGET} PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

# Configura el componente LVGL para que ignore advertencias de formato printf.
# LVGL usa funciones de impresión con formatos no estándar que generan warnings.
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
