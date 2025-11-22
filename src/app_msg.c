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
//     "interval":  {
//         "sensor": {"soil_moist": 2100, "air_temp": 7100, "light": 11000},
//         "netconn": 360000
//     },
//     "threshold": {"soilmoist": 1234, "airtemp": 35.5, "light": 4321, "daylength":7200000 }
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
#define  GMON_APPMSG_DATA_NAME_SENSOR_KEY "sensor"
#define  GMON_APPMSG_DATA_NAME_INTERVAL_SOIL_MOIST_KEY "soil_moist"
#define  GMON_APPMSG_DATA_NAME_INTERVAL_AIR_TEMP_KEY   "air_temp"
#define  GMON_APPMSG_DATA_NAME_INTERVAL_LIGHT_KEY      "light"

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

typedef struct {
    GMON_SENSORDATA_COMMON_FIELDS;
    unsigned int  curr_ticks;
    unsigned int  curr_days ;
    struct {
        uint8_t air_val_written:1;
        uint8_t soil_val_written:1;
        uint8_t light_val_written:1;
    } flgs;
} gmonSensorRecord_t;

static gmonSensorRecord_t  gmon_latest_sensor_data[GMON_APPMSG_NUM_RECORDS] = {0};

static short      gmon_appmsg_outflight_num_avail_records;

static gmonMsgItem_t gmon_appmsg_log_records[GMON_APPMSG_NUM_ITEMS_PER_RECORD] = {
    {GMON_APPMSG_DATA_NAME_SOILMOIST,(unsigned short)GMON_APPMSG_DATA_SZ_SOILMOIST, {0}},
    {GMON_APPMSG_DATA_NAME_AIRTEMP  ,(unsigned short)GMON_APPMSG_DATA_SZ_AIRTEMP  , {0}},
    {GMON_APPMSG_DATA_NAME_AIRHUMID ,(unsigned short)GMON_APPMSG_DATA_SZ_AIRHUMID , {0}},
    {GMON_APPMSG_DATA_NAME_LIGHT    ,(unsigned short)GMON_APPMSG_DATA_SZ_LIGHT    , {0}},
    {GMON_APPMSG_DATA_NAME_TICKS    ,(unsigned short)GMON_APPMSG_DATA_SZ_TICKS    , {0}},
    {GMON_APPMSG_DATA_NAME_DAYS     ,(unsigned short)GMON_APPMSG_DATA_SZ_DAYS     , {0}},
};

static gmonMsgItem_t gmon_appmsg_ctrl_threshold[GMON_APPMSG_NUM_ITEMS_PER_THRESHOLD_NODE] = {
    {GMON_APPMSG_DATA_NAME_SOILMOIST,(unsigned short)GMON_APPMSG_DATA_SZ_SOILMOIST, {0}},
    {GMON_APPMSG_DATA_NAME_AIRTEMP  ,(unsigned short)GMON_APPMSG_DATA_SZ_AIRTEMP  , {0}},
    {GMON_APPMSG_DATA_NAME_LIGHT    ,(unsigned short)GMON_APPMSG_DATA_SZ_LIGHT    , {0}},
    {GMON_APPMSG_DATA_NAME_DAYLENGTH,(unsigned short)GMON_APPMSG_DATA_SZ_DAYLENGTH, {0}},
};

#define  GMON_NUM_JSON_TOKEN_DECODE  32

static unsigned short staAppMsgOutflightCalcRequiredBufSz(void) {
    unsigned short outlen = 0, len = 0, idx = 0;
    outlen  = 5 + XSTRLEN(GMON_APPMSG_DATA_NAME_RECORDS) + 2;
    len  = 2 + 3 * GMON_APPMSG_NUM_ITEMS_PER_RECORD + (GMON_APPMSG_NUM_ITEMS_PER_RECORD - 1);
    for(idx = 0; idx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; idx++) {
        len += XSTRLEN(gmon_appmsg_log_records[idx].name) + gmon_appmsg_log_records[idx].val_sz;
    }
    len = len * GMON_APPMSG_NUM_RECORDS + (GMON_APPMSG_NUM_RECORDS - 1);
    outlen += len;
    return outlen;
}

static unsigned short staAppMsgInflightCalcRequiredBufSz(void) {
    // Using a sufficiently large fixed buffer for incoming control JSON messages.
    // 256 bytes should be ample to accommodate various configuration updates.
    return 256;
}

static  void staParseFixedPartsOutAppMsg(gmonStr_t *outmsg) {
    unsigned short  pos = 0,  idx = 0, jdx = 0;
    unsigned char  *buf = outmsg->data;
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
    XMEMSET(&gmon_latest_sensor_data[0], 0x00, GMON_APPMSG_NUM_RECORDS * sizeof(gmonSensorRecord_t));
}

gMonStatus  staAppMsgInit(gardenMonitor_t *gmon) {
    gMonRawMsg_t *rmsg = &gmon->rawmsg;
    // Calculate required buffer sizes for outflight and inflight messages
    rmsg->outflight.len = staAppMsgOutflightCalcRequiredBufSz();
    rmsg->inflight.len = staAppMsgInflightCalcRequiredBufSz();
    rmsg->outflight.data = XMALLOC(sizeof(unsigned char) * rmsg->outflight.len);
    rmsg->inflight.data = XMALLOC(sizeof(unsigned char) * rmsg->inflight.len);
    rmsg->jsn_decoder = XCALLOC(1, sizeof(jsmn_parser));
    rmsg->jsn_decoded_token = XCALLOC(GMON_NUM_JSON_TOKEN_DECODE, sizeof(jsmntok_t));
    uint8_t any_failed = (rmsg->outflight.data == NULL) ||
        (rmsg->inflight.data == NULL) || (rmsg->jsn_decoder == NULL)
        || (rmsg->jsn_decoded_token == NULL);
    if (any_failed) { // call de-init function below if init failed
        return GMON_RESP_ERRMEM;
    } else {
        staParseFixedPartsOutAppMsg(&rmsg->outflight);
        staAppMsgOutResetAllRecords();
        return GMON_RESP_OK;
    }
}

gMonStatus  staAppMsgDeinit(gardenMonitor_t *gmon) {
    if (gmon->rawmsg.outflight.data) {
        XMEMFREE(gmon->rawmsg.outflight.data);
        gmon->rawmsg.outflight.data = NULL;
    }
    if (gmon->rawmsg.inflight.data) {
        XMEMFREE(gmon->rawmsg.inflight.data);
        gmon->rawmsg.inflight.data = NULL;
    }
    if (gmon->rawmsg.jsn_decoder) {
        XMEMFREE(gmon->rawmsg.jsn_decoder);
        gmon->rawmsg.jsn_decoder = NULL;
    }
    if (gmon->rawmsg.jsn_decoded_token) {
        XMEMFREE(gmon->rawmsg.jsn_decoded_token);
        gmon->rawmsg.jsn_decoded_token = NULL;
    }
    return GMON_RESP_OK;
}

static void  renderRecords2AppMsg(gmonSensorRecord_t *record) {
    uint32_t   num_chr = 0;
    short      idx = 0, jdx = 0;
    XASSERT(gmon_appmsg_outflight_num_avail_records > 0);
    idx = GMON_APPMSG_NUM_RECORDS - gmon_appmsg_outflight_num_avail_records;
    gmon_appmsg_outflight_num_avail_records--;
    for(jdx = 0; jdx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; jdx++) {
        XMEMSET(gmon_appmsg_log_records[jdx].val_pos[idx], GMON_JSON_WHITESPACE, gmon_appmsg_log_records[jdx].val_sz);
    }
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[0].val_pos[idx], record->soil_moist);
    XASSERT(num_chr <= gmon_appmsg_log_records[0].val_sz);
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[3].val_pos[idx], record->lightness );
    XASSERT(num_chr <= gmon_appmsg_log_records[3].val_sz);
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[4].val_pos[idx], record->curr_ticks);
    XASSERT(num_chr <= gmon_appmsg_log_records[4].val_sz);
    num_chr = staCvtUNumToStr(gmon_appmsg_log_records[5].val_pos[idx], record->curr_days );
    XASSERT(num_chr <= gmon_appmsg_log_records[5].val_sz);
    num_chr = staCvtFloatToStr(gmon_appmsg_log_records[1].val_pos[idx], record->air_temp, 0x2);
    XASSERT(num_chr <= gmon_appmsg_log_records[1].val_sz);
    num_chr = staCvtFloatToStr(gmon_appmsg_log_records[2].val_pos[idx], record->air_humid, 0x2);
    XASSERT(num_chr <= gmon_appmsg_log_records[2].val_sz);
} // end of renderRecords2AppMsg


gMonStatus staRefreshAppMsgOutflight(gardenMonitor_t *gmon) {
    (void) gmon;
    for (uint8_t idx = 0; idx < GMON_APPMSG_NUM_RECORDS; idx++) {
        renderRecords2AppMsg(&gmon_latest_sensor_data[idx]);
    }
    staAppMsgOutResetAllRecords();
    return GMON_RESP_OK;
}


static  gMonStatus  staDecodeMsgCvtStrToInt(const unsigned char *json_data, jsmntok_t *tokn, int *out) {
    unsigned char  *user_var_name = NULL;
    int    user_var_len  = 0;
    gMonStatus status = GMON_RESP_OK;
    user_var_len  =  tokn->end - tokn->start;
    user_var_name = (unsigned char *)&json_data[tokn->start];
    status = staChkIntFromStr(user_var_name, user_var_len);
    if(status == GMON_RESP_OK) {
        *out = staCvtIntFromStr(user_var_name, user_var_len);
    }
    return status;
}

static gMonStatus staDecodeSensorIntervals(gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx, int *tokens_consumed_out) {
    gMonStatus status = GMON_RESP_OK;
    int parsed_int = 0;
    jsmntok_t *sensor_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1; // For the sensor object itself

    if (sensor_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int children_start_idx = start_object_token_idx + 1;
    int children_limit_idx = children_start_idx + sensor_obj_token->size * 2;

    for (int child_idx = children_start_idx; child_idx < children_limit_idx && (status == GMON_RESP_OK); child_idx += 2) { // Iterate key-value pairs
        jsmntok_t *child_key_token = &tokens[child_idx];
        jsmntok_t *child_value_token = &tokens[child_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL_SOIL_MOIST_KEY, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->sensors.soil_moist.read_interval_ms = (unsigned int)parsed_int;
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL_AIR_TEMP_KEY, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->sensors.air_temp.read_interval_ms = (unsigned int)parsed_int;
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL_LIGHT_KEY, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->sensors.light.read_interval_ms = (unsigned int)parsed_int;
                }
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE; // Expected string key
        }
        *tokens_consumed_out += 2; // for key and value
    }
    return status;
}

static gMonStatus staDecodeIntervalObject(gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx, int *tokens_consumed_out) {
    gMonStatus status = GMON_RESP_OK;
    int parsed_int = 0;
    jsmntok_t *interval_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1; // For the interval object token itself

    if (interval_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int current_child_token_idx = start_object_token_idx + 1; // Start of children tokens

    // Iterate through each key-value pair within the interval object
    for (int i = 0; i < interval_obj_token->size && (status == GMON_RESP_OK); i++) {
        jsmntok_t *child_key_token = &tokens[current_child_token_idx];
        jsmntok_t *child_value_token = &tokens[current_child_token_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_SENSOR_KEY, child_key_name, child_key_len) == 0) {
                int sensor_tokens_consumed = 0;
                status = staDecodeSensorIntervals(gmon, json_data, tokens, current_child_token_idx + 1, &sensor_tokens_consumed);
                *tokens_consumed_out += (1 /*key token*/ + sensor_tokens_consumed);
                current_child_token_idx += (1 /*key token*/ + sensor_tokens_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_NETCONN, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->user_ctrl.status.interval.netconn = staSetNetConnTaskInterval(gmon, (unsigned int)parsed_int);
                }
                *tokens_consumed_out += 2; // Key + Primitive Value
                current_child_token_idx += 2; // Move past key and its value
            } else {
                // Unknown key in interval object, skip its value
                int skip_tokens_for_value = 1; // Default for primitive value
                if (child_value_token->type == JSMN_OBJECT || child_value_token->type == JSMN_ARRAY) {
                    // If the value is an object or array, we need to skip the value token itself AND all its children
                    skip_tokens_for_value += child_value_token->size * (child_value_token->type == JSMN_OBJECT ? 2 : 1);
                }
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                current_child_token_idx += (1 /*key token*/ + skip_tokens_for_value);
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE; // Expected string key
            current_child_token_idx++; // Attempt to move past the invalid token.
            *tokens_consumed_out += 1; // Count the malformed token.
        }
    }
    return status;
}

static gMonStatus staDecodeThresholdObject(gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx, int *tokens_consumed_out) {
    gMonStatus status = GMON_RESP_OK;
    int parsed_int = 0;
    jsmntok_t *threshold_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1; // For the threshold object itself

    if (threshold_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int children_start_idx = start_object_token_idx + 1;
    int children_limit_idx = children_start_idx + threshold_obj_token->size * 2;

    for (int child_idx = children_start_idx; child_idx < children_limit_idx && (status == GMON_RESP_OK); child_idx += 2) { // Iterate key-value pairs
        jsmntok_t *child_key_token = &tokens[child_idx];
        jsmntok_t *child_value_token = &tokens[child_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_SOILMOIST, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->user_ctrl.status.threshold.soil_moist = staSetTrigThresholdPump(&gmon->outdev.pump , (unsigned int)parsed_int);
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_AIRTEMP , child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->user_ctrl.status.threshold.air_temp = staSetTrigThresholdFan(&gmon->outdev.fan, (unsigned int)parsed_int);
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_LIGHT    , child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->user_ctrl.status.threshold.lightness = staSetTrigThresholdBulb(&gmon->outdev.bulb, (unsigned int)parsed_int);
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_DAYLENGTH, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->user_ctrl.status.threshold.daylength = staSetRequiredDaylenTicks(gmon, (unsigned int)parsed_int);
                }
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE; // Expected string key
        }
        *tokens_consumed_out += 2; // For key and value
    }
    return status;
}


// decode JSON-based message sent by remote backend server, the message may contain modification request
// e.g. threshold to trigger each output device, time interval to send logs to remote backend service ...
// the function below checks each node of the JSON message, update everything specified by remote user after
// successful decoding process. This is the core logic moved to a separate function for testability.
static gMonStatus staDecodeAppMsgInflightCore(
    gardenMonitor_t *gmon, const unsigned char *json_data,
    jsmntok_t *tokens, int num_parsed_tokens
) {
    unsigned char *user_var_name = NULL;
    int            user_var_len  = 0;
    gMonStatus status = GMON_RESP_OK;

    if (num_parsed_tokens == 0) {
        return GMON_RESP_ERR_MSG_DECODE; // No tokens to process
    }
    // The first token (index 0) must be the root JSON object
    if (tokens[0].type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int current_token_idx = 1; // Start from the first key-token of the root object
    int num_top_level_pairs = tokens[0].size;

    for (int i = 0; i < num_top_level_pairs && (status == GMON_RESP_OK); i++) {
        if (current_token_idx + 1 >= num_parsed_tokens) {
            status = GMON_RESP_ERR_MSG_DECODE; // Incomplete JSON, expected value for key
            break;
        }

        jsmntok_t *key_token = &tokens[current_token_idx];
        if (key_token->type != JSMN_STRING) {
            status = GMON_RESP_ERR_MSG_DECODE; // Expected a string key
            break;
        }

        user_var_len  =  key_token->end - key_token->start;
        user_var_name = (unsigned char *)&json_data[key_token->start];
        jsmntok_t *value_token = &tokens[current_token_idx + 1];

        int tokens_consumed_by_value = 0; // This will be updated by helper functions, or calculated for unknown keys

        if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL, user_var_name, user_var_len) == 0) {
            status = staDecodeIntervalObject(gmon, json_data, tokens, current_token_idx + 1, &tokens_consumed_by_value);
        } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_THRESHOLD, user_var_name, user_var_len) == 0) {
            status = staDecodeThresholdObject(gmon, json_data, tokens, current_token_idx + 1, &tokens_consumed_by_value);
        } else {
            // Unknown top-level key. Calculate tokens to skip for its value.
            if (value_token->type == JSMN_OBJECT || value_token->type == JSMN_ARRAY) {
                tokens_consumed_by_value = 1 + value_token->size * 2;
            } else { // primitive, string
                tokens_consumed_by_value = 1;
            }
        }
        current_token_idx += (1 /* for key_token */ + tokens_consumed_by_value);
    }

    return status;
}

// decode JSON-based message sent by remote backend server, the message may contain modification request
// e.g. threshold to trigger each output device, time interval to send logs to remote backend service ...
// the function below checks each node of the JSON message, update everything specified by remote user after
// successful decoding process.
gMonStatus  staDecodeAppMsgInflight(gardenMonitor_t *gmon) {
    gMonStatus status = GMON_RESP_OK;
    jsmn_parser *decoder_ptr = (jsmn_parser *)gmon->rawmsg.jsn_decoder;
    jsmntok_t   *tokens_ptr  = (jsmntok_t *)gmon->rawmsg.jsn_decoded_token;
    
    jsmn_init(decoder_ptr);
    short  r = (short) jsmn_parse(decoder_ptr, (const char*)gmon->rawmsg.inflight.data,  gmon->rawmsg.inflight.len,
                   tokens_ptr, GMON_NUM_JSON_TOKEN_DECODE);
    if(r <= 0) { // If parse failed or no tokens
        status = GMON_RESP_ERR_MSG_DECODE;
    } else {
        status = staDecodeAppMsgInflightCore(gmon, gmon->rawmsg.inflight.data, tokens_ptr, r);
    }
    XMEMSET(gmon->rawmsg.inflight.data, GMON_JSON_WHITESPACE, sizeof(char) * gmon->rawmsg.inflight.len);
    return status;
}

gmonStr_t* staGetAppMsgOutflight(gardenMonitor_t *gmon) {
    return &gmon->rawmsg.outflight;
}

gmonStr_t*  staGetAppMsgInflight(gardenMonitor_t *gmon) {
    gMonRawMsg_t *rmsg = &gmon->rawmsg;
    XMEMSET(rmsg->inflight.data, 0, rmsg->inflight.len);
    return &rmsg->inflight;
}

static void staUpdateLastRecord(gmonSensorRecord_t *records, gmonEvent_t *evt) {
    // `records[0]` represents the newest/currently aggregating record.
    // `records[GMON_APPMSG_NUM_RECORDS - 1]` represents the oldest record.

    // Check if the current aggregating record (`records[0]`) has collected all sensor data types.
    if (records[0].flgs.air_val_written && records[0].flgs.soil_val_written && records[0].flgs.light_val_written) {
        // If the current record is complete, shift all existing records to make space for a new one at `record[0]`.
        // This effectively discards the oldest record (at `record[GMON_APPMSG_NUM_RECORDS - 1]`).
        for (int i = GMON_APPMSG_NUM_RECORDS - 1; i > 0; i--) {
            records[i] = records[i-1];
        }
        // Clear the new `records[0]` to prepare it for fresh aggregation of incoming events.
        XMEMSET(&records[0], 0, sizeof(gmonSensorRecord_t));
    }

    // Update the current aggregating record (`record[0]`) with data from the new event.
    records[0].curr_ticks = evt->curr_ticks;
    records[0].curr_days  = evt->curr_days ;

    switch(evt->event_type) {
        case GMON_EVENT_SOIL_MOISTURE_UPDATED:
            records[0].soil_moist = evt->data.soil_moist;
            records[0].flgs.soil_val_written = 1;
            break;
        case GMON_EVENT_LIGHTNESS_UPDATED:
            records[0].lightness = evt->data.lightness;
            records[0].flgs.light_val_written = 1;
            break;
        case GMON_EVENT_AIR_TEMP_UPDATED:
            records[0].air_temp = evt->data.air_temp;
            records[0].air_humid = evt->data.air_humid;
            records[0].flgs.air_val_written = 1;
            break;
        default:
            break;
    }
}

void  stationSensorDataAggregatorTaskFn(void *params) {
    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    while (1) {
        const uint32_t  block_time = 5000;
        gmonEvent_t  *new_evt = NULL;
        gMonStatus status = staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&new_evt, block_time);
        if(new_evt != NULL) {
            configASSERT(status == GMON_RESP_OK);
            staUpdateLastRecord(gmon_latest_sensor_data, new_evt);
            staFreeSensorEvent(new_evt);
            new_evt = NULL;
        }
    }
}
