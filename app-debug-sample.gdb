define reload_image
    monitor  reset
    monitor  halt
    load
end

define  connect_openocd_local_server
    target   remote localhost:3333
    reload_image
end

#file  build/your_test_image.elf
file   $PWD/build/garden_monitor_edge.elf

connect_openocd_local_server

show environment RTOS_HW_BUILD_PATH
# --- snapshot heap usage ---
set $config_use_posix_errno_enabled = 1
set $cfg_max_priority = 7
set $platform_mem_baseaddr = 0x20000000
set $platform_mem_maxsize = 0x1ffff
#source $RTOS_HW_BUILD_PATH/Src/os/FreeRTOS/util.gdb
#freertos_heap4_snapshot

#break $PWD/src/netconn.c:74
#break $PWD/src/network/mqtt_client.c:222
#break src/tls/core/tls_client.c:221
#break *0x8020bb2

