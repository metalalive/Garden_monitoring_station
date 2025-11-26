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
//     "sensor": {
//         "soilmoist": {"interval": 2100, "threshold": 1234},
//         "airtemp": {"interval": 7100, "threshold": 35},
//         "light": {"interval": 11000, "threshold": 4321}
//     },
//     "netconn":  {"interval": 360000},
//     "daylength":7200012,
//     "actuators": {
//         "pump": {"max_worktime": 4000, "min_resttime": 1500},
//         "fan":  {"max_worktime": 5300, "min_resttime": 2100},
//         "bulb": {"max_worktime": 7100, "min_resttime": 5700}
//     }
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
#define  GMON_APPMSG_DATA_NAME_SENSOR     "sensor"
#define  GMON_APPMSG_DATA_NAME_SOILMOIST  "soilmoist"
#define  GMON_APPMSG_DATA_NAME_AIRTEMP    "airtemp"
#define  GMON_APPMSG_DATA_NAME_AIRHUMID   "airhumid"
#define  GMON_APPMSG_DATA_NAME_LIGHT      "light"
#define  GMON_APPMSG_DATA_NAME_NETCONN    "netconn"
#define  GMON_APPMSG_DATA_NAME_INTERVAL   "interval"
#define  GMON_APPMSG_DATA_NAME_ACTUATORS  "actuators"
#define  GMON_APPMSG_DATA_NAME_PUMP       "pump"
#define  GMON_APPMSG_DATA_NAME_FAN        "fan"
#define  GMON_APPMSG_DATA_NAME_BULB       "bulb"
#define  GMON_APPMSG_DATA_NAME_MAX_WORKTIME "max_worktime"
#define  GMON_APPMSG_DATA_NAME_MIN_RESTTIME "min_resttime"
#define  GMON_APPMSG_DATA_NAME_DAYLENGTH  "daylength"
#define  GMON_APPMSG_DATA_NAME_THRESHOLD  "threshold"
#define  GMON_APPMSG_DATA_NAME_TICKS      "ticks"
#define  GMON_APPMSG_DATA_NAME_DAYS       "days"

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

typedef struct {
    const char      *name;
    unsigned  short  val_sz;
    unsigned  char  *val_pos[GMON_APPMSG_NUM_RECORDS];
} gmonMsgItem_t;

static gmonMsgItem_t gmon_appmsg_log_records[GMON_APPMSG_NUM_ITEMS_PER_RECORD] = {
    {GMON_APPMSG_DATA_NAME_SOILMOIST,(unsigned short)GMON_APPMSG_DATA_SZ_SOILMOIST, {0}},
    {GMON_APPMSG_DATA_NAME_AIRTEMP  ,(unsigned short)GMON_APPMSG_DATA_SZ_AIRTEMP  , {0}},
    {GMON_APPMSG_DATA_NAME_AIRHUMID ,(unsigned short)GMON_APPMSG_DATA_SZ_AIRHUMID , {0}},
    {GMON_APPMSG_DATA_NAME_LIGHT    ,(unsigned short)GMON_APPMSG_DATA_SZ_LIGHT    , {0}},
    {GMON_APPMSG_DATA_NAME_TICKS    ,(unsigned short)GMON_APPMSG_DATA_SZ_TICKS    , {0}},
    {GMON_APPMSG_DATA_NAME_DAYS     ,(unsigned short)GMON_APPMSG_DATA_SZ_DAYS     , {0}}
};

#define  GMON_NUM_JSON_TOKEN_DECODE  47

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
    // 384 bytes should be ample to accommodate various configuration updates.
    return 384;
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


static void  staAppMsgOutResetAllRecords(gardenMonitor_t *gmon) {
    size_t record_sz = GMON_APPMSG_NUM_RECORDS * sizeof(gmonSensorRecord_t);
    XMEMSET(&gmon->sensors.latest_records[0], 0x00, record_sz);
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
        staAppMsgOutResetAllRecords(gmon);
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

static void  renderRecords2AppMsg(gmonSensorRecord_t *record, short idx) {
    uint32_t   num_chr = 0;
    for(short  jdx = 0; jdx < GMON_APPMSG_NUM_ITEMS_PER_RECORD; jdx++) {
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
    num_chr = staCvtFloatToStr(gmon_appmsg_log_records[1].val_pos[idx], record->air_cond.temporature, 0x2);
    XASSERT(num_chr <= gmon_appmsg_log_records[1].val_sz);
    num_chr = staCvtFloatToStr(gmon_appmsg_log_records[2].val_pos[idx], record->air_cond.humidity, 0x2);
    XASSERT(num_chr <= gmon_appmsg_log_records[2].val_sz);
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

static int staCalcTokensToSkip(jsmntok_t *token) {
    // Calculate how many tokens to skip for a given token value
    // For objects/arrays, we need to skip the token itself plus all nested key-value pairs
    // For primitives/strings, we only skip 1 token
    int tokens_to_skip = 1; // The token itself
    if (token->type == JSMN_OBJECT || token->type == JSMN_ARRAY) {
        tokens_to_skip += token->size * 2; // Each pair is key + value
    }
    return tokens_to_skip;
}

static gMonStatus staDecodeActuatorConfig(
    const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx,
    gMonOutDev_t *outdev, int *tokens_consumed_out)
{
    gMonStatus status = GMON_RESP_OK;
    int parsed_int = 0;
    jsmntok_t *config_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1; // For the config object itself

    if (config_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }
    int current_child_token_idx = start_object_token_idx + 1;
    for (int i = 0; i < config_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[current_child_token_idx];
        jsmntok_t *child_value_token = &tokens[current_child_token_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_MAX_WORKTIME, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    outdev->max_worktime = (unsigned int)parsed_int;
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_MIN_RESTTIME, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    outdev->min_resttime = (unsigned int)parsed_int;
                }
            } else {
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                current_child_token_idx += (1 /*key token*/ + skip_tokens_for_value);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                continue;
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
        }
        current_child_token_idx += 2;
        *tokens_consumed_out += 2;
    }
    return status;
}

static gMonStatus staDecodeActuatorsBlock(gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx, int *tokens_consumed_out) {
    gMonStatus status = GMON_RESP_OK;
    jsmntok_t *actuators_block_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1;

    if (actuators_block_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int current_child_token_idx = start_object_token_idx + 1;
    for (int i = 0; i < actuators_block_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[current_child_token_idx];
        jsmntok_t *child_value_token = &tokens[current_child_token_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_PUMP, child_key_name, child_key_len) == 0) {
                int config_tokens_consumed = 0;
                status = staDecodeActuatorConfig(json_data, tokens, current_child_token_idx + 1,
                                                 &gmon->outdev.pump, &config_tokens_consumed);
                *tokens_consumed_out += (1 /*key token*/ + config_tokens_consumed);
                current_child_token_idx += (1 /*key token*/ + config_tokens_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_FAN, child_key_name, child_key_len) == 0) {
                int config_tokens_consumed = 0;
                status = staDecodeActuatorConfig(json_data, tokens, current_child_token_idx + 1,
                                                 &gmon->outdev.fan, &config_tokens_consumed);
                *tokens_consumed_out += (1 /*key token*/ + config_tokens_consumed);
                current_child_token_idx += (1 /*key token*/ + config_tokens_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_BULB, child_key_name, child_key_len) == 0) {
                int config_tokens_consumed = 0;
                status = staDecodeActuatorConfig(json_data, tokens, current_child_token_idx + 1,
                                                 &gmon->outdev.bulb, &config_tokens_consumed);
                *tokens_consumed_out += (1 /*key token*/ + config_tokens_consumed);
                current_child_token_idx += (1 /*key token*/ + config_tokens_consumed);
            } else {
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                current_child_token_idx += (1 /*key token*/ + skip_tokens_for_value);
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
            current_child_token_idx++;
            *tokens_consumed_out += 1;
        }
    }
    return status;
}

static gMonStatus staDecodeSensorConfig(
    const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx,
    gMonSensor_t *sensor_cfg, gMonOutDev_t *outdev,
    gMonStatus (*set_threshold_fn)(gMonOutDev_t *, unsigned int),
    gMonStatus *threshold_status_field, int *tokens_consumed_out)
{
    gMonStatus status = GMON_RESP_OK;
    int parsed_int = 0;
    jsmntok_t *config_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1; // For the config object itself

    if (config_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }
    int current_child_token_idx = start_object_token_idx + 1;
    for (int i = 0; i < config_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[current_child_token_idx];
        jsmntok_t *child_value_token = &tokens[current_child_token_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK && sensor_cfg != NULL) {
                    sensor_cfg->read_interval_ms = (unsigned int)parsed_int;
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_THRESHOLD, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK && outdev != NULL && set_threshold_fn != NULL && threshold_status_field != NULL) {
                    *threshold_status_field = set_threshold_fn(outdev, (unsigned int)parsed_int);
                }
            } else {
                // Unknown key in sensor config, skip its value (which could be an object)
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                current_child_token_idx += (1 /*key token*/ + skip_tokens_for_value);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                continue; // Skip the normal advancement at the end of the loop
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
        }
        current_child_token_idx += 2;
        *tokens_consumed_out += 2;
    }
    return status;
}

static gMonStatus staDecodeSensorBlock(gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx, int *tokens_consumed_out) {
    gMonStatus status = GMON_RESP_OK;
    jsmntok_t *sensor_block_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1; // For the sensor block object itself

    if (sensor_block_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int current_child_token_idx = start_object_token_idx + 1;
    for (int i = 0; i < sensor_block_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[current_child_token_idx];
        jsmntok_t *child_value_token = &tokens[current_child_token_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_SOILMOIST, child_key_name, child_key_len) == 0) {
                int config_tokens_consumed = 0;
                status = staDecodeSensorConfig(json_data, tokens, current_child_token_idx + 1,
                                               &gmon->sensors.soil_moist, &gmon->outdev.pump, staSetTrigThresholdPump,
                                               &gmon->user_ctrl.status.threshold.soil_moist,
                                               &config_tokens_consumed);
                *tokens_consumed_out += (1 /*key token*/ + config_tokens_consumed);
                current_child_token_idx += (1 /*key token*/ + config_tokens_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_AIRTEMP, child_key_name, child_key_len) == 0) {
                int config_tokens_consumed = 0;
                status = staDecodeSensorConfig(json_data, tokens, current_child_token_idx + 1,
                                               &gmon->sensors.air_temp, &gmon->outdev.fan, staSetTrigThresholdFan,
                                               &gmon->user_ctrl.status.threshold.air_temp,
                                               &config_tokens_consumed);
                *tokens_consumed_out += (1 /*key token*/ + config_tokens_consumed);
                current_child_token_idx += (1 /*key token*/ + config_tokens_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_LIGHT, child_key_name, child_key_len) == 0) {
                int config_tokens_consumed = 0;
                status = staDecodeSensorConfig(json_data, tokens, current_child_token_idx + 1,
                                               &gmon->sensors.light, &gmon->outdev.bulb, staSetTrigThresholdBulb,
                                               &gmon->user_ctrl.status.threshold.lightness,
                                               &config_tokens_consumed);
                *tokens_consumed_out += (1 /*key token*/ + config_tokens_consumed);
                current_child_token_idx += (1 /*key token*/ + config_tokens_consumed);
            } else {
                // Unknown sensor type, skip its value (which is an object)
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                current_child_token_idx += (1 /*key token*/ + skip_tokens_for_value);
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
            current_child_token_idx++;
            *tokens_consumed_out += 1;
        }
    }
    return status;
}

static gMonStatus staDecodeNetconnBlock(gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_object_token_idx, int *tokens_consumed_out) {
    gMonStatus status = GMON_RESP_OK;
    int parsed_int = 0;
    jsmntok_t *netconn_obj_token = &tokens[start_object_token_idx];
    *tokens_consumed_out = 1; // For the netconn object itself

    if (netconn_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int current_child_token_idx = start_object_token_idx + 1;
    for (int i = 0; i < netconn_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[current_child_token_idx];
        jsmntok_t *child_value_token = &tokens[current_child_token_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->user_ctrl.status.interval.netconn = staSetNetConnTaskInterval(gmon, (unsigned int)parsed_int);
                }
            } else {
                // Unknown key in netconn block, skip its value (which could be an object)
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                current_child_token_idx += (1 /*key token*/ + skip_tokens_for_value);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                continue; // Skip the normal advancement at the end of the loop
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
        }
        current_child_token_idx += 2;
        *tokens_consumed_out += 2;
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

    int parsed_int = 0;
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

        if (XSTRNCMP(GMON_APPMSG_DATA_NAME_SENSOR, user_var_name, user_var_len) == 0) {
            status = staDecodeSensorBlock(gmon, json_data, tokens, current_token_idx + 1, &tokens_consumed_by_value);
        } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_NETCONN, user_var_name, user_var_len) == 0) {
            status = staDecodeNetconnBlock(gmon, json_data, tokens, current_token_idx + 1, &tokens_consumed_by_value);
        } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_DAYLENGTH, user_var_name, user_var_len) == 0) {
            status = staDecodeMsgCvtStrToInt(json_data, value_token, &parsed_int);
            if (status == GMON_RESP_OK) {
                gmon->user_ctrl.status.threshold.daylength = staSetRequiredDaylenTicks(gmon, (unsigned int)parsed_int);
            }
            tokens_consumed_by_value = 1; // Primitive value consumes 1 token
        } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_ACTUATORS, user_var_name, user_var_len) == 0) {
            status = staDecodeActuatorsBlock(gmon, json_data, tokens, current_token_idx + 1, &tokens_consumed_by_value);

        } else {
            // Unknown top-level key. Calculate tokens to skip for its value.
            tokens_consumed_by_value = staCalcTokensToSkip(value_token);
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
    return status;
}

gmonStr_t* staGetAppMsgOutflight(gardenMonitor_t *gmon) {
    stationSysEnterCritical();
    for (uint8_t idx = 0; idx < GMON_APPMSG_NUM_RECORDS; idx++) {
        // latest record comes first
        renderRecords2AppMsg(&gmon->sensors.latest_records[idx], idx);
    }
    staAppMsgOutResetAllRecords(gmon);
    stationSysExitCritical();
    return &gmon->rawmsg.outflight;
}

gmonStr_t*  staGetAppMsgInflight(gardenMonitor_t *gmon) {
    gMonRawMsg_t *rmsg = &gmon->rawmsg;
    XMEMSET(rmsg->inflight.data, 0, rmsg->inflight.len);
    return &rmsg->inflight;
}

static gmonSensorRecord_t staShiftRecords(gmonSensorRecord_t *records) {
    gmonSensorRecord_t discarded = records[GMON_APPMSG_NUM_RECORDS - 1];
    // - shift all existing records to make space for a new one at `record[0]`.
    // - this effectively discards the oldest record (at `record[GMON_APPMSG_NUM_RECORDS - 1]`).
    for (int i = GMON_APPMSG_NUM_RECORDS - 1; i > 0; i--) {
        records[i] = records[i-1];
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
    switch(evt->event_type) {
        case GMON_EVENT_SOIL_MOISTURE_UPDATED:
            if (records[0].flgs.soil_val_written) {
                discarded = staShiftRecords(records);
            }
            // Update the current aggregating record (`record[0]`) with data
            // from the new event.
            records[0].curr_ticks = evt->curr_ticks;
            records[0].curr_days  = evt->curr_days ;
            records[0].soil_moist = evt->data.soil_moist;
            records[0].flgs.soil_val_written = 1;
            break;
        case GMON_EVENT_LIGHTNESS_UPDATED:
            if (records[0].flgs.light_val_written) {
                discarded = staShiftRecords(records);
            }
            records[0].curr_ticks = evt->curr_ticks;
            records[0].curr_days  = evt->curr_days ;
            records[0].lightness = evt->data.lightness;
            records[0].flgs.light_val_written = 1;
            break;
        case GMON_EVENT_AIR_TEMP_UPDATED:
            if (records[0].flgs.air_val_written) {
                discarded = staShiftRecords(records);
            }
            records[0].curr_ticks = evt->curr_ticks;
            records[0].curr_days  = evt->curr_days ;
            records[0].air_cond = evt->data.air_cond;
            records[0].flgs.air_val_written = 1;
            break;
        default:
            break;
    }
    stationSysExitCritical();
    return discarded;
} // end of staUpdateLastRecord

void  stationSensorDataAggregatorTaskFn(void *params) {
    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    while (1) {
        const uint32_t  block_time = 5000;
        gmonEvent_t  *new_evt = NULL;
        gMonStatus status = staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&new_evt, block_time);
        if(new_evt != NULL) { // This must be inside the loop
            configASSERT(status == GMON_RESP_OK);
            staUpdateLastRecord(gmon->sensors.latest_records, new_evt);
            staFreeSensorEvent(new_evt);
            new_evt = NULL;
        }
    }
}
