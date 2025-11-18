#ifndef STATION_TYPES_H
#define STATION_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

// return code that represents status after executing TLS function
typedef enum {
    GMON_RESP_OK              =  0,
    GMON_RESP_REQ_MOREDATA    =  1,
    GMON_RESP_SKIP            =  2,
    GMON_RESP_ERR             = -1,
    GMON_RESP_ERRARGS         = -2,
    GMON_RESP_ERRMEM          = -3,
    GMON_RESP_ERR_MSG_ENCODE  = -5,
    GMON_RESP_ERR_MSG_DECODE  = -6,
    GMON_RESP_TIMEOUT         = -7,  // network connection timeout, or read sensor timeout
    GMON_RESP_ERR_NOT_SUPPORT = -8,
    GMON_RESP_MALFORMED_DATA  = -9,
    GMON_RESP_INVALID_REQ     = -10, // invalid request from the remote user to configure system
    GMON_RESP_SENSOR_FAIL     = -11, // operation related to sensor (e.g. read) failed
    GMON_RESP_CTRL_FAIL       = -12, // control failure e.g. pump, fan, lamp doesn't work when
                                     // user need to change their state
    GMON_RESP_ERR_CONN        = -15, // Connection error (failed) to MQTT broker 
    GMON_RESP_ERR_SECURE_CONN = -16, // secure connection error, failed to start a session of secure connection

} gMonStatus;


typedef enum {
    GMON_OUT_DEV_STATUS_OFF = 0,
    GMON_OUT_DEV_STATUS_ON,
    GMON_OUT_DEV_STATUS_PAUSE,
    GMON_OUT_DEV_STATUS_BROKEN,
} gMonOutDevStatus;


typedef struct {
    unsigned short  len;
    unsigned char  *data;
} gmonStr_t;

typedef enum {
    GMON_EVENT_SOIL_MOISTURE_UPDATED,
    GMON_EVENT_AIR_TEMP_UPDATED,
    GMON_EVENT_LIGHTNESS_UPDATED,
} gmonEventType_t;

#define GMON_SENSORDATA_COMMON_FIELDS \
    float         air_temp; \
    float         air_humid; \
    unsigned int  soil_moist; \
    unsigned int  lightness;

typedef struct {
    gmonEventType_t event_type;
    union {
        GMON_SENSORDATA_COMMON_FIELDS;
    } data;
    unsigned int  curr_ticks;
    unsigned int  curr_days ;
    struct {
        unsigned char alloc:1;
    } flgs;
} gmonEvent_t;


typedef struct {
    int             threshold;
    unsigned int    max_worktime; // maximum time in milliseconds for a device that has been
                                  // continuously working in one day, maximum value MUST NOT
                                  // be greater than 1000 * 60 * 60 * 24 = 0x5265c00
    unsigned int    curr_worktime; // current working time since this device is turned on last time
    unsigned int    min_resttime; // minimum time in milliseconds to pause a device after it continuously
                                  // worked overtime but still needs to work longer to change
                                  // environment condition e.g. temperature drop, provide more growing
                                  // light...etc.
                                  // Again the maximum value MUST NOT be greater than 1000 * 60 * 60 * 24 = 0x5265c00
    unsigned int    curr_resttime;
    unsigned int    sensor_read_interval;
    gMonOutDevStatus status;
} gMonOutDev_t;


typedef struct {
    gmonStr_t     print_str[GMON_DISPLAY_NUM_PRINT_STRINGS];
    unsigned int  interval_ms;
} gMonDisplay_t;

#ifdef __cplusplus
}
#endif
#endif // end of STATION_TYPES_H
