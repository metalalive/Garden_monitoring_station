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
#define stationSysDelayMs(time_ms) (void)(time_ms)
#define stationSysEnterCritical()
#define stationSysExitCritical()
#define stationSysGetTickCount()  UTestSysGetTickCount()
#define configASSERT(x) assert(x)
#define staSysMsgBoxCreate(length)  UTestSysMsgBoxCreate(length)
#define staSysMsgBoxDelete(msgbuf)  UTestSysMsgBoxDelete(msgbuf)
#define staSysMsgBoxGet(msgbuf, msg, block_time) UTestSysMsgBoxGet(msgbuf, msg, block_time)
#define staSysMsgBoxPut(msgbuf, msg, block_time) UTestSysMsgBoxPut(msgbuf, msg, block_time)

#define staCvtUNumToStr(out_chr_p, num) ({ \
    char _inner_buf[20] = {0}; \
    uint32_t wr_sz = (uint32_t)snprintf(_inner_buf, 20, "%d", (int)num); \
    assert(wr_sz <= 20); \
    XMEMCPY((char *)out_chr_p, _inner_buf, wr_sz); \
    wr_sz; \
})

#define staCvtUNumToHexStr(out_chr_p, num) ({ \
    char _inner_buf[20] = {0}; \
    uint32_t wr_sz = (uint32_t)snprintf(_inner_buf, 20, "%x", (int)num); \
    assert(wr_sz <= 20); \
    XMEMCPY((char *)out_chr_p, _inner_buf, wr_sz); \
    wr_sz; \
})

typedef void* stationSysTask_t;
typedef void (*stationSysTaskFn_t)(void*);
typedef void* stationSysMsgbox_t;
// Mock queue structure for testing purposes
typedef struct {
    void** buffer;
    size_t capacity;
    size_t head; // index of the oldest element
    size_t tail; // index where the next element will be inserted
    size_t count; // number of elements currently in the queue
} mock_msg_queue_t;

extern uint32_t g_mock_tick_count;

stationSysMsgbox_t UTestSysMsgBoxCreate(size_t length);
void       UTestSysMsgBoxDelete(stationSysMsgbox_t *msgbuf_ptr);
gMonStatus UTestSysMsgBoxGet(stationSysMsgbox_t msgbuf, void **msg, uint32_t block_time);
gMonStatus UTestSysMsgBoxPut(stationSysMsgbox_t msgbuf, void  *msg, uint32_t block_time);

uint32_t UTestSysGetTickCount(void);
void setMockTickCount(uint32_t count);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_MIDDLEWARE_H
