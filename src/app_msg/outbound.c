#include "station_include.h"
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

#define GMON_APPMSG_NUM_RECORDS GMON_CFG_NUM_SENSOR_RECORDS_KEEP

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

unsigned short staAppMsgOutflightCalcRequiredBufSz(void) {
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

void staParseFixedPartsOutAppMsg(gmonStr_t *outmsg) {
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

void staAppMsgOutResetAllRecords(gardenMonitor_t *gmon) {
    size_t record_sz = GMON_APPMSG_NUM_RECORDS * sizeof(gmonSensorRecord_t);
    XMEMSET(&gmon->sensors.latest_records[0], 0x00, record_sz);
}

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

gmonStr_t *staGetAppMsgOutflight(gardenMonitor_t *gmon) {
    stationSysEnterCritical();
    for (uint8_t idx = 0; idx < GMON_APPMSG_NUM_RECORDS; idx++) {
        // latest record comes first
        renderRecords2AppMsg(&gmon->sensors.latest_records[idx], idx);
    }
    staAppMsgOutResetAllRecords(gmon);
    stationSysExitCritical();
    return &gmon->rawmsg.outflight;
}

static gmonSensorRecord_t staShiftRecords(gmonSensorRecord_t *records) {
    gmonSensorRecord_t discarded = records[GMON_APPMSG_NUM_RECORDS - 1];
    // - shift all existing records to make space for a new one at `record[0]`.
    // - this effectively discards the oldest record (at `record[GMON_APPMSG_NUM_RECORDS - 1]`).
    for (int i = GMON_APPMSG_NUM_RECORDS - 1; i > 0; i--) {
        records[i] = records[i - 1];
    }
    // Clear the new `records[0]` to prepare it for fresh aggregation of incoming events.
    XMEMSET(&records[0], 0, sizeof(gmonSensorRecord_t));
    return discarded;
}

gmonSensorRecord_t staUpdateLastRecord(gmonSensorRecord_t *records, gmonEvent_t *evt) {
    gmonSensorRecord_t discarded = {0};
    // `records[0]` represents the newest/currently aggregating record.
    // `records[GMON_APPMSG_NUM_RECORDS - 1]` represents the oldest record.
    stationSysEnterCritical();
    // Check if the first aggregating record has collected any sensor data type.
    switch (evt->event_type) {
    case GMON_EVENT_SOIL_MOISTURE_UPDATED:
        if (records[0].flgs.soil_val_written) {
            discarded = staShiftRecords(records);
        }
        // Update the current aggregating record (`record[0]`) with data
        // from the new event.
        records[0].curr_ticks = evt->curr_ticks;
        records[0].curr_days = evt->curr_days;
        records[0].soil_moist = ((unsigned int *)evt->data)[0]; // TODO
        records[0].flgs.soil_val_written = 1;
        break;
    case GMON_EVENT_LIGHTNESS_UPDATED:
        if (records[0].flgs.light_val_written) {
            discarded = staShiftRecords(records);
        }
        records[0].curr_ticks = evt->curr_ticks;
        records[0].curr_days = evt->curr_days;
        records[0].lightness = ((unsigned int *)evt->data)[0]; // TODO
        records[0].flgs.light_val_written = 1;
        break;
    case GMON_EVENT_AIR_TEMP_UPDATED:
        if (records[0].flgs.air_val_written) {
            discarded = staShiftRecords(records);
        }
        records[0].curr_ticks = evt->curr_ticks;
        records[0].curr_days = evt->curr_days;
        records[0].air_cond = ((gmonAirCond_t *)evt->data)[0]; // TODO
        records[0].flgs.air_val_written = 1;
        break;
    default:
        break;
    }
    stationSysExitCritical();
    return discarded;
} // end of staUpdateLastRecord

void stationSensorDataAggregatorTaskFn(void *params) {
    gardenMonitor_t *gmon = (gardenMonitor_t *)params;
    while (1) {
        const uint32_t block_time = 5000;
        gmonEvent_t   *new_evt = NULL;
        gMonStatus     status = staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&new_evt, block_time);
        if (new_evt != NULL) { // This must be inside the loop
            configASSERT(status == GMON_RESP_OK);
            staUpdateLastRecord(gmon->sensors.latest_records, new_evt);
            staFreeSensorEvent(&gmon->sensors.event, new_evt);
            new_evt = NULL;
        }
    }
}
