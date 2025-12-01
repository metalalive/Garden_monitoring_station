#ifndef STATION_NETWORK_H
#define STATION_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // abstract low-level connection handle object
    void *lowlvl;
    // interval in milliseconds for network handling task
    unsigned int interval_ms;
    // timeout in milliseconds for reading control commands from remote user
    unsigned int read_timeout_ms;
    struct {
        gMonStatus sent;
        gMonStatus recv;
    } status;
} gMonNet_t;

gMonStatus stationNetConnInit(gMonNet_t *);
gMonStatus stationNetConnDeinit(gMonNet_t *);

gMonStatus stationNetConnEstablish(gMonNet_t *);
gMonStatus stationNetConnClose(gMonNet_t *);

gMonStatus stationNetConnSend(gMonNet_t *, gmonStr_t *app_msg);
gMonStatus stationNetConnRecv(gMonNet_t *, gmonStr_t *app_msg);

gMonStatus staSetNetConnTaskInterval(gMonNet_t *, unsigned int new_interval);

void stationNetConnHandlerTaskFn(void *params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_NETWORK_H
