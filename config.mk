_APP_SYS_INCLUDE = $(MONT_STATION_PROJ_HOME)/include/system
_APP_SYS_SOURCE = $(MONT_STATION_PROJ_HOME)/src/system

_COMMON_C_ENTRY_FILE = src/station.c

_COMMON_C_HEADERS = \
    include/station_aircond_track.h \
    include/station_app_msg.h \
    include/station_config.h \
    include/station_daylight_track.h \
    include/station_default_config.h \
    include/station_display.h \
    include/station_include.h \
    include/station_io.h \
    include/station_monitor.h \
    include/station_network.h \
    include/station_soilcond_track.h \
    include/station_types.h \
    include/station_util.h \
	include/system/middleware/ESP_AT_parser/FreeRTOSConfig.h \
	include/system/middleware/ESP_AT_parser/esp_config.h \
	include/system/middleware/ESP_AT_parser/station_middleware.h \
    include/system/platform/stm32/stm32f446/pin_map.h \
    include/system/platform/stm32/stm32f446/station_platform.h \
    include/system/platform/stm32/stm32f446/stm32f4xx_hal_conf.h


_COMMON_C_SOURCES_FUNC = \
    src/util.c \
    src/daylight_track.c \
    src/aircond_track.c \
    src/soilcond_track.c \
    src/netconn.c \
    src/app_msg/inbound.c \
    src/app_msg/outbound.c \
    src/app_msg/misc.c \
    src/network/mqtt_client.c \
    src/IO/sensor_event.c \
    src/IO/sensor_sample.c \
    src/IO/actuator.c  \
    src/IO/display.c \
    src/IO/display/textfonts.c \
    src/IO/relay/SRD-0xVDC-SL-C.c \
    src/IO/display/SSD1315_OLED.c \
    src/IO/soilsensor.c \
    src/IO/LDR.c  \
    src/IO/DHT11.c

_COMMON_C_SOURCES_3PTY = \
    src/system/middleware/ESP_AT_parser/middleware.c \
    src/system/platform/stm32/stm32f446/error.c \
    src/system/platform/stm32/stm32f446/iodev.c \
    src/system/platform/stm32/stm32f446/net_mqtt.c

APPCFG_MIDDLEWARE_C_INCLUDES = $(_APP_SYS_INCLUDE)/middleware/ESP_AT_parser

APPCFG_HW_C_INCLUDES = $(_APP_SYS_INCLUDE)/platform/stm32/stm32f446

APPCFG_ASM_SOURCES = $(_APP_SYS_SOURCE)/platform/stm32/stm32f446/bootcode_stm32f446.s

APP_LINK_SCRIPT = $(_APP_SYS_SOURCE)/platform/stm32/stm32f446/stm32f446_flash.ld

