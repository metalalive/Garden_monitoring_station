MQC_PROJ_HOME ?= /path/to/proj/mqtt-client/

DEBUG ?= yes

# extra defined parameters users want to specify
EXTRA_C_DEFS ?= 

MONT_STATION_PROJ_HOME = $(shell pwd)

include ./config.mk

BUILD_DIR_TOP = $(MONT_STATION_PROJ_HOME)/build
BUILD_DIR = $(BUILD_DIR_TOP)

######################################
# build options
######################################

# LD_SPECS_FILE = -specs=nano.specs
# EXTRA_LIBS =  -lnosys
# CPU_ARCH_FLAGS = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

#######################################
# build the application
#######################################

build_exe: export MONT_STATION_PROJ_HOME := $(MONT_STATION_PROJ_HOME)
build_exe:
	@make config -C $(MQC_PROJ_HOME)  MQC_CFG_FULLPATH=$(MONT_STATION_PROJ_HOME)/mqttclient.conf
	@make gen_3pty_libs -C $(MQC_PROJ_HOME)  DEBUG=$(DEBUG)
	@make gen_lib -C $(MQC_PROJ_HOME) DEBUG=$(DEBUG)  BUILD_DIR=$(BUILD_DIR) \
		EXTRA_C_DEFS=$(EXTRA_C_DEFS) APPCFG_BASEPATH=$(MONT_STATION_PROJ_HOME)
	@make buildapp -C $(MQC_PROJ_HOME) DEBUG=$(DEBUG) BUILD_DIR=$(BUILD_DIR) \
		EXTRA_C_DEFS=$(EXTRA_C_DEFS) APP_NAME=garden_monitor_edge \
		APP_BASEPATH=$(MONT_STATION_PROJ_HOME)

clean:
	@make clean  -C $(MQC_PROJ_HOME)  BUILD_DIR=$(BUILD_DIR)

dbg_server:
	@make dbg_server -C $(MQC_PROJ_HOME)

dbg_client:
	@make dbg_client -C $(MQC_PROJ_HOME)  GDB_SCRIPT_PATH=${PWD}/app-debug.gdb

