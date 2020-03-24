
# extra defined parameters users want to specify
EXTRA_C_DEFS ?= 

######################################
# source
######################################
# C sources
C_SOURCES = \
src/station.c \
src/util.c \
src/daylight_track.c \
src/aircond_track.c \
src/netconn.c \
src/app_msg.c \
src/IO/sensor_reader.c \
src/IO/outdev.c \
src/IO/relay/SRD-0xVDC-SL-C.c \
src/IO/YL69.c \
src/IO/LDR.c  \
src/IO/DHT11.c

C_SOURCES += \
src/network/mqtt_client.c \
src/system/middleware/ESP_AT_parser/middleware.c \
src/system/platform/stm32/stm32f446/platform.c \
third_party/MQTT_Client/generate/src/mqtt_generate.c \
third_party/MQTT_Client/third_party/ESP8266_AT_parser/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c

C_ENTRY_FILE = src/station.c

# ASM sources
ASM_SOURCES = 


# C defines
C_DEFS = $(foreach def, $(EXTRA_C_DEFS), $(addprefix -D, $(def)) )

# C includes
C_INCLUDES = -Iinclude \
-Iinclude/system/middleware/ESP_AT_parser \
-Iinclude/system/platform/stm32/stm32f446

# ------ middleware / platform specific configuration (TODO: refactor) ------
C_INCLUDES += \
-Ithird_party/MQTT_Client/third_party/libtomcrypt/src/headers \
-Ithird_party/MQTT_Client/third_party/libtommath \
-Ithird_party/MQTT_Client/include \
-Ithird_party/MQTT_Client/generate/include \
-Ithird_party/MQTT_Client/third_party/ESP8266_AT_parser/Src/ESP_AT_parser/inc \
-Ithird_party/MQTT_Client/include/system/middleware/ESP_AT_parser \
-Ithird_party/MQTT_Client/third_party/ESP8266_AT_parser/Src/FreeRTOS/Source/include \
-Ithird_party/MQTT_Client/third_party/ESP8266_AT_parser/Src/FreeRTOS/Source/portable/GCC/ARM_CM4_MPU \
-Ithird_party/MQTT_Client/third_party/ESP8266_AT_parser/Drivers/STM32F4xx_HAL_Driver/Inc \
-Ithird_party/MQTT_Client/third_party/ESP8266_AT_parser/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
-Ithird_party/MQTT_Client/third_party/ESP8266_AT_parser/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
-Ithird_party/MQTT_Client/third_party/ESP8266_AT_parser/Drivers/CMSIS/Include \
-Ithird_party/MQTT_Client/include/system/platform/arm/armv7m/stm \
-Ithird_party/jsmn


# C defines
C_DEFS += -DUSE_HAL_DRIVER  -DSTM32F446xx  -DHAL_ADC_MODULE_ENABLED
# ASM sources
ASM_SOURCES += third_party/MQTT_Client/src/system/platform/arm/armv7m/stm/bootcode_stm32f446.s
# link script
LDSCRIPT = -T./third_party/MQTT_Client/src/system/platform/arm/armv7m/stm/stm32f446_flash.ld
LD_SPECS_FILE = -specs=nano.specs
EXTRA_LIBS =  -lnosys
C_TOOLCHAIN_PREFIX = arm-none-eabi-
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
CPU_ARCH_FLAGS = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
# Debug
OPENOCD_CFG_FILES =  -f $(OPENOCD_HOME)/tcl/board/st_nucleo_f4.cfg   -f $(OPENOCD_HOME)/tcl/interface/stlink-v2-1.cfg 
REMOTE_DEBUGGER_CMD = openocd
DBG_SERVER_CMD = $(REMOTE_DEBUGGER_CMD) $(OPENOCD_CFG_FILES)  -c init -c "reset init"
GDB_CMD = gdb-multiarch
GDB_SCRIPT_PATH = ./third_party/MQTT_Client/auto/platform/utility.gdb
DBG_CLIENT_CMD = $(GDB_CMD) -x $(GDB_SCRIPT_PATH)
# ------------------------------------------------------------------------- #








BUILD_DIR_TOP=build
BUILD_DIR=$(BUILD_DIR_TOP)

#--------------------------------------------------------------------------------------
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable 
# (> make GCC_PATH=xxx) either it can be added to the PATH environment variable.
ifdef GCC_PATH
    CC = $(GCC_PATH)/$(C_TOOLCHAIN_PREFIX)gcc
    AS = $(GCC_PATH)/$(C_TOOLCHAIN_PREFIX)gcc -x assembler-with-cpp
    CP = $(GCC_PATH)/$(C_TOOLCHAIN_PREFIX)objcopy
    SZ = $(GCC_PATH)/$(C_TOOLCHAIN_PREFIX)size
    AR = $(GCC_PATH)/$(C_TOOLCHAIN_PREFIX)ar
    CCOV = $(GCC_PATH)/$(C_TOOLCHAIN_PREFIX)gcov
    DUMP = $(GCC_PATH)/$(C_TOOLCHAIN_PREFIX)objdump
else
    CC = $(C_TOOLCHAIN_PREFIX)gcc
    AS = $(C_TOOLCHAIN_PREFIX)gcc -x assembler-with-cpp
    CP = $(C_TOOLCHAIN_PREFIX)objcopy
    SZ = $(C_TOOLCHAIN_PREFIX)size
    AR = $(C_TOOLCHAIN_PREFIX)ar
    CCOV = $(C_TOOLCHAIN_PREFIX)gcov
    DUMP = $(C_TOOLCHAIN_PREFIX)objdump
endif

HEX = $(CP) -O ihex

BIN = $(CP) -O binary -S

# optimization
OPT = -Og

CFLAGS += $(CPU_ARCH_FLAGS) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -Wint-to-pointer-cast

ifeq ($(DEBUG), yes)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# LDFLAGS
#######################################

# libraries
LIBS += -lc -lm $(EXTRA_LIBS)

LIBDIR =

# TODO: xxx.map should be platform-specific 
LDFLAGS = $(CPU_ARCH_FLAGS) $(LD_SPECS_FILE)  $(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$<.map,--cref -Wl,--gc-sections

ifeq ($(MAKECMDGOALS), utest_helper) # if unit test is enabled
CFLAGS  += -coverage
LDFLAGS += -coverage
endif

#######################################
# build the application
#######################################
# collect C/assembly objects compiled for library (may be built by cross-compile toolchain)
C_ASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

C_ASM_OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

THIRD_PARTY_LIBS_PATH = \
third_party/MQTT_Client/build/libmqttclient.a \
third_party/MQTT_Client/third_party/libtomcrypt/libtomcrypt.a \
third_party/MQTT_Client/third_party/libtommath/libtommath.a



build_exe: $(BUILD_DIR)/station.elf  $(BUILD_DIR)/station.text  $(BUILD_DIR)/station.bin

$(BUILD_DIR)/%.o: %.c makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

%.elf:  $(C_ASM_OBJECTS) $(THIRD_PARTY_LIBS_PATH)
	$(CC) $^  $(LDFLAGS) -o $@ 
	$(SZ) $@

%.bin: %.elf | $(BUILD_DIR)
	$(BIN) $< $@

%.text: %.elf | $(BUILD_DIR)
	$(DUMP) -Dh $< > $@

$(BUILD_DIR):
	@mkdir -p $@

clean:
	-rm -fR $(BUILD_DIR_TOP)

# optional function for those who use code navigation tools e.g. ctags
update_navigator:
	@rm -rf ./tags; ctags -R ./include ./src  ./third_party/MQTT_Client/third_party/ESP8266_AT_parser  ./third_party/jsmn

dbg_server:
	@$(DBG_SERVER_CMD)

dbg_client:
	@$(DBG_CLIENT_CMD)


