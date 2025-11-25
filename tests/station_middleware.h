#ifndef STATION_MIDDLEWARE_H
#define STATION_MIDDLEWARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>  // for sprintf
#include <string.h> // For memcpy, memset, strlen, strncmp
#include <stdlib.h> // For malloc, free
#include <stdint.h> // for size_t
#include <assert.h> // For assert

#define  GMON_SYS_TICK_RATE_HZ   1000 // 1 tick = 1000 Hz

// Mock common macros used in app_msg.c
#define XSTRLEN(s)      strlen((const char *)(s))
#define XMEMCPY(d, s, n) memcpy((d), (s), (n))
#define XMEMSET(d, v, n) memset((d), (v), (n))
#define XASSERT(cond)   assert(cond)
#define XSTRNCMP(s1, s2, n) strncmp((const char *)(s1), (const char *)(s2), (n))

// Mock memory allocation for host
#define XCALLOC             calloc
#define XMALLOC(size)       malloc(size)
#define XMEMFREE(ptr)       free(ptr)

// Mock FreeRTOS-specific functions for host
#define stationSysEnterCritical()
#define stationSysExitCritical()
#define stationSysGetTickCount()  UTestSysGetTickCount()
#define configASSERT(x) assert(x)
// TODO, mock following system-level functions
#define staSysMsgBoxCreate(length)  (stationSysMsgbox_t)NULL
#define staSysMsgBoxDelete(msgbuf)
#define staSysMsgBoxGet(msgbuf, msg, block_time)  GMON_RESP_OK
#define staSysMsgBoxPut(msgbuf, msg, block_time)  GMON_RESP_OK

#define staCvtUNumToStr(out_chr_p, num) ({ \
    char _inner_buf[20] = {0}; \
    uint32_t wr_sz = (uint32_t)snprintf(_inner_buf, 20, "%d", (int)num); \
    assert(wr_sz <= 20); \
    XMEMCPY((char *)out_chr_p, _inner_buf, wr_sz); \
    wr_sz; \
})

typedef void* stationSysTask_t;
typedef void (*stationSysTaskFn_t)(void*);
typedef void* stationSysMsgbox_t;

extern uint32_t g_mock_tick_count;

uint32_t UTestSysGetTickCount(void);
void setMockTickCount(uint32_t count);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_MIDDLEWARE_H
