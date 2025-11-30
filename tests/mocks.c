#include "station_include.h"
#include "mocks.h"

const unsigned short gmon_txt_font_bitmap_11x18[] = {2,3,4,5};

uint32_t UTestSysGetTickCount(void) {
    return g_mock_tick_count;
}

// Global mock variable for system tick count
uint32_t g_mock_tick_count = 0;

void setMockTickCount(uint32_t count) { g_mock_tick_count = count; }

stationSysMsgbox_t UTestSysMsgBoxCreate(size_t length) {
    mock_msg_queue_t* queue = (mock_msg_queue_t*)XMALLOC(sizeof(mock_msg_queue_t));
    if (queue == NULL) {
        return NULL;
    }
    queue->buffer = (void**)XMALLOC(sizeof(void*) * length);
    if (queue->buffer == NULL) {
        XMEMFREE(queue);
        return NULL;
    }
    queue->capacity = length;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    return (stationSysMsgbox_t)queue;
}

void UTestSysMsgBoxDelete(stationSysMsgbox_t *msgbuf_ptr) {
    if (msgbuf_ptr == NULL || *msgbuf_ptr == NULL) {
        return;
    }
    mock_msg_queue_t* queue = (mock_msg_queue_t*)*msgbuf_ptr;
    XMEMFREE(queue->buffer);
    XMEMFREE(queue);
    *msgbuf_ptr = NULL;
}

gMonStatus UTestSysMsgBoxGet(stationSysMsgbox_t msgbuf, void **msg, uint32_t block_time) {
    (void) block_time;
    mock_msg_queue_t* queue = (mock_msg_queue_t*)msgbuf;
    if (queue == NULL || msg == NULL) {
        return GMON_RESP_ERRARGS;
    }
    if (queue->count == 0) {
        return GMON_RESP_TIMEOUT;
    }
    *msg = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    return GMON_RESP_OK;
}

gMonStatus UTestSysMsgBoxPut(stationSysMsgbox_t msgbuf, void *msg, uint32_t block_time) {
    (void) block_time;
    mock_msg_queue_t* queue = (mock_msg_queue_t*)msgbuf;
    if (queue == NULL) {
        return GMON_RESP_ERRARGS;
    }
    if (queue->count == queue->capacity) {
        return GMON_RESP_TIMEOUT;
    }
    queue->buffer[queue->tail] = msg;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    return GMON_RESP_OK;
}

gMonStatus  staSensorInitSoilMoist(void) {
    return GMON_RESP_OK;
}
gMonStatus  staSensorDeInitSoilMoist(void) {
    return GMON_RESP_OK;
}

gMonStatus  staSensorInitLight(void) {
    return GMON_RESP_OK;
}
gMonStatus  staSensorDeInitLight(void) {
    return GMON_RESP_OK;
}

gMonStatus  staSensorInitAirTemp(void) {
    return GMON_RESP_OK;
}
gMonStatus  staSensorDeInitAirTemp(void) {
    return GMON_RESP_OK;
}

gMonStatus  staActuatorInitPump(gMonActuator_t *dev) {
    (void) dev;
    return GMON_RESP_OK;
}
gMonStatus  staActuatorDeinitPump(void) {
    return GMON_RESP_OK;
}

gMonStatus  staActuatorInitFan(gMonActuator_t *dev) {
    (void) dev;
    return GMON_RESP_OK;
}
gMonStatus  staActuatorDeinitFan(void) {
    return GMON_RESP_OK;
}

gMonStatus  staActuatorInitBulb(gMonActuator_t *dev) {
    (void) dev;
    return GMON_RESP_OK;
}
gMonStatus  staActuatorDeinitBulb(void) {
    return GMON_RESP_OK;
}

gMonStatus staSetNetConnTaskInterval(gMonNet_t *net, unsigned int interval_ms) {
    net->interval_ms = interval_ms; // Update gmon for verification
    return GMON_RESP_OK;
}

gMonStatus staSetRequiredDaylenTicks(gardenMonitor_t *gmon, unsigned int light_length) {
    if (light_length <= GMON_MAX_REQUIRED_LIGHT_LENGTH_TICKS) {
        gmon->user_ctrl.required_light_daylength_ticks = light_length;
        return GMON_RESP_OK;
    }
    return GMON_RESP_INVALID_REQ;
}

gMonStatus  staDisplayDevInit(void) {
    return GMON_RESP_OK;
}
gMonStatus  staDisplayDevDeInit(void) {
    return GMON_RESP_OK;
}
gMonStatus  staDiplayDevPrintString(gmonPrintInfo_t *printinfo) {
    (void) printinfo;
    return GMON_RESP_OK;
}
unsigned short  staDisplayDevGetScreenWidth(void) {
    return 120;
}

gMonStatus staDisplayRefreshScreen(void) {
    return GMON_RESP_OK;
}
