#ifndef STATION_TYPES_H
#define STATION_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

// return code that represents status after executing TLS function
typedef enum {
    GMON_RESP_OK = 0,
    GMON_RESP_SKIP = 2,
    GMON_RESP_ERR = -1,
    GMON_RESP_ERRARGS = -2,
    GMON_RESP_ERRMEM = -3,
    GMON_RESP_ERR_MSG_ENCODE = -5,
    GMON_RESP_ERR_MSG_DECODE = -6,
    GMON_RESP_TIMEOUT = -7, // network connection timeout, or read sensor timeout
    GMON_RESP_ERR_NOT_SUPPORT = -8,
    GMON_RESP_MALFORMED_DATA = -9,
    GMON_RESP_INVALID_REQ = -10, // invalid request from the remote user to configure system
    GMON_RESP_SENSOR_FAIL = -11, // operation related to sensor (e.g. read) failed
    GMON_RESP_CTRL_FAIL = -12,   // control failure e.g. pump, fan, lamp doesn't work when
                                 // user need to change their state
    GMON_RESP_ERR_CONN = -15,    // Connection error (failed) to MQTT broker
    // secure connection error, failed to start a session of secure connection
    GMON_RESP_ERR_SECURE_CONN = -16,

} gMonStatus;

typedef enum {
    GMON_OUT_DEV_STATUS_OFF = 0,
    GMON_OUT_DEV_STATUS_ON,
    GMON_OUT_DEV_STATUS_PAUSE,
    GMON_OUT_DEV_STATUS_BROKEN,
} gMonActuatorStatus;

typedef struct {
    unsigned short len;
    unsigned char *data;
} gmonStr_t;

typedef struct {
    float temporature;
    float humidity;
} gmonAirCond_t;

typedef enum {
    GMON_SENSOR_DATA_TYPE_UNKNOWN = 0,
    GMON_SENSOR_DATA_TYPE_U32 = 1,
    GMON_SENSOR_DATA_TYPE_AIRCOND,
} gmonSensorDataType_t;

typedef struct {
    gmonSensorDataType_t dtype : 4;
    // identity for each sensor type
    unsigned char id : 4;
    // num of items in `data` field
    unsigned short len;
    // array of values read from an individual sensor
    // cast the pointer to corresponding type depending on `dtype` above :
    // - unsigned int , if GMON_SENSOR_DATA_TYPE_U32
    // - gmonAirCond_t , if GMON_SENSOR_DATA_TYPE_AIRCOND
    void *data;
    // 1-bit flag for each read data  which indicates whether it
    // is outlier among sample data sequence due to impulse noise
    unsigned char *outlier;
} gmonSensorSample_t;

typedef enum {
    GMON_EVENT_SOIL_MOISTURE_UPDATED,
    GMON_EVENT_AIR_TEMP_UPDATED,
    GMON_EVENT_LIGHTNESS_UPDATED,
} gmonEventType_t;

#define GMON_SENSORDATA_COMMON_FIELDS \
    gmonAirCond_t air_cond; \
    unsigned int  soil_moist; \
    unsigned int  lightness;

typedef struct {
    gmonEventType_t event_type : 4;
    struct {
        // data-corruption flags for installed sensors of the same type,
        // each bit flag indicates sensor ID from `gmonSensorSample_t`
        unsigned char corruption;
        unsigned char alloc : 1;
    } flgs;
    union {
        GMON_SENSORDATA_COMMON_FIELDS;
    } data;
    unsigned int curr_ticks;
    unsigned int curr_days;
} gmonEvent_t;

typedef struct {
    GMON_SENSORDATA_COMMON_FIELDS;
    unsigned int curr_ticks;
    unsigned int curr_days;
    struct {
        unsigned char air_val_written   : 1;
        unsigned char soil_val_written  : 1;
        unsigned char light_val_written : 1;
    } flgs;
} gmonSensorRecord_t;

typedef struct {
    gmonEvent_t *pool;
    unsigned int len;
} gMonEvtPool_t;

typedef struct {
    void        *lowlvl;
    float        outlier_threshold;
    unsigned int read_interval_ms;
    // number of sensors installed in one spot ,
    // note current appllication does not identify sensors in several different spots
    unsigned char num_items     : 4;
    unsigned char num_resamples : 4;
} gMonSensor_t;

typedef struct {
    int threshold;
    // maximum time in milliseconds for a device that has been
    // continuously working in one day, maximum value MUST NOT
    // be greater than 1000 * 60 * 60 * 24 = 0x5265c00
    unsigned int max_worktime;
    // current working time since this device is turned on last time
    unsigned int curr_worktime;
    // minimum time in milliseconds to pause a device after it continuously
    // worked overtime but still needs to work longer to change
    // environment condition e.g. temperature drop, provide more growing
    // light...etc.
    // Again the maximum value MUST NOT be greater than 1000 * 60 * 60 * 24 = 0x5265c00
    unsigned int       min_resttime;
    unsigned int       curr_resttime;
    gMonActuatorStatus status;
} gMonActuator_t;

typedef struct {
    unsigned int ticks_per_day;
    unsigned int days;
    unsigned int last_read;
} gmonTick_t;

// forware declaration
struct gardenMonitor_s;

#ifdef __cplusplus
}
#endif
#endif // end of STATION_TYPES_H
