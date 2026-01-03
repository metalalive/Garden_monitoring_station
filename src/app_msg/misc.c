#include "station_include.h"
#define JSMN_HEADER
#include "jsmn.h"

#define FREE_IF_EXIST(v) \
    if (v) { \
        XMEMFREE(v); \
        v = NULL; \
    }

gMonStatus staAppMsgInit(gardenMonitor_t *gmon) {
    gMonRawMsg_t *rmsg = &gmon->rawmsg;
    rmsg->outflight.data = NULL;
    rmsg->inflight.data = NULL;
    // Calculate required buffer sizes for outflight and inflight messages
    rmsg->inflight.len = staAppMsgInflightCalcRequiredBufSz();
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

    uint8_t any_failed = (rmsg->jsn_decoder == NULL) || (rmsg->jsn_decoded_token == NULL) ||
                         (gmon->latest_logs.soilmoist.events == NULL) ||
                         (gmon->latest_logs.aircond.events == NULL) ||
                         (gmon->latest_logs.light.events == NULL);
    if (any_failed) // call de-init function below if init failed
        return GMON_RESP_ERRMEM;
    // init internal write pointers of sensor records
    staAppMsgOutResetAllRecords(gmon);
    return GMON_RESP_OK;
} // end of staAppMsgInit

gMonStatus staAppMsgDeinit(gardenMonitor_t *gmon) {
    FREE_IF_EXIST(gmon->rawmsg.outflight.data);
    gmon->rawmsg.inflight.data = NULL;
    FREE_IF_EXIST(gmon->rawmsg.jsn_decoder);
    FREE_IF_EXIST(gmon->rawmsg.jsn_decoded_token);
    FREE_IF_EXIST(gmon->latest_logs.aircond.events);
    FREE_IF_EXIST(gmon->latest_logs.soilmoist.events);
    FREE_IF_EXIST(gmon->latest_logs.light.events);
    return GMON_RESP_OK;
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
