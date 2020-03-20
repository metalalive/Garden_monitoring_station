#ifndef STATION_MIDDLEWARE_H
#define STATION_MIDDLEWARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

#undef   XMALLOC
#define  XMALLOC   ESP_MALLOC

#undef   XCALLOC
#define  XCALLOC   ESP_CALLOC

#undef   XREALLOC
#define  XREALLOC  ESP_REALLOC

#undef   XMEMFREE
#define  XMEMFREE  ESP_MEMFREE

#undef   XASSERT
#define  XASSERT  ESP_ASSERT

#undef   XMEMSET
#define  XMEMSET  ESP_MEMSET

#undef   XMEMCPY
#define  XMEMCPY  ESP_MEMCPY

#undef   XSTRLEN
#define  XSTRLEN  ESP_STRLEN

#define  GMON_TASKS_PRIO_MIN     ESP_APPS_THREAD_PRIO

#define  GMON_MAX_BLOCKTIME_SYS_MSGBOX    ESP_SYS_MAX_TIMEOUT

#define  GMON_SYS_TICK_RATE_HZ   ESP_SYS_TICK_RATE_HZ

#define  stationSysEnterCritical()  vESPsysEnterCritical()

#define  stationSysExitCritical()   vESPsysExitCritical()

#define  stationSysGetTickCount()   uiESPsysGetTickCount()

#define  stationSysDelayMs(time_ms)   vESPsysDelay(time_ms)

#define  stationSysTaskWaitUntilExit(task_p, return_p)

#define   staSysMsgBoxCreate(length)  (stationSysMsgbox_t)xESPsysMboxCreate((length))

#define   staSysMsgBoxDelete(msgbuf)  vESPsysMboxDelete((msgbuf))

#define   staSysMsgBoxGet(msgbuf, msg, block_time)    staSysCvtResp(eESPsysMboxGet((espSysMbox_t)(msgbuf), (msg), (block_time)))

#define   staSysMsgBoxPut(msgbuf, msg, block_time)    staSysCvtResp(eESPsysMboxPut((espSysMbox_t)(msgbuf), (msg), (block_time)))

#define   staCvtUNumToStr(outstr, num)    uiESPcvtNumToStr((uint8_t *)(outstr), (num), ESP_DIGIT_BASE_DECIMAL)


typedef  espSysThread_t  stationSysTask_t;

typedef  espSysThreFunc  stationSysTaskFn_t;

typedef  espSysMbox_t    stationSysMsgbox_t;

gMonStatus  stationSysCreateTask(const char *task_name,  stationSysTaskFn_t task_fp, void* const args,
                             size_t stack_sz, uint32_t prio, uint8_t isPrivileged, stationSysTask_t *task_ptr);

gMonStatus  stationSysTaskDelete(stationSysTask_t *task_ptr);

gMonStatus  stationSysInit(void);

gMonStatus  stationSysDelayUs(unsigned short time_us);

gMonStatus  staSysCvtResp(int resp_in);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_MIDDLEWARE_H
