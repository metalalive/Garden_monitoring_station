#ifndef STATION_MONITOR_H
#define STATION_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct gMonMsgPipe_t {
    stationSysMsgbox_t sensor2display;
    stationSysMsgbox_t sensor2net;
};

typedef struct {
    gmonStr_t outflight;
    gmonStr_t inflight;
    void     *jsn_decoded_token;
    void     *jsn_decoder;
} gMonRawMsg_t;

// collecting all information, network handling objects in this application
typedef struct gardenMonitor_s {
    struct {
        gMonActuator_t bulb;
        gMonActuator_t pump;
        gMonActuator_t fan;
    } actuator;
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
        gMonSoilSensorMeta_t soil_moist;
        gMonSensorMeta_t     air_temp;
        gMonSensorMeta_t     light;
        gMonEvtPool_t        event;
    } sensors;
    struct {
        gmonSensorRecord_t soilmoist;
        gmonSensorRecord_t aircond;
        gmonSensorRecord_t light;
    } latest_logs;
    struct {
        struct {
            struct {
                gMonStatus sensorread; // TODO, expand for each individual sensor
                gMonStatus netconn;
            } interval;
            struct {
                gMonStatus air_temp;
                gMonStatus soil_moist;
                gMonStatus lightness;
                gMonStatus daylength;
            } threshold;
        } status;
        struct {
            unsigned int ticks;
            unsigned int days;
        } last_update;
        unsigned int required_light_daylength_ticks;
    } user_ctrl;
    gMonDisplayContext_t display;
    gMonNet_t            netconn;
    gmonTick_t           tick;
    gMonRawMsg_t         rawmsg;
} gardenMonitor_t;

#ifdef __cplusplus
}
#endif
#endif // end of STATION_MONITOR_H
