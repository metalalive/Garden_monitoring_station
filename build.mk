include  $(MONT_STATION_PROJ_HOME)/config.mk

# TODO / FIXME
# note that `APP_REQUIRED_C_HEADER_PATHS` and `APP_REQUIRED_C_SOURCE_FILES` are declared
# in mqtt-client code repository, ideally they should be encapsulated in that repository
# and never exposed to external apps.

APPCFG_C_INCLUDES = \
    $(MONT_STATION_PROJ_HOME)/include \
	$(MONT_STATION_PROJ_HOME)/third_party/jsmn \
	$(APP_REQUIRED_C_HEADER_PATHS) \
	$(APPCFG_MIDDLEWARE_C_INCLUDES) \
    $(APPCFG_HW_C_INCLUDES)


_C_ENTRY_FILE = src/station.c

_C_SOURCES_FUNC = \
    src/util.c \
    src/daylight_track.c \
    src/aircond_track.c \
    src/netconn.c \
    src/app_msg.c \
    src/network/mqtt_client.c \
    src/IO/sensor_reader.c \
    src/IO/outdev.c  \
    src/IO/display.c \
    src/IO/display/textfonts.c \
    src/IO/relay/SRD-0xVDC-SL-C.c \
    src/IO/display/SSD1315_OLED.c \
    src/IO/soilsensor.c \
    src/IO/LDR.c  \
    src/IO/DHT11.c

_C_SOURCES_3PTY = \
    src/system/middleware/ESP_AT_parser/middleware.c \
    src/system/platform/stm32/stm32f446/platform.c \
    src/system/platform/stm32/stm32f446/stm32f446_config.c

_APP_C_SRCS = $(_C_ENTRY_FILE) $(_C_SOURCES_FUNC) $(_C_SOURCES_3PTY)

APPCFG_C_SOURCES = \
	$(addprefix $(MONT_STATION_PROJ_HOME)/, $(_APP_C_SRCS)) \
	$(APP_REQUIRED_C_SOURCE_FILES)

# append more paths for libraries to build-in variable `APPCFG_LIBS_PATHS`
# or preprocessor parameters to `APPCFG_C_DEFS` for application requirement .

#$(info EXTRA_C_DEFS : $(EXTRA_C_DEFS))
#$(info APPCFG_C_DEFS : $(APPCFG_C_DEFS))
