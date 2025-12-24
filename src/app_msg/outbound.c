#include "station_include.h"
#define JSMN_HEADER
#include "jsmn.h"

// clang-format off
// topic : garden/log
// message :
// {
//     "records": [
//         {"soilmoist": 1023, "airtemp": 23.7, "airhumid":80, "light":1023, "ticks":99999999, "days":365000 },
//         {"soilmoist": 1021, "airtemp": 24.4, "airhumid":70, "light":1000, "ticks":99999998, "days":365000 },
//         {"soilmoist": 1001, "airtemp": 24.9, "airhumid":65, "light": 180, "ticks":99999979, "days":365000 },
//         {"soilmoist": 1004, "airtemp": 25.8, "airhumid":54, "light": 101, "ticks":99999969, "days":365000 },
//     ]
// }
// 
//
// clang-format on

#define GMON_JSON_CURLY_BRACKET_LEFT   '{'
#define GMON_JSON_CURLY_BRACKET_RIGHT  '}'
#define GMON_JSON_SQUARE_BRACKET_LEFT  '['
#define GMON_JSON_SQUARE_BRACKET_RIGHT ']'
#define GMON_JSON_QUOTATION            '"'
#define GMON_JSON_COLON                ':'
#define GMON_JSON_COMMA                ','
#define GMON_JSON_WHITESPACE           ' '

#define GMON_APPMSG_DATA_NAME_RECORDS  "records"
#define GMON_APPMSG_DATA_NAME_AIRHUMID "airhumid"
#define GMON_APPMSG_DATA_NAME_TICKS    "ticks"
#define GMON_APPMSG_DATA_NAME_DAYS     "days"

#define GMON_APPMSG_DATA_SZ_SOILMOIST 4
// 3 bytes for integral part, 1-byte decimal seperator, 2 bytes for fractional part
#define GMON_APPMSG_DATA_SZ_AIRTEMP   6
#define GMON_APPMSG_DATA_SZ_AIRHUMID  6
#define GMON_APPMSG_DATA_SZ_LIGHT     4
#define GMON_APPMSG_DATA_SZ_TICKS     8
#define GMON_APPMSG_DATA_SZ_DAYS      6
#define GMON_APPMSG_DATA_SZ_NETCONN   8
#define GMON_APPMSG_DATA_SZ_DAYLENGTH 8

#define GMON_APPMSG_NUM_ITEMS_PER_RECORD 6

#define GMON_APPMSG_NUM_RECORDS 6 // TODO, remove

typedef struct {
    const char    *name;
    unsigned short val_sz;
    unsigned char *val_pos[GMON_APPMSG_NUM_RECORDS];
} gmonMsgItem_t;

static gmonMsgItem_t gmon_appmsg_log_records[GMON_APPMSG_NUM_ITEMS_PER_RECORD] = {
    {GMON_APPMSG_DATA_NAME_SOILMOIST, (unsigned short)GMON_APPMSG_DATA_SZ_SOILMOIST, {0}},
    {GMON_APPMSG_DATA_NAME_AIRTEMP, (unsigned short)GMON_APPMSG_DATA_SZ_AIRTEMP, {0}},
    {GMON_APPMSG_DATA_NAME_AIRHUMID, (unsigned short)GMON_APPMSG_DATA_SZ_AIRHUMID, {0}},
    {GMON_APPMSG_DATA_NAME_LIGHT, (unsigned short)GMON_APPMSG_DATA_SZ_LIGHT, {0}},
    {GMON_APPMSG_DATA_NAME_TICKS, (unsigned short)GMON_APPMSG_DATA_SZ_TICKS, {0}},
    {GMON_APPMSG_DATA_NAME_DAYS, (unsigned short)GMON_APPMSG_DATA_SZ_DAYS, {0}}
};

static unsigned short staAppMsgOutflightCalcRequiredBufSz(void) {
    unsigned short outlen = 0, len = 0, idx = 0;
    outlen = 5 + XSTRLEN(GMON_APPMSG_DATA_NAME_RECORDS) + 2;
    len = 2 + 3 * GMON_APPMSG_NUM_ITEMS_PER_RECORD + (GMON_APPMSG_NUM_ITEMS_PER_RECORD - 1);
    for (idx = 0; idx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; idx++) {
        len += XSTRLEN(gmon_appmsg_log_records[idx].name) + gmon_appmsg_log_records[idx].val_sz;
    }
    len = len * GMON_APPMSG_NUM_RECORDS + (GMON_APPMSG_NUM_RECORDS - 1);
    outlen += len;
    return outlen;
}

static void staParseFixedPartsOutAppMsg(gmonStr_t *outmsg) {
    unsigned short pos = 0, idx = 0, jdx = 0;
    unsigned char *buf = outmsg->data;
    *buf++ = GMON_JSON_CURLY_BRACKET_LEFT;
    *buf++ = GMON_JSON_QUOTATION;
    pos = XSTRLEN(GMON_APPMSG_DATA_NAME_RECORDS);
    XMEMCPY(buf, GMON_APPMSG_DATA_NAME_RECORDS, pos);
    buf += pos;
    *buf++ = GMON_JSON_QUOTATION;
    *buf++ = GMON_JSON_COLON;
    *buf++ = GMON_JSON_SQUARE_BRACKET_LEFT;
    for (idx = 0; idx < GMON_APPMSG_NUM_RECORDS; idx++) {
        if (idx > 0) {
            *buf++ = GMON_JSON_COMMA;
        }
        *buf++ = GMON_JSON_CURLY_BRACKET_LEFT;
        for (jdx = 0; jdx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; jdx++) {
            if (jdx > 0) {
                *buf++ = GMON_JSON_COMMA;
            }
            *buf++ = GMON_JSON_QUOTATION;
            pos = XSTRLEN(gmon_appmsg_log_records[jdx].name);
            XMEMCPY(buf, gmon_appmsg_log_records[jdx].name, pos);
            buf += pos;
            *buf++ = GMON_JSON_QUOTATION;
            *buf++ = GMON_JSON_COLON;
            XMEMSET(buf, GMON_JSON_WHITESPACE, gmon_appmsg_log_records[jdx].val_sz);
            gmon_appmsg_log_records[jdx].val_pos[idx] = buf;
            buf += gmon_appmsg_log_records[jdx].val_sz;
        } // end of for loop
        *buf++ = GMON_JSON_CURLY_BRACKET_RIGHT;
    } // end of for loop
    *buf++ = GMON_JSON_SQUARE_BRACKET_RIGHT;
    *buf++ = GMON_JSON_CURLY_BRACKET_RIGHT;
    XASSERT((unsigned short)(buf - outmsg->data) == outmsg->len);
} // end of staParseFixedPartsOutAppMsg

static void appMsgRecordReset(gMonEvtPool_t *epool, gmonSensorRecord_t *sr) {
    unsigned int record_sz = sr->num_refs * sizeof(gmonEvent_t *);
    for (unsigned char idx = 0; idx < sr->num_refs; idx++) {
        if (sr->events[idx]) {
            gMonStatus status = staFreeSensorEvent(epool, sr->events[idx]);
            XASSERT(status == GMON_RESP_OK);
        }
    }
    XMEMSET(sr->events, 0x00, record_sz);
    sr->inner_wr_ptr = 0;
}

#if 0
static void renderRecords2AppMsg(gmonSensorRecord_t *record, short idx) {
    uint32_t num_chr = 0;
    for (short jdx = 0; jdx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; jdx++) {
        XMEMSET(
            gmon_appmsg_log_records[jdx].val_pos[idx], GMON_JSON_WHITESPACE,
            gmon_appmsg_log_records[jdx].val_sz
        );
    }
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[0].val_pos[idx], record->soil_moist);
    XASSERT(num_chr <= gmon_appmsg_log_records[0].val_sz);
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[3].val_pos[idx], record->lightness);
    XASSERT(num_chr <= gmon_appmsg_log_records[3].val_sz);
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[4].val_pos[idx], record->curr_ticks);
    XASSERT(num_chr <= gmon_appmsg_log_records[4].val_sz);
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[5].val_pos[idx], record->curr_days);
    XASSERT(num_chr <= gmon_appmsg_log_records[5].val_sz);
    num_chr = staCvtFloatToStr(gmon_appmsg_log_records[1].val_pos[idx], record->air_cond.temporature, 0x2);
    XASSERT(num_chr <= gmon_appmsg_log_records[1].val_sz);
    num_chr = staCvtFloatToStr(gmon_appmsg_log_records[2].val_pos[idx], record->air_cond.humidity, 0x2);
    XASSERT(num_chr <= gmon_appmsg_log_records[2].val_sz);
}
#endif

gmonStr_t *staGetAppMsgOutflight(gardenMonitor_t *gmon) {
    stationSysEnterCritical();
    XASSERT(0); // TODO, finish implementation
#if 0
    for (uint8_t idx = 0; idx < GMON_APPMSG_NUM_RECORDS; idx++) {
        // latest record comes first
        renderRecords2AppMsg(&gmon->sensors.latest_records[idx], idx);
    }
#endif
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.soilmoist);
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.aircond);
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.light);
    stationSysExitCritical();
    return &gmon->rawmsg.outflight;
}

gMonStatus staAppMsgInit(gardenMonitor_t *gmon) {
    gMonRawMsg_t *rmsg = &gmon->rawmsg;
    // Calculate required buffer sizes for outflight and inflight messages
    rmsg->outflight.len = staAppMsgOutflightCalcRequiredBufSz();
    rmsg->inflight.len = staAppMsgInflightCalcRequiredBufSz();
    // TODO, shared buffer between inflight / outflight messages
    rmsg->outflight.data = XMALLOC(sizeof(unsigned char) * rmsg->outflight.len);
    rmsg->inflight.data = XMALLOC(sizeof(unsigned char) * rmsg->inflight.len);
    rmsg->jsn_decoder = XCALLOC(1, sizeof(jsmn_parser));
    rmsg->jsn_decoded_token = XCALLOC(GMON_NUM_JSON_TOKEN_DECODE, sizeof(jsmntok_t));

    // Initialize record fields for latest_logs
    gmonSensorRecord_t *record = &gmon->latest_logs.soilmoist;
    record->events = XCALLOC(GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP, sizeof(gmonEvent_t *));
    record->num_refs = GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP;
    record = &gmon->latest_logs.aircond;
    record->events = XCALLOC(GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP, sizeof(gmonEvent_t *));
    record->num_refs = GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP;
    record = &gmon->latest_logs.light;
    record->events = XCALLOC(GMON_CFG_NUM_LIGHT_SENSOR_RECORDS_KEEP, sizeof(gmonEvent_t *));
    record->num_refs = GMON_CFG_NUM_LIGHT_SENSOR_RECORDS_KEEP;

    uint8_t any_failed =
        (rmsg->outflight.data == NULL) || (rmsg->inflight.data == NULL) || (rmsg->jsn_decoder == NULL) ||
        (rmsg->jsn_decoded_token == NULL) || (gmon->latest_logs.soilmoist.events == NULL) ||
        (gmon->latest_logs.aircond.events == NULL) || (gmon->latest_logs.light.events == NULL);
    if (any_failed) // call de-init function below if init failed
        return GMON_RESP_ERRMEM;
    staParseFixedPartsOutAppMsg(&rmsg->outflight);
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.soilmoist);
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.aircond);
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.light);
    return GMON_RESP_OK;
} // end of staAppMsgInit

gMonStatus staAppMsgDeinit(gardenMonitor_t *gmon) {
#define FREE_IF_EXIST(v) \
    if (v) { \
        XMEMFREE(v); \
        v = NULL; \
    }
    FREE_IF_EXIST(gmon->rawmsg.outflight.data);
    FREE_IF_EXIST(gmon->rawmsg.inflight.data);
    FREE_IF_EXIST(gmon->rawmsg.jsn_decoder);
    FREE_IF_EXIST(gmon->rawmsg.jsn_decoded_token);
    FREE_IF_EXIST(gmon->latest_logs.aircond.events);
    FREE_IF_EXIST(gmon->latest_logs.soilmoist.events);
    FREE_IF_EXIST(gmon->latest_logs.light.events);
    return GMON_RESP_OK;
#undef FREE_IF_EXIST
}

static gmonEvent_t *staPopOldestEvent(gmonSensorRecord_t *sr) {
    // Get the event that will be replaced (this is the oldest one if the buffer is full).
    gmonEvent_t *discarded = sr->events[sr->inner_wr_ptr];
    // Explicitly clear the slot to 'make space'.
    sr->events[sr->inner_wr_ptr] = NULL;
    return discarded;
}

gmonEvent_t *staUpdateLastRecord(gmonSensorRecord_t *sr, gmonEvent_t *new_evt) {
    if (sr == NULL || sr->events == NULL || sr->num_refs == 0 || new_evt == NULL)
        return NULL;
    gmonEvent_t *discarded = NULL;
    stationSysEnterCritical();
    // pop off the oldest event reference from given sensor record, to make space
    // for inserting the new reference
    discarded = staPopOldestEvent(sr);
    // Insert the new event into the slot pointed to by inner_wr_ptr.
    // This slot was just 'popped' (its content retrieved and cleared).
    if (sr != NULL && sr->events != NULL && sr->num_refs > 0) {
        sr->events[sr->inner_wr_ptr] = new_evt;
        // Advance the wraparound counter to the next position for insertion.
        sr->inner_wr_ptr = (sr->inner_wr_ptr + 1) % sr->num_refs;
    }
    stationSysExitCritical();
    return discarded;
}

void stationSensorDataLogTaskFn(void *params) {
    gardenMonitor_t *gmon = (gardenMonitor_t *)params;
    while (1) {
        const uint32_t block_time = 5000;
        gmonEvent_t   *new_evt = NULL, *discarded_evt = NULL;
        gMonStatus     status = staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&new_evt, block_time);
        if (new_evt == NULL)
            continue;
        configASSERT(status == GMON_RESP_OK);
        configASSERT(new_evt->data != NULL);
        switch (new_evt->event_type) {
        case GMON_EVENT_SOIL_MOISTURE_UPDATED:
            discarded_evt = staUpdateLastRecord(&gmon->latest_logs.soilmoist, new_evt);
            break;
        case GMON_EVENT_LIGHTNESS_UPDATED:
            discarded_evt = staUpdateLastRecord(&gmon->latest_logs.light, new_evt);
            break;
        case GMON_EVENT_AIR_TEMP_UPDATED:
            discarded_evt = staUpdateLastRecord(&gmon->latest_logs.aircond, new_evt);
            break;
        default:
            break;
        }
        if (discarded_evt)
            staFreeSensorEvent(&gmon->sensors.event, discarded_evt);
    }
}
