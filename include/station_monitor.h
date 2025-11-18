#ifndef STATION_MONITOR_H
#define STATION_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct gMonMsgPipe_t {
    stationSysMsgbox_t  sensor2display;
    stationSysMsgbox_t  sensor2net;
};

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
        void *sensor_data_aggregator_net;
        void *display_handler;
    } tasks;
    struct gMonMsgPipe_t msgpipe;
    struct {
        unsigned int  default_ms;
        unsigned int  curr_ms;
    } sensor_read_interval;
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
                gMonStatus  sensorread;
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
    } user_ctrl;
    gMonDisplay_t  display;
} gardenMonitor_t;

#ifdef __cplusplus
}
#endif
#endif // end of STATION_MONITOR_H
