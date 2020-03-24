#include "station_include.h"
#include "jsmn.h"

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
// topic : garden/control
// message :
// {
//    "interval":  {"sensorread": 1200000, "netconn":3600000 },
//    "threshold": {"soilmoist": 1234, "airtemp": 35.5, "light": 4321, "daylength":7200000 }
// }


#define  GMON_JSON_CURLY_BRACKET_LEFT    '{'
#define  GMON_JSON_CURLY_BRACKET_RIGHT   '}'
#define  GMON_JSON_SQUARE_BRACKET_LEFT   '['
#define  GMON_JSON_SQUARE_BRACKET_RIGHT  ']'
#define  GMON_JSON_QUOTATION             '"'
#define  GMON_JSON_COLON                 ':'
#define  GMON_JSON_COMMA                 ','
#define  GMON_JSON_WHITESPACE            ' '

#define  GMON_APPMSG_DATA_NAME_RECORDS    "records"
#define  GMON_APPMSG_DATA_NAME_SOILMOIST  "soilmoist"
#define  GMON_APPMSG_DATA_NAME_AIRTEMP    "airtemp"
#define  GMON_APPMSG_DATA_NAME_AIRHUMID   "airhumid"
#define  GMON_APPMSG_DATA_NAME_LIGHT      "light"
#define  GMON_APPMSG_DATA_NAME_TICKS      "ticks"
#define  GMON_APPMSG_DATA_NAME_DAYS       "days"
#define  GMON_APPMSG_DATA_NAME_INTERVAL   "interval"
#define  GMON_APPMSG_DATA_NAME_SENSORREAD "sensorread"
#define  GMON_APPMSG_DATA_NAME_NETCONN    "netconn"
#define  GMON_APPMSG_DATA_NAME_THRESHOLD  "threshold"
#define  GMON_APPMSG_DATA_NAME_DAYLENGTH  "daylength"

#define  GMON_APPMSG_DATA_SZ_SOILMOIST     4
#define  GMON_APPMSG_DATA_SZ_AIRTEMP       6 // 3 bytes for integral part, 1-byte decimal seperator, 2 bytes for fractional part
#define  GMON_APPMSG_DATA_SZ_AIRHUMID      6 // 3 bytes for integral part, 1-byte decimal seperator, 2 bytes for fractional part
#define  GMON_APPMSG_DATA_SZ_LIGHT         4
#define  GMON_APPMSG_DATA_SZ_TICKS         8
#define  GMON_APPMSG_DATA_SZ_DAYS          6
#define  GMON_APPMSG_DATA_SZ_SENSORREAD    8
#define  GMON_APPMSG_DATA_SZ_NETCONN       8
#define  GMON_APPMSG_DATA_SZ_DAYLENGTH     8

#define  GMON_APPMSG_NUM_RECORDS            GMON_CFG_NUM_SENSOR_RECORDS_KEEP
#define  GMON_APPMSG_NUM_ITEMS_PER_RECORD   6

#define  GMON_APPMSG_NUM_ITEMS_PER_THRESHOLD_NODE  4

typedef struct {
    const char      *name;
    unsigned  short  val_sz;
    unsigned  char  *val_pos[GMON_APPMSG_NUM_RECORDS];
} gmonMsgItem_t;

extern stationSysMsgbox_t  sensor_to_netconn_buf;

static gmonStr_t  gmon_appmsg_outflight;
static gmonStr_t  gmon_appmsg_inflight;
static short      gmon_appmsg_outflight_num_avail_records;

static gmonMsgItem_t gmon_appmsg_log_records[GMON_APPMSG_NUM_ITEMS_PER_RECORD] = {
    {(const char *)&(GMON_APPMSG_DATA_NAME_SOILMOIST),(unsigned short)GMON_APPMSG_DATA_SZ_SOILMOIST, },
    {(const char *)&(GMON_APPMSG_DATA_NAME_AIRTEMP  ),(unsigned short)GMON_APPMSG_DATA_SZ_AIRTEMP  , },
    {(const char *)&(GMON_APPMSG_DATA_NAME_AIRHUMID ),(unsigned short)GMON_APPMSG_DATA_SZ_AIRHUMID , },
    {(const char *)&(GMON_APPMSG_DATA_NAME_LIGHT    ),(unsigned short)GMON_APPMSG_DATA_SZ_LIGHT    , },
    {(const char *)&(GMON_APPMSG_DATA_NAME_TICKS    ),(unsigned short)GMON_APPMSG_DATA_SZ_TICKS    , },
    {(const char *)&(GMON_APPMSG_DATA_NAME_DAYS     ),(unsigned short)GMON_APPMSG_DATA_SZ_DAYS     , },
}; // gmon_appmsg_log_records

static gmonMsgItem_t gmon_appmsg_ctrl_threshold[GMON_APPMSG_NUM_ITEMS_PER_THRESHOLD_NODE] = {
    {(const char *)&(GMON_APPMSG_DATA_NAME_SOILMOIST),(unsigned short)GMON_APPMSG_DATA_SZ_SOILMOIST, },
    {(const char *)&(GMON_APPMSG_DATA_NAME_AIRTEMP  ),(unsigned short)GMON_APPMSG_DATA_SZ_AIRTEMP  , },
    {(const char *)&(GMON_APPMSG_DATA_NAME_LIGHT    ),(unsigned short)GMON_APPMSG_DATA_SZ_LIGHT    , },
    {(const char *)&(GMON_APPMSG_DATA_NAME_DAYLENGTH),(unsigned short)GMON_APPMSG_DATA_SZ_DAYLENGTH, },
}; // gmon_appmsg_ctrl_threshold

#define  GMON_NUM_JSON_TOKEN_DECODE  18
jsmntok_t    gmon_json_decode_token[GMON_NUM_JSON_TOKEN_DECODE];
jsmn_parser  gmon_json_decoder;


static void staAppMsgCalcRequiredBufSz(unsigned short *outlen, unsigned short *inlen)
{
    unsigned short  len = 0;
    unsigned short  idx = 0;
    *outlen  = 5 + XSTRLEN(GMON_APPMSG_DATA_NAME_RECORDS) + 2;
    len  = 2 + 3 * GMON_APPMSG_NUM_ITEMS_PER_RECORD + (GMON_APPMSG_NUM_ITEMS_PER_RECORD - 1);
    for(idx = 0; idx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; idx++) {
        len += XSTRLEN(gmon_appmsg_log_records[idx].name) + gmon_appmsg_log_records[idx].val_sz;
    }
    len = len * GMON_APPMSG_NUM_RECORDS + (GMON_APPMSG_NUM_RECORDS - 1);
    *outlen += len;
    *inlen   = 2 + 1 + 2 * 5;
    *inlen  += XSTRLEN(GMON_APPMSG_DATA_NAME_INTERVAL) + XSTRLEN(GMON_APPMSG_DATA_NAME_THRESHOLD);
    *inlen  += 2 * 3 + 1 + XSTRLEN(GMON_APPMSG_DATA_NAME_SENSORREAD) + XSTRLEN(GMON_APPMSG_DATA_NAME_NETCONN);
    *inlen  += GMON_APPMSG_DATA_SZ_SENSORREAD + GMON_APPMSG_DATA_SZ_NETCONN;
    len = 2 + 3 * GMON_APPMSG_NUM_ITEMS_PER_THRESHOLD_NODE + (GMON_APPMSG_NUM_ITEMS_PER_THRESHOLD_NODE - 1);
    for(idx = 0; idx < GMON_APPMSG_NUM_ITEMS_PER_THRESHOLD_NODE; idx++) {
        len += XSTRLEN(gmon_appmsg_ctrl_threshold[idx].name) + gmon_appmsg_ctrl_threshold[idx].val_sz;
    }
    *inlen  += len;
} // end of staAppMsgCalcRequiredBufSz


static  void staParseFixedPartsOutAppMsg(gmonStr_t *outmsg)
{
    unsigned char  *buf = NULL;
    unsigned short  pos = 0;
    unsigned short  idx = 0, jdx = 0;
    buf = outmsg->data;

    *buf++ = GMON_JSON_CURLY_BRACKET_LEFT;
    *buf++ = GMON_JSON_QUOTATION;
    pos = XSTRLEN(GMON_APPMSG_DATA_NAME_RECORDS);
    XMEMCPY(buf, GMON_APPMSG_DATA_NAME_RECORDS, pos);
    buf += pos;
    *buf++ = GMON_JSON_QUOTATION;
    *buf++ = GMON_JSON_COLON;
    *buf++ = GMON_JSON_SQUARE_BRACKET_LEFT;
    for(idx = 0; idx < GMON_APPMSG_NUM_RECORDS; idx++) {
        if(idx > 0) { *buf++ = GMON_JSON_COMMA; }
        *buf++ = GMON_JSON_CURLY_BRACKET_LEFT;
        for(jdx = 0; jdx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; jdx++) {
            if(jdx > 0) { *buf++ = GMON_JSON_COMMA; }
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


static void  staAppMsgOutResetAllRecords(void) {
   gmon_appmsg_outflight_num_avail_records = GMON_APPMSG_NUM_RECORDS;
} // end of staAppMsgOutResetAllRecords



gMonStatus  staAppMsgInit(void)
{
    unsigned char  *buf = NULL;
    gMonStatus status = GMON_RESP_OK;
    // parse fixed parts of outflight application messages on initialization
    staAppMsgCalcRequiredBufSz(&gmon_appmsg_outflight.len, &gmon_appmsg_inflight.len);
    buf = XMALLOC(sizeof(unsigned char) * (gmon_appmsg_outflight.len + gmon_appmsg_inflight.len));
    gmon_appmsg_outflight.data = &buf[0];
    gmon_appmsg_inflight.data  = &buf[gmon_appmsg_outflight.len];
    staParseFixedPartsOutAppMsg(&gmon_appmsg_outflight);
    staAppMsgOutResetAllRecords();
    return status;
} // end of staAppMsgInit


gMonStatus  staAppMsgDeinit(void)
{
    XMEMFREE(gmon_appmsg_outflight.data);
    gmon_appmsg_outflight.data = NULL;
    gmon_appmsg_inflight.data  = NULL;
    return GMON_RESP_OK;
} // end of staAppMsgDeinit


gMonStatus  staAddRecordToAppMsg(gmonSensorRecord_t  *new_record)
{
    if(new_record == NULL) { return GMON_RESP_ERRARGS; }
    gMonStatus status = GMON_RESP_OK;
    uint32_t   num_chr = 0;
    short      idx = 0, jdx = 0;
    if(gmon_appmsg_outflight_num_avail_records > 0) {
        idx = GMON_APPMSG_NUM_RECORDS - gmon_appmsg_outflight_num_avail_records;
        gmon_appmsg_outflight_num_avail_records--;
        for(jdx = 0; jdx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; jdx++) {
            XMEMSET(gmon_appmsg_log_records[jdx].val_pos[idx], GMON_JSON_WHITESPACE, gmon_appmsg_log_records[jdx].val_sz);
        }
        num_chr = staCvtUNumToStr(gmon_appmsg_log_records[0].val_pos[idx], new_record->soil_moist);
        XASSERT(num_chr <= gmon_appmsg_log_records[0].val_sz);
        num_chr = staCvtUNumToStr(gmon_appmsg_log_records[3].val_pos[idx], new_record->lightness );
        XASSERT(num_chr <= gmon_appmsg_log_records[3].val_sz);
        num_chr = staCvtUNumToStr(gmon_appmsg_log_records[4].val_pos[idx], new_record->curr_ticks);
        XASSERT(num_chr <= gmon_appmsg_log_records[4].val_sz);
        num_chr = staCvtUNumToStr(gmon_appmsg_log_records[5].val_pos[idx], new_record->curr_days );
        XASSERT(num_chr <= gmon_appmsg_log_records[5].val_sz);
        num_chr = staCvtFloatToStr(gmon_appmsg_log_records[1].val_pos[idx], new_record->air_temp, 0x2);
        XASSERT(num_chr <= gmon_appmsg_log_records[1].val_sz);
        num_chr = staCvtFloatToStr(gmon_appmsg_log_records[2].val_pos[idx], new_record->air_humid, 0x2);
        XASSERT(num_chr <= gmon_appmsg_log_records[2].val_sz);
    } else {
        status = GMON_RESP_SKIP;
    }
    return status;
} // end of staAddRecordToAppMsg


gMonStatus staRefreshAppMsgOutflight(void)
{
    const uint32_t  block_time = 0;
    gmonSensorRecord_t  *new_record = NULL;
    gMonStatus status = GMON_RESP_OK;
    uint8_t  num_records_read = 0;
    do {
        status = staSysMsgBoxGet(sensor_to_netconn_buf, (void **)&new_record, block_time);
        if(new_record != NULL) {
            // contruct JSON-data with sensor records, append to application bytes
            staAddRecordToAppMsg(new_record);
            staFreeSensorRecord(new_record);
            new_record = NULL;
            num_records_read++;
        }
    } while (status == GMON_RESP_OK);
    // TODO: fill zero to all other items that are not updated with new value
    staAppMsgOutResetAllRecords();
    status = (num_records_read == 0)? GMON_RESP_SKIP: GMON_RESP_OK;
    return status;
} // end of staRefreshAppMsgOutflight


static  gMonStatus  staDecodeMsgCvtStrToInt(jsmntok_t *tokn, int *out)
{
    unsigned char  *user_var_name = NULL;
    int    user_var_len  = 0;
    gMonStatus status = GMON_RESP_OK;
    user_var_len  =  tokn->end - tokn->start;
    user_var_name = &gmon_appmsg_inflight.data[tokn->start];
    status = staChkIntFromStr(user_var_name, user_var_len);
    if(status == GMON_RESP_OK) {
        *out = staCvtIntFromStr(user_var_name, user_var_len);
    }
    return status;
} // end of staDecodeMsgCvtStrToInt


// decode JSON-based message sent by remote backend server, the message may contain modification request
// e.g. threshold to trigger each output device, time interval to send logs to remote backend service ...
// the function below checks each node of the JSON message, update everything specified by remote user after
// successful decoding process.
gMonStatus  staDecodeAppMsgInflight(gardenMonitor_t *gmon)
{
    unsigned char  *user_var_name = NULL;
    int    user_var_len  = 0;
    int    parsed_int   = 0;
    short  r  = 0;
    short  idx = 0;
    gMonStatus status = GMON_RESP_OK;

    jsmn_init(&gmon_json_decoder);
    r = (short) jsmn_parse(&gmon_json_decoder, gmon_appmsg_inflight.data,  gmon_appmsg_inflight.len,
                   &gmon_json_decode_token[0], GMON_NUM_JSON_TOKEN_DECODE);
    if(r > 0) {
        for(idx = 0; (idx < r) && (status == GMON_RESP_OK); idx++) {
            if(gmon_json_decode_token[idx].type == JSMN_STRING) {
                user_var_len  =  gmon_json_decode_token[idx].end - gmon_json_decode_token[idx].start;
                user_var_name = &gmon_appmsg_inflight.data[gmon_json_decode_token[idx].start];
                if(XSTRNCMP(GMON_APPMSG_DATA_NAME_SENSORREAD, user_var_name, user_var_len) == 0) {
                    status = staDecodeMsgCvtStrToInt(&gmon_json_decode_token[idx + 1], &parsed_int);
                    if(status == GMON_RESP_OK) {
                        gmon->user_ctrl.status.interval.sensorread  = staSetDefaultSensorReadInterval(gmon, (unsigned int)parsed_int);
                    }
                } else if(XSTRNCMP(GMON_APPMSG_DATA_NAME_NETCONN  , user_var_name, user_var_len) == 0) {
                    status = staDecodeMsgCvtStrToInt(&gmon_json_decode_token[idx + 1], &parsed_int);
                    if(status == GMON_RESP_OK) {
                        gmon->user_ctrl.status.interval.netconn = staSetNetConnTaskInterval(gmon, (unsigned int)parsed_int);
                    }
                } else if(XSTRNCMP(GMON_APPMSG_DATA_NAME_SOILMOIST, user_var_name, user_var_len) == 0) {
                    status = staDecodeMsgCvtStrToInt(&gmon_json_decode_token[idx + 1], &parsed_int);
                    if(status == GMON_RESP_OK) {
                        gmon->user_ctrl.status.threshold.soil_moist = staSetTrigThresholdPump(&gmon->outdev.pump , (unsigned int)parsed_int);
                    }
                } else if(XSTRNCMP(GMON_APPMSG_DATA_NAME_AIRTEMP , user_var_name, user_var_len) == 0) {
                    status = staDecodeMsgCvtStrToInt(&gmon_json_decode_token[idx + 1], &parsed_int);
                    if(status == GMON_RESP_OK) {
                        gmon->user_ctrl.status.threshold.air_temp = staSetTrigThresholdFan(&gmon->outdev.fan, (unsigned int)parsed_int);
                    }
                } else if(XSTRNCMP(GMON_APPMSG_DATA_NAME_LIGHT    , user_var_name, user_var_len) == 0) {
                    status = staDecodeMsgCvtStrToInt(&gmon_json_decode_token[idx + 1], &parsed_int);
                    if(status == GMON_RESP_OK) {
                        gmon->user_ctrl.status.threshold.lightness = staSetTrigThresholdBulb(&gmon->outdev.bulb, (unsigned int)parsed_int);
                    }
                } else if(XSTRNCMP(GMON_APPMSG_DATA_NAME_DAYLENGTH, user_var_name, user_var_len) == 0) {
                    status = staDecodeMsgCvtStrToInt(&gmon_json_decode_token[idx + 1], &parsed_int);
                    if(status == GMON_RESP_OK) {
                        gmon->user_ctrl.status.threshold.daylength = staSetRequiredDaylenTicks((unsigned int)parsed_int);
                    }
                }
            } // if token is json string
        } // end of for loop
        if(status == GMON_RESP_OK) {
            gmon->user_ctrl.last_update.ticks = stationGetTicksPerDay();
            gmon->user_ctrl.last_update.days  = stationGetDays();
        }
    } else {
        status = GMON_RESP_ERR_MSG_DECODE ;
    }
    // clean up all received bytes after decoding process
    XMEMSET(gmon_appmsg_inflight.data, GMON_JSON_WHITESPACE, sizeof(char) * gmon_appmsg_inflight.len);
    return status;
} // end of staDecodeAppMsgInflight


gmonStr_t* staGetAppMsgOutflight(void)
{
    return &gmon_appmsg_outflight;
} // end of staGetAppMsgOutflight

gmonStr_t*  staGetAppMsgInflight(void)
{
    return &gmon_appmsg_inflight;
} // end of staGetAppMsgInflight

