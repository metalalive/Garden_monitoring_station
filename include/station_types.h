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
    GMON_RESP_INVALID_REQ     = -10, // invalid request from the peer
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


typedef struct {
    float         air_temp;
    float         air_humid;
    unsigned int  soil_moist;
    unsigned int  lightness;
    unsigned int  curr_ticks;
    unsigned int  curr_days ;
    struct {
        unsigned char alloc:1;
        unsigned char avail_air_temp:1;
        unsigned char avail_air_humid:1;
        unsigned char avail_soil_moist:1;
        unsigned char avail_lightness:1;
    } flgs;
} gmonSensorRecord_t;


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


// collecting all information, network handling objects in this application
typedef struct {
    struct {
        gMonOutDev_t bulb;
        gMonOutDev_t pump;
        gMonOutDev_t fan;
    } outdev;
    struct {
        void *sensor_reader;
        void *dev_controller;
        void *netconn_handler;
    } tasks;
    struct {
        unsigned int  default_ms;
        unsigned int  curr_ms;
    } sensor_read_interval;
    void         *netconn;
    unsigned int  netconn_interval_ms;
} gardenMonitor_t;

#ifdef __cplusplus
}
#endif
#endif // end of STATION_TYPES_H
