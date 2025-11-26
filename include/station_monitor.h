#ifndef STATION_MONITOR_H
#define STATION_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct gMonMsgPipe_t {
    stationSysMsgbox_t  sensor2display;
    stationSysMsgbox_t  sensor2net;
};

typedef struct  {
    gmonStr_t  outflight;
    gmonStr_t  inflight;
    void      *jsn_decoded_token;
    void      *jsn_decoder;
} gMonRawMsg_t;


// collecting all information, network handling objects in this application
typedef struct {
    struct {
        gMonOutDev_t bulb;
        gMonOutDev_t pump;
        gMonOutDev_t fan;
    } outdev;
    struct {
        void *pump_controller; // New task for pump control and soil moisture
        void *air_quality_monitor;
        void *light_controller;
        void *netconn_handler;
        void *sensor_data_aggregator_net;
        void *display_handler;
    } tasks;
    struct gMonMsgPipe_t msgpipe;
    struct {
        gMonSensor_t soil_moist;
        gMonSensor_t air_temp;
        gMonSensor_t light;
        struct {
            gmonEvent_t *pool;
            size_t len;
        } event;
        gmonSensorRecord_t  latest_records[GMON_CFG_NUM_SENSOR_RECORDS_KEEP];
    } sensors;
    struct {
        void         *handle_obj;
        unsigned int  interval_ms;
        struct {
            gMonStatus  sent;
            gMonStatus  recv;
        } status;
    } netconn;
    struct {
        struct {
            struct {
                gMonStatus  sensorread; // TODO, expand for each individual sensor
                gMonStatus  netconn;
            } interval;
            struct {
                gMonStatus  air_temp;
                gMonStatus  soil_moist;
                gMonStatus  lightness;
                gMonStatus  daylength;
            } threshold;
        } status ;
        struct {
            unsigned int  ticks;
            unsigned int  days ;
        } last_update;
        unsigned int  required_light_daylength_ticks;
    } user_ctrl;
    gMonDisplay_t  display;
    gmonTick_t     tick;
    gMonRawMsg_t   rawmsg;
} gardenMonitor_t;

#ifdef __cplusplus
}
#endif
#endif // end of STATION_MONITOR_H
