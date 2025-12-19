#include "station_include.h"
#include "jsmn.h"

// clang-format off
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
// clang-format on

#define GMON_APPMSG_DATA_NAME_SENSOR       "sensor"
#define GMON_APPMSG_DATA_NAME_NETCONN      "netconn"
#define GMON_APPMSG_DATA_NAME_INTERVAL     "interval"
#define GMON_APPMSG_DATA_NAME_ACTUATORS    "actuators"
#define GMON_APPMSG_DATA_NAME_PUMP         "pump"
#define GMON_APPMSG_DATA_NAME_FAN          "fan"
#define GMON_APPMSG_DATA_NAME_BULB         "bulb"
#define GMON_APPMSG_DATA_NAME_MAX_WORKTIME "max_worktime"
#define GMON_APPMSG_DATA_NAME_MIN_RESTTIME "min_resttime"
#define GMON_APPMSG_DATA_NAME_DAYLENGTH    "daylength"
#define GMON_APPMSG_DATA_NAME_THRESHOLD    "threshold"

static gMonStatus staDecodeMsgCvtStrToInt(const unsigned char *json_data, jsmntok_t *tokn, int *out) {
    unsigned char *user_var_name = NULL;
    int            user_var_len = 0;
    gMonStatus     status = GMON_RESP_OK;
    user_var_len = tokn->end - tokn->start;
    user_var_name = (unsigned char *)&json_data[tokn->start];
    status = staChkIntFromStr(user_var_name, user_var_len);
    if (status == GMON_RESP_OK) {
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
    const unsigned char *json_data, jsmntok_t *tokens, int start_obj_tok_idx, gMonActuator_t *actuator,
    int *tokens_consumed_out
) {
    gMonStatus status = GMON_RESP_OK;
    jsmntok_t *config_obj_token = &tokens[start_obj_tok_idx];
    *tokens_consumed_out = 1; // For the config object itself

    if (config_obj_token->type != JSMN_OBJECT)
        return GMON_RESP_ERR_MSG_DECODE;
    int parsed_int = 0, curr_child_tok_idx = start_obj_tok_idx + 1;
    for (int i = 0; i < config_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[curr_child_tok_idx];
        jsmntok_t *child_value_token = &tokens[curr_child_tok_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int            child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_MAX_WORKTIME, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    actuator->max_worktime = (unsigned int)parsed_int;
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_MIN_RESTTIME, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    actuator->min_resttime = (unsigned int)parsed_int;
                }
            } else {
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                curr_child_tok_idx += (1 /*key token*/ + skip_tokens_for_value);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                continue;
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
        }
        curr_child_tok_idx += 2;
        *tokens_consumed_out += 2;
    }
    return status;
}

static gMonStatus staDecodeActuatorsBlock(
    gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_obj_tok_idx,
    int *tokens_consumed_out
) {
    gMonStatus status = GMON_RESP_OK;
    jsmntok_t *actuators_block_obj_token = &tokens[start_obj_tok_idx];
    *tokens_consumed_out = 1;

    if (actuators_block_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }

    int curr_child_tok_idx = start_obj_tok_idx + 1;
    for (int i = 0; i < actuators_block_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[curr_child_tok_idx];
        jsmntok_t *child_value_token = &tokens[curr_child_tok_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];
            int            child_key_len = child_key_token->end - child_key_token->start;
            int            cfg_toks_consumed = 0;

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_PUMP, child_key_name, child_key_len) == 0) {
                status = staDecodeActuatorConfig(
                    json_data, tokens, curr_child_tok_idx + 1, &gmon->actuator.pump, &cfg_toks_consumed
                );
                *tokens_consumed_out += (1 /*key token*/ + cfg_toks_consumed);
                curr_child_tok_idx += (1 /*key token*/ + cfg_toks_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_FAN, child_key_name, child_key_len) == 0) {
                status = staDecodeActuatorConfig(
                    json_data, tokens, curr_child_tok_idx + 1, &gmon->actuator.fan, &cfg_toks_consumed
                );
                *tokens_consumed_out += (1 /*key token*/ + cfg_toks_consumed);
                curr_child_tok_idx += (1 /*key token*/ + cfg_toks_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_BULB, child_key_name, child_key_len) == 0) {
                status = staDecodeActuatorConfig(
                    json_data, tokens, curr_child_tok_idx + 1, &gmon->actuator.bulb, &cfg_toks_consumed
                );
                *tokens_consumed_out += (1 /*key token*/ + cfg_toks_consumed);
                curr_child_tok_idx += (1 /*key token*/ + cfg_toks_consumed);
            } else {
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                curr_child_tok_idx += (1 /*key token*/ + skip_tokens_for_value);
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
            curr_child_tok_idx++;
            *tokens_consumed_out += 1;
        }
    }
    return status;
}

static gMonStatus staDecodeSensorConfig(
    const unsigned char *json_data, jsmntok_t *tokens, int start_obj_tok_idx, gMonSensorMeta_t *sensor_cfg,
    gMonActuator_t *actuator, gMonStatus (*set_threshold_fn)(gMonActuator_t *, unsigned int),
    gMonStatus *threshold_status_field, int *tokens_consumed_out
) {
    gMonStatus status = GMON_RESP_OK;
    int        parsed_int = 0;
    jsmntok_t *config_obj_token = &tokens[start_obj_tok_idx];
    *tokens_consumed_out = 1; // For the config object itself

    if (config_obj_token->type != JSMN_OBJECT) {
        return GMON_RESP_ERR_MSG_DECODE;
    }
    int curr_child_tok_idx = start_obj_tok_idx + 1;
    for (int i = 0; i < config_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[curr_child_tok_idx];
        jsmntok_t *child_value_token = &tokens[curr_child_tok_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int            child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK && sensor_cfg != NULL) {
                    sensor_cfg->read_interval_ms = (unsigned int)parsed_int;
                }
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_THRESHOLD, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK && actuator != NULL && set_threshold_fn != NULL &&
                    threshold_status_field != NULL) {
                    *threshold_status_field = set_threshold_fn(actuator, (unsigned int)parsed_int);
                }
            } else {
                // Unknown key in sensor config, skip its value (which could be an object)
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                curr_child_tok_idx += (1 /*key token*/ + skip_tokens_for_value);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                continue; // Skip the normal advancement at the end of the loop
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
        }
        curr_child_tok_idx += 2;
        *tokens_consumed_out += 2;
    }
    return status;
}

static gMonStatus staDecodeSensorBlock(
    gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_obj_tok_idx,
    int *tokens_consumed_out
) {
    gMonStatus status = GMON_RESP_OK;
    jsmntok_t *sensor_block_obj_token = &tokens[start_obj_tok_idx];
    *tokens_consumed_out = 1; // For the sensor block object itself

    if (sensor_block_obj_token->type != JSMN_OBJECT)
        return GMON_RESP_ERR_MSG_DECODE;

    int curr_child_tok_idx = start_obj_tok_idx + 1;
    for (int i = 0; i < sensor_block_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[curr_child_tok_idx];
        jsmntok_t *child_value_token = &tokens[curr_child_tok_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];
            int            child_key_len = child_key_token->end - child_key_token->start;
            int            cfg_toks_consumed = 0;

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_SOILMOIST, child_key_name, child_key_len) == 0) {
                status = staDecodeSensorConfig(
                    json_data, tokens, curr_child_tok_idx + 1, &gmon->sensors.soil_moist.super,
                    &gmon->actuator.pump, staSetTrigThresholdPump,
                    &gmon->user_ctrl.status.threshold.soil_moist, &cfg_toks_consumed
                );
                *tokens_consumed_out += (1 /*key token*/ + cfg_toks_consumed);
                curr_child_tok_idx += (1 /*key token*/ + cfg_toks_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_AIRTEMP, child_key_name, child_key_len) == 0) {
                status = staDecodeSensorConfig(
                    json_data, tokens, curr_child_tok_idx + 1, &gmon->sensors.air_temp, &gmon->actuator.fan,
                    staSetTrigThresholdFan, &gmon->user_ctrl.status.threshold.air_temp, &cfg_toks_consumed
                );
                *tokens_consumed_out += (1 /*key token*/ + cfg_toks_consumed);
                curr_child_tok_idx += (1 /*key token*/ + cfg_toks_consumed);
            } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_LIGHT, child_key_name, child_key_len) == 0) {
                status = staDecodeSensorConfig(
                    json_data, tokens, curr_child_tok_idx + 1, &gmon->sensors.light, &gmon->actuator.bulb,
                    staSetTrigThresholdBulb, &gmon->user_ctrl.status.threshold.lightness, &cfg_toks_consumed
                );
                *tokens_consumed_out += (1 /*key token*/ + cfg_toks_consumed);
                curr_child_tok_idx += (1 /*key token*/ + cfg_toks_consumed);
            } else {
                // Unknown sensor type, skip its value (which is an object)
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                curr_child_tok_idx += (1 /*key token*/ + skip_tokens_for_value);
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
            curr_child_tok_idx++;
            *tokens_consumed_out += 1;
        }
    }
    return status;
}

static gMonStatus staDecodeNetconnBlock(
    gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int start_obj_tok_idx,
    int *tokens_consumed_out
) {
    gMonStatus status = GMON_RESP_OK;
    jsmntok_t *netconn_obj_token = &tokens[start_obj_tok_idx];
    *tokens_consumed_out = 1; // For the netconn object itself

    if (netconn_obj_token->type != JSMN_OBJECT)
        return GMON_RESP_ERR_MSG_DECODE;

    int parsed_int = 0, curr_child_tok_idx = start_obj_tok_idx + 1;
    for (int i = 0; i < netconn_obj_token->size && status == GMON_RESP_OK; i++) {
        jsmntok_t *child_key_token = &tokens[curr_child_tok_idx];
        jsmntok_t *child_value_token = &tokens[curr_child_tok_idx + 1];

        if (child_key_token->type == JSMN_STRING) {
            int            child_key_len = child_key_token->end - child_key_token->start;
            unsigned char *child_key_name = (unsigned char *)&json_data[child_key_token->start];

            if (XSTRNCMP(GMON_APPMSG_DATA_NAME_INTERVAL, child_key_name, child_key_len) == 0) {
                status = staDecodeMsgCvtStrToInt(json_data, child_value_token, &parsed_int);
                if (status == GMON_RESP_OK) {
                    gmon->user_ctrl.status.interval.netconn =
                        staSetNetConnTaskInterval(&gmon->netconn, (unsigned int)parsed_int);
                }
            } else {
                // Unknown key in netconn block, skip its value (which could be an object)
                int skip_tokens_for_value = staCalcTokensToSkip(child_value_token);
                curr_child_tok_idx += (1 /*key token*/ + skip_tokens_for_value);
                *tokens_consumed_out += (1 /*key token*/ + skip_tokens_for_value);
                continue; // Skip the normal advancement at the end of the loop
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
        }
        curr_child_tok_idx += 2;
        *tokens_consumed_out += 2;
    }
    return status;
}

// decode JSON-based message sent by remote backend server, the message may contain modification request
// e.g. threshold to trigger each output device, time interval to send logs to remote backend service ...
// the function below checks each node of the JSON message, update everything specified by remote user after
// successful decoding process. This is the core logic moved to a separate function for testability.
static gMonStatus staDecodeAppMsgInflightCore(
    gardenMonitor_t *gmon, const unsigned char *json_data, jsmntok_t *tokens, int num_parsed_tokens
) {
    unsigned char *user_var_name = NULL;
    int            user_var_len = 0;
    gMonStatus     status = GMON_RESP_OK;

    if (num_parsed_tokens == 0) // No tokens to process
        return GMON_RESP_ERR_MSG_DECODE;
    // The first token (index 0) must be the root JSON object
    if (tokens[0].type != JSMN_OBJECT)
        return GMON_RESP_ERR_MSG_DECODE;
    // `curr_tok_idx` starts from the first key-token of the root object
    int parsed_int = 0, curr_tok_idx = 1, num_top_level_pairs = tokens[0].size;

    for (int i = 0; i < num_top_level_pairs && (status == GMON_RESP_OK); i++) {
        if (curr_tok_idx + 1 >= num_parsed_tokens) {
            status = GMON_RESP_ERR_MSG_DECODE; // Incomplete JSON, expected value for key
            break;
        }
        jsmntok_t *key_token = &tokens[curr_tok_idx];
        if (key_token->type != JSMN_STRING) {
            status = GMON_RESP_ERR_MSG_DECODE; // Expected a string key
            break;
        }
        user_var_len = key_token->end - key_token->start;
        user_var_name = (unsigned char *)&json_data[key_token->start];
        jsmntok_t *value_token = &tokens[curr_tok_idx + 1];
        // This will be updated by helper functions, or calculated for unknown keys
        int num_consumed = 0;
        if (XSTRNCMP(GMON_APPMSG_DATA_NAME_SENSOR, user_var_name, user_var_len) == 0) {
            status = staDecodeSensorBlock(gmon, json_data, tokens, curr_tok_idx + 1, &num_consumed);
        } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_NETCONN, user_var_name, user_var_len) == 0) {
            status = staDecodeNetconnBlock(gmon, json_data, tokens, curr_tok_idx + 1, &num_consumed);
        } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_DAYLENGTH, user_var_name, user_var_len) == 0) {
            status = staDecodeMsgCvtStrToInt(json_data, value_token, &parsed_int);
            if (status == GMON_RESP_OK) {
                gmon->user_ctrl.status.threshold.daylength =
                    staSetRequiredDaylenTicks(gmon, (unsigned int)parsed_int);
            }
            num_consumed = 1; // Primitive value consumes 1 token
        } else if (XSTRNCMP(GMON_APPMSG_DATA_NAME_ACTUATORS, user_var_name, user_var_len) == 0) {
            status = staDecodeActuatorsBlock(gmon, json_data, tokens, curr_tok_idx + 1, &num_consumed);
        } else {
            // Unknown top-level key. Calculate tokens to skip for its value.
            num_consumed = staCalcTokensToSkip(value_token);
        }
        curr_tok_idx += (1 /* for key_token */ + num_consumed);
    }
    return status;
}

// decode JSON-based message sent by remote backend server, the message may contain modification request
// e.g. threshold to trigger each output device, time interval to send logs to remote backend service ...
// the function below checks each node of the JSON message, update everything specified by remote user after
// successful decoding process.
gMonStatus staDecodeAppMsgInflight(gardenMonitor_t *gmon) {
    gMonStatus   status = GMON_RESP_OK;
    jsmn_parser *decoder_ptr = (jsmn_parser *)gmon->rawmsg.jsn_decoder;
    jsmntok_t   *tokens_ptr = (jsmntok_t *)gmon->rawmsg.jsn_decoded_token;

    jsmn_init(decoder_ptr);
    short r = (short)jsmn_parse(
        decoder_ptr, (const char *)gmon->rawmsg.inflight.data, gmon->rawmsg.inflight.len, tokens_ptr,
        GMON_NUM_JSON_TOKEN_DECODE
    );
    if (r <= 0) { // If parse failed or no tokens
        status = GMON_RESP_ERR_MSG_DECODE;
    } else {
        status = staDecodeAppMsgInflightCore(gmon, gmon->rawmsg.inflight.data, tokens_ptr, r);
    }
    return status;
}

gmonStr_t *staGetAppMsgInflight(gardenMonitor_t *gmon) {
    gMonRawMsg_t *rmsg = &gmon->rawmsg;
    XMEMSET(rmsg->inflight.data, 0, rmsg->inflight.len);
    return &rmsg->inflight;
}

gMonStatus staAppMsgInit(gardenMonitor_t *gmon) {
    gMonRawMsg_t *rmsg = &gmon->rawmsg;
    // Calculate required buffer sizes for outflight and inflight messages
    rmsg->outflight.len = staAppMsgOutflightCalcRequiredBufSz();
    rmsg->inflight.len = staAppMsgInflightCalcRequiredBufSz();
    rmsg->outflight.data = XMALLOC(sizeof(unsigned char) * rmsg->outflight.len);
    rmsg->inflight.data = XMALLOC(sizeof(unsigned char) * rmsg->inflight.len);
    rmsg->jsn_decoder = XCALLOC(1, sizeof(jsmn_parser));
    rmsg->jsn_decoded_token = XCALLOC(GMON_NUM_JSON_TOKEN_DECODE, sizeof(jsmntok_t));
    uint8_t any_failed = (rmsg->outflight.data == NULL) || (rmsg->inflight.data == NULL) ||
                         (rmsg->jsn_decoder == NULL) || (rmsg->jsn_decoded_token == NULL);
    if (any_failed) // call de-init function below if init failed
        return GMON_RESP_ERRMEM;
    staParseFixedPartsOutAppMsg(&rmsg->outflight);
    staAppMsgOutResetAllRecords(gmon);
    return GMON_RESP_OK;
}

gMonStatus staAppMsgDeinit(gardenMonitor_t *gmon) {
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
