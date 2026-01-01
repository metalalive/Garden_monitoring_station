#include "station_include.h"
#define JSMN_HEADER
#include "jsmn.h"

// clang-format off
// topic : garden/log
// message :
//
// {
//     "soilmoist": {
//         "ticks": 1999929999, "days": 365, "qty": 2,
//         "corruption": [0, 0, 2, 1, 0, 0],
//         "values": [[1019,1020], [999,997], [988,972], [995,994], [980,958], [1001,996]]
//     },
//     "airtemp": {
//         "ticks": 1999929998, "days": 365, "qty": 1,
//         "corruption": [0, 0, 0, 1, 0],
//         "values": {
//             "temp": [[23.7], [23.8], [23.9], [23.8], [24.0]],
//             "humid": [[89.1], [91.2], [99.9], [101.7], [99.4]]
//         }
//     },
//     "light": {
//         "ticks": 1999829997, "days": 365, "qty": 2,
//         "corruption": [2, 0, 0, 2, 0, 0],
//         "values": [[1023,1020], [1010,1011], [1008,1012], [null,994], [980,958], [1001,996]]
//     }
// }
//
// clang-format on

#define GMON_APPMSG_DATA_NAME_TICKS      "ticks"
#define GMON_APPMSG_DATA_NAME_DAYS       "days"
#define GMON_APPMSG_DATA_NAME_QTY        "qty"
#define GMON_APPMSG_DATA_NAME_CORRUPTION "corruption"
#define GMON_APPMSG_DATA_NAME_VALUES     "values"
#define GMON_APPMSG_DATA_NAME_TEMP       "temp"
#define GMON_APPMSG_DATA_NAME_HUMID      "humid"

#define GMON_APPMSG_MAX_NBYTES_SERIAL_NUMBER 12

// Max number of digits for various fields (as per new proposal)
#define GMON_APPMSG_MAX_DIGITS_TICKS          10
#define GMON_APPMSG_MAX_DIGITS_DAYS           4
#define GMON_APPMSG_MAX_DIGITS_QTY            2
#define GMON_APPMSG_MAX_DIGITS_CORRUPTION     3
#define GMON_APPMSG_MAX_DIGITS_SOIL_LIGHT     4 // for u32 (e.g., 1023)
#define GMON_APPMSG_MAX_DIGITS_AIR_TEMP_HUMID 6 // for float (e.g., -99.99, 101.70)

// Helper to calculate size for a JSON key-value pair, excluding trailing comma
// name_len: length of the key string
// val_len: length of the value string (or max digits)
// returns: "key":value (quotes + colon + value length)
#define KEY_VAL_LEN(name_len, val_len) ((name_len) + 2 /*quotes*/ + 1 /*colon*/ + val_len)

// Helper to calculate size for a JSON array of primitive values
// item_len: length of each item (e.g., max digits)
// num_items: number of items in the array
// returns: [item1,item2,item3] (brackets + items + commas)
#define PRIMITIVE_ARRAY_LEN(item_len, num_items) \
    (2 /*brackets*/ + ((num_items > 0) ? (num_items * item_len + (num_items - 1) * 1 /*commas*/) : 0))

// Helper to calculate size for a JSON 2D array of primitive values
// item_len: length of each innermost item
// num_inner_items: number of items in inner array (e.g., GMON_MAXNUM_SOIL_SENSORS)
// num_outer_items: number of outer arrays (e.g., GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP)
// returns: [[v1,v2],[v3,v4]]
#define PRIMITIVE_2D_ARRAY_LEN(item_len, num_inner_items, num_outer_items) \
    (2 /*outer brackets*/ + (num_outer_items * PRIMITIVE_ARRAY_LEN(item_len, num_inner_items) + \
                             (num_outer_items > 0 ? (num_outer_items - 1) * 1 /*commas*/ : 0)))

// Macro for common fields within a sensor's object (ticks, days, qty, corruption, values key)
#define COMMON_SENSOR_OBJ_BASE_LEN(evts_keep_cfg) \
    (2 /* inner { } for sensor type object */ + \
     KEY_VAL_LEN(sizeof(GMON_APPMSG_DATA_NAME_TICKS) - 1, GMON_APPMSG_MAX_DIGITS_TICKS) + 1 /* comma */ + \
     KEY_VAL_LEN(sizeof(GMON_APPMSG_DATA_NAME_DAYS) - 1, GMON_APPMSG_MAX_DIGITS_DAYS) + 1 /* comma */ + \
     KEY_VAL_LEN(sizeof(GMON_APPMSG_DATA_NAME_QTY) - 1, GMON_APPMSG_MAX_DIGITS_QTY) + 1 /* comma */ + \
     KEY_VAL_LEN( \
         sizeof(GMON_APPMSG_DATA_NAME_CORRUPTION) - 1, \
         PRIMITIVE_ARRAY_LEN(GMON_APPMSG_MAX_DIGITS_CORRUPTION, evts_keep_cfg) \
     ) + \
     1 /* comma */ + \
     KEY_VAL_LEN(sizeof(GMON_APPMSG_DATA_NAME_VALUES) - 1, 0 /* value content size added separately */))

#define FREE_IF_EXIST(v) \
    if (v) { \
        XMEMFREE(v); \
        v = NULL; \
    }

static unsigned short staAppMsgOutflightRequiredBufSz(
    unsigned char num_soil_sensors, unsigned char num_air_sensors, unsigned char num_light_sensors,
    unsigned char num_soil_evts, unsigned char num_air_evts, unsigned char num_light_evts
) {
    unsigned short total_len = 0;
    // Overall JSON object: { ... }
    total_len += 2; // For '{' and '}'
    // --- soilmoist object ---
    // "soilmoist": (value size placeholder)
    total_len += KEY_VAL_LEN(sizeof(GMON_APPMSG_DATA_NAME_SOILMOIST) - 1, 0);
    total_len += COMMON_SENSOR_OBJ_BASE_LEN(num_soil_evts);
    total_len += PRIMITIVE_2D_ARRAY_LEN(GMON_APPMSG_MAX_DIGITS_SOIL_LIGHT, num_soil_sensors, num_soil_evts);
    total_len += 1; // Comma after soilmoist object

    // --- airtemp object ---
    total_len += KEY_VAL_LEN(sizeof(GMON_APPMSG_DATA_NAME_AIRTEMP) - 1, 0); // "airtemp":
    total_len += COMMON_SENSOR_OBJ_BASE_LEN(num_air_evts);
    // "values": { "temp":[[f]], "humid":[[f]] }
    total_len += 2; // For values' inner { }
    total_len += KEY_VAL_LEN(
        sizeof(GMON_APPMSG_DATA_NAME_TEMP) - 1,
        PRIMITIVE_2D_ARRAY_LEN(GMON_APPMSG_MAX_DIGITS_AIR_TEMP_HUMID, num_air_sensors, num_air_evts)
    );
    total_len += 1; // Comma between temp and humid
    total_len += KEY_VAL_LEN(
        sizeof(GMON_APPMSG_DATA_NAME_HUMID) - 1,
        PRIMITIVE_2D_ARRAY_LEN(GMON_APPMSG_MAX_DIGITS_AIR_TEMP_HUMID, num_air_sensors, num_air_evts)
    );
    total_len += 1; // Comma after airtemp object

    // --- light object ---
    total_len += KEY_VAL_LEN(sizeof(GMON_APPMSG_DATA_NAME_LIGHT) - 1, 0); // "light":
    total_len += COMMON_SENSOR_OBJ_BASE_LEN(num_light_evts);
    total_len += PRIMITIVE_2D_ARRAY_LEN(GMON_APPMSG_MAX_DIGITS_SOIL_LIGHT, num_light_sensors, num_light_evts);
    // No comma after the last top-level object "light"
    return total_len;
}

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

gMonStatus staAppMsgOutResetAllRecords(gardenMonitor_t *gmon) {
    if (gmon == NULL)
        return GMON_RESP_ERRARGS;
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.soilmoist);
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.aircond);
    appMsgRecordReset(&gmon->sensors.event, &gmon->latest_logs.light);
    return GMON_RESP_OK;
}

gMonStatus staAppMsgReallocBuffer(gardenMonitor_t *gmon) {
    if (gmon == NULL)
        return GMON_RESP_ERRARGS;
    unsigned char num_soil_sensors = gmon->sensors.soil_moist.super.num_items;
    unsigned char num_air_sensors = gmon->sensors.air_temp.num_items;
    unsigned char num_light_sensors = gmon->sensors.light.num_items;
    gMonRawMsg_t *rmsg = &gmon->rawmsg;

    unsigned short new_outflight_required_len = staAppMsgOutflightRequiredBufSz(
        num_soil_sensors, num_air_sensors, num_light_sensors, GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP,
        GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP, GMON_CFG_NUM_LIGHT_SENSOR_RECORDS_KEEP
    );
    unsigned short inflight_required_len = rmsg->inflight.len;
    unsigned short overall_required_buf_sz = (new_outflight_required_len > inflight_required_len)
                                                 ? new_outflight_required_len
                                                 : inflight_required_len;
    // Use staEnsureStrBufferSize to manage the outflight buffer.
    // This function handles freeing old buffer, allocating new one if size differs,
    // and setting str_info->data and str_info->len.
    gMonStatus status = staEnsureStrBufferSize(&rmsg->outflight, overall_required_buf_sz);
    if (status == GMON_RESP_OK) {
        rmsg->inflight.data = rmsg->outflight.data;
    } else {
        rmsg->inflight.data = NULL;
    }
    return status;
}

// --- Helper functions for JSON serialization ---

// Appends a string to the buffer and updates pointers/length. Returns GMON_RESP_OK or GMON_RESP_ERRMEM.
gMonStatus
staAppMsgSerializeAppendStr(unsigned char **buf_ptr, unsigned short *remaining_len, const char *str) {
    unsigned int len = strlen(str);
    if (len >= *remaining_len)
        return GMON_RESP_ERRMEM;
    XMEMCPY(*buf_ptr, str, len);
    *buf_ptr += len;
    *remaining_len -= len;
    return GMON_RESP_OK;
}

// Writes an unsigned integer to the buffer and updates pointers/length.
gMonStatus staAppMsgSerializeUInt(
    unsigned char **buf_ptr, unsigned short *remaining_len, unsigned int val, unsigned int max_nbytes_used
) {
    // Max for ticks, includes null terminator
    unsigned char num_str[GMON_APPMSG_MAX_NBYTES_SERIAL_NUMBER + 1] = {0};
    unsigned int  len = staCvtUNumToStr(num_str, val);
    XASSERT(len < (GMON_APPMSG_MAX_NBYTES_SERIAL_NUMBER + 1));
    if (len >= *remaining_len)
        return GMON_RESP_ERRMEM;
    else if (len > max_nbytes_used)
        return GMON_RESP_ERR_MSG_ENCODE;
    XMEMCPY(*buf_ptr, num_str, len);
    *buf_ptr += len;
    *remaining_len -= len;
    return GMON_RESP_OK;
}

// Writes a float to the buffer and updates pointers/length. Returns GMON_RESP_OK or GMON_RESP_ERRMEM.
gMonStatus staAppMsgSerializeFloat(
    unsigned char **buf_ptr, unsigned short *remaining_len, float val, unsigned short precision,
    unsigned int max_nbytes_used
) {
    // Max for floats, includes null terminator
    unsigned char num_str[GMON_APPMSG_MAX_NBYTES_SERIAL_NUMBER + 1] = {0};
    unsigned int  len = staCvtFloatToStr(num_str, val, precision);
    XASSERT(len < (GMON_APPMSG_MAX_NBYTES_SERIAL_NUMBER + 1));
    if (len >= *remaining_len)
        return GMON_RESP_ERRMEM;
    else if (len > max_nbytes_used)
        return GMON_RESP_ERR_MSG_ENCODE;
    XMEMCPY(*buf_ptr, num_str, len);
    *buf_ptr += len;
    *remaining_len -= len;
    return GMON_RESP_OK;
}

// Gets the latest valid event from a circular record buffer.
static gmonEvent_t *get_latest_event_from_record(gmonSensorRecord_t *sr) {
    if (sr == NULL || sr->events == NULL || sr->num_refs == 0)
        return NULL;
    // Search backwards from the position *before* inner_wr_ptr for the first non-NULL event
    for (int i = 0; i < sr->num_refs; ++i) {
        unsigned char idx = (sr->inner_wr_ptr - 1 - i + sr->num_refs) % sr->num_refs;
        if (sr->events[idx] != NULL) {
            return sr->events[idx];
        }
    }
    return NULL; // No valid events found
}

#define SERIALIZE_LOG_EVTS(rec, evt, code) \
    { \
        unsigned char i = 0, cdx = 0, _num_written = 0; \
        unsigned char _num_refs = (rec)->num_refs; \
        for (i = 0, cdx = (rec)->inner_wr_ptr; i < _num_refs; ++i, cdx = (cdx + 1) % _num_refs) { \
            gmonEvent_t *(evt) = (rec)->events[cdx]; \
            if ((evt) == NULL) \
                continue; \
            if (_num_written > 0) { \
                if (staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",") != GMON_RESP_OK) \
                    return GMON_RESP_ERRMEM; \
            } \
            (code); \
            _num_written++; \
        } \
    }

// Serializes the "corruption" array field.
static gMonStatus
serialize_corruption_array(unsigned char **buf_ptr, unsigned short *remaining_len, gmonSensorRecord_t *rec) {
    if (staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"corruption\":[") != GMON_RESP_OK)
        return GMON_RESP_ERRMEM;
    SERIALIZE_LOG_EVTS(rec, evt, {
        gMonStatus status = staAppMsgSerializeUInt(
            buf_ptr, remaining_len, evt->flgs.corruption, GMON_APPMSG_MAX_DIGITS_CORRUPTION
        );
        if (status != GMON_RESP_OK)
            return status;
    });
    if (staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "]") != GMON_RESP_OK)
        return GMON_RESP_ERRMEM;
    return GMON_RESP_OK;
}

// Serializes a 2D array of unsigned integer values (for soil and light sensors).
static gMonStatus serialize_uint_2d_array(
    unsigned char **buf_ptr, unsigned short *remaining_len, gmonSensorRecord_t *rec,
    unsigned char num_inner_items, unsigned int max_nbytes_used4int
) {
    gMonStatus status = GMON_RESP_ERRMEM;
    if (staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"values\":[") != GMON_RESP_OK)
        return status;
    SERIALIZE_LOG_EVTS(rec, evt, {
        if (evt->data != NULL) {
            if (staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "[") != GMON_RESP_OK)
                return GMON_RESP_ERRMEM;
            unsigned int *data = (unsigned int *)evt->data;
            for (unsigned char j = 0; j < num_inner_items; ++j) {
                if (j < evt->num_active_sensors) {
                    status = staAppMsgSerializeUInt(buf_ptr, remaining_len, data[j], max_nbytes_used4int);
                } else {
                    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "null");
                }
                if (status != GMON_RESP_OK)
                    return status;
                if (j < num_inner_items - 1) {
                    if (staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",") != GMON_RESP_OK)
                        return GMON_RESP_ERRMEM;
                }
            }
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "]");
        } else {
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "null");
        }
        if (status != GMON_RESP_OK)
            return GMON_RESP_ERRMEM;
    }); // end of SERIALIZE_LOG_EVTS
    if (staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "]") != GMON_RESP_OK)
        return GMON_RESP_ERRMEM;
    return GMON_RESP_OK;
}

// Serializes air conditioning values (temperature and humidity 2D arrays).
static gMonStatus serialize_aircond_values(
    unsigned char **buf_ptr, unsigned short *remaining_len, gmonSensorRecord_t *rec,
    unsigned char num_inner_items, unsigned int max_nbytes_used4fp
) {
    gMonStatus status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"values\":{");
    if (status != GMON_RESP_OK)
        return status;

    // "temp" array
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"temp\":[");
    if (status != GMON_RESP_OK)
        return status;
    SERIALIZE_LOG_EVTS(rec, evt, {
        if (evt->data != NULL) {
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "[");
            if (status != GMON_RESP_OK)
                return status;
            gmonAirCond_t *data = (gmonAirCond_t *)evt->data;
            for (unsigned char j = 0; j < num_inner_items; ++j) {
                if (j < evt->num_active_sensors) {
                    status = staAppMsgSerializeFloat(
                        buf_ptr, remaining_len, data[j].temporature, 1, max_nbytes_used4fp
                    );
                } else {
                    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "null");
                }
                if (status != GMON_RESP_OK)
                    return status;
                if (j < num_inner_items - 1) {
                    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",");
                    if (status != GMON_RESP_OK)
                        return status;
                }
            }
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "]");
        } else {
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "null");
        }
        if (status != GMON_RESP_OK)
            return status;
    });
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "],"); // Comma after "temp" array
    if (status != GMON_RESP_OK)
        return status;

    // "humid" array
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"humid\":[");
    if (status != GMON_RESP_OK)
        return status;
    SERIALIZE_LOG_EVTS(rec, evt, {
        if (evt->data != NULL) {
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "[");
            if (status != GMON_RESP_OK)
                return status;
            gmonAirCond_t *data = (gmonAirCond_t *)evt->data;
            for (unsigned char j = 0; j < num_inner_items; ++j) {
                if (j < evt->num_active_sensors) {
                    status = staAppMsgSerializeFloat(
                        buf_ptr, remaining_len, data[j].humidity, 1, max_nbytes_used4fp
                    );
                } else {
                    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "null");
                }
                if (status != GMON_RESP_OK)
                    return status;
                if (j < num_inner_items - 1) {
                    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",");
                    if (status != GMON_RESP_OK)
                        return status;
                }
            }
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "]");
        } else {
            status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "null");
        }
        if (status != GMON_RESP_OK)
            return status;
    });                                                                // end of SERIALIZE_LOG_EVTS
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "]"); // Comma after "humid" array
    if (status != GMON_RESP_OK)
        return status;

    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "}"); // Close "values" object
    if (status != GMON_RESP_OK)
        return status;
    return GMON_RESP_OK;
}

// Function to handle serialization of a single sensor type (e.g., soilmoist, airtemp, light)
static gMonStatus serialize_sensor_type_object(
    unsigned char **buf_ptr, unsigned short *remaining_len, const char *sensor_name, gMonSensorMeta_t *s_meta,
    gmonSensorRecord_t *rec, gmonSensorDataType_t data_type,
    unsigned char is_last_sensor_type // Flag to avoid trailing comma for the last top-level object
) {
    gMonStatus status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"");
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, sensor_name);
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\":{");
    if (status != GMON_RESP_OK)
        return status;

    gmonEvent_t *latest_evt = get_latest_event_from_record(rec);
    unsigned int ticks = (latest_evt != NULL) ? latest_evt->curr_ticks : 0;
    unsigned int days = (latest_evt != NULL) ? latest_evt->curr_days : 0;
    // "ticks"
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"ticks\":");
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeUInt(buf_ptr, remaining_len, ticks, GMON_APPMSG_MAX_DIGITS_TICKS);
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",");
    if (status != GMON_RESP_OK)
        return status;

    // "days"
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"days\":");
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeUInt(buf_ptr, remaining_len, days, GMON_APPMSG_MAX_DIGITS_DAYS);
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",");
    if (status != GMON_RESP_OK)
        return status;

    // "qty"
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"qty\":");
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeUInt(buf_ptr, remaining_len, s_meta->num_items, GMON_APPMSG_MAX_DIGITS_QTY);
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",");
    if (status != GMON_RESP_OK)
        return status;

    // "corruption"
    status = serialize_corruption_array(buf_ptr, remaining_len, rec);
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ",");
    if (status != GMON_RESP_OK)
        return status;

    // "values" (specific to data type)
    switch (data_type) {
    case GMON_SENSOR_DATA_TYPE_U32:
        status = serialize_uint_2d_array(
            buf_ptr, remaining_len, rec, s_meta->num_items, GMON_APPMSG_MAX_DIGITS_SOIL_LIGHT
        );
        break;
    case GMON_SENSOR_DATA_TYPE_AIRCOND:
        status = serialize_aircond_values(
            buf_ptr, remaining_len, rec, s_meta->num_items, GMON_APPMSG_MAX_DIGITS_AIR_TEMP_HUMID
        );
        break;
    default:
        // Fallback for unsupported types
        status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "\"values\":[]");
        break;
    }
    if (status != GMON_RESP_OK)
        return status;
    status = staAppMsgSerializeAppendStr(buf_ptr, remaining_len, "}"); // Close sensor type object
    if (status != GMON_RESP_OK)
        return status;
    if (!is_last_sensor_type) {
        status =
            staAppMsgSerializeAppendStr(buf_ptr, remaining_len, ","); // Add comma if not the last sensor type
    }
    return status;
}

gmonAppMsgOutflightResult_t staGetAppMsgOutflight(gardenMonitor_t *gmon) {
    gmonStr_t     *outflight_msg = &gmon->rawmsg.outflight;
    unsigned char *buf_ptr = outflight_msg->data;
    unsigned short remaining_len = outflight_msg->len;
    // XMEMSET(buf_ptr, 0x0, sizeof(unsigned char) * remaining_len);
    // Start of the overall JSON object
    gMonStatus status = staAppMsgSerializeAppendStr(&buf_ptr, &remaining_len, "{");
    if (status != GMON_RESP_OK)
        goto done;
    status = serialize_sensor_type_object(
        &buf_ptr, &remaining_len, GMON_APPMSG_DATA_NAME_SOILMOIST, &gmon->sensors.soil_moist.super,
        &gmon->latest_logs.soilmoist, GMON_SENSOR_DATA_TYPE_U32, 0 // Not last sensor type
    );
    if (status != GMON_RESP_OK)
        goto done;
    status = serialize_sensor_type_object(
        &buf_ptr, &remaining_len, GMON_APPMSG_DATA_NAME_AIRTEMP, &gmon->sensors.air_temp,
        &gmon->latest_logs.aircond, GMON_SENSOR_DATA_TYPE_AIRCOND, 0 // Not last sensor type
    );
    if (status != GMON_RESP_OK)
        goto done;
    status = serialize_sensor_type_object(
        &buf_ptr, &remaining_len, GMON_APPMSG_DATA_NAME_LIGHT, &gmon->sensors.light, &gmon->latest_logs.light,
        GMON_SENSOR_DATA_TYPE_U32,
        1 // Last sensor type, no trailing comma needed
    );
    if (status != GMON_RESP_OK)
        goto done;
    status = staAppMsgSerializeAppendStr(&buf_ptr, &remaining_len, "}\x00");
done:
    outflight_msg->nbytes_written = outflight_msg->len - remaining_len;
    return (gmonAppMsgOutflightResult_t){.msg = outflight_msg, .status = status};
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
