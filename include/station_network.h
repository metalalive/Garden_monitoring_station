#ifndef STATION_NETWORK_H
#define STATION_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus  stationNetConnInit(gardenMonitor_t *gmon);

gMonStatus  stationNetConnDeinit(void *connobj);

gMonStatus  stationNetConnEstablish(void *connobj);

gMonStatus  stationNetConnClose(void *connobj);

gMonStatus  stationNetConnSend(void *connobj, gmonStr_t *app_msg);

gMonStatus  stationNetConnRecv(void *connobj, gmonStr_t *app_msg, int timeout_ms);

gMonStatus  staSetNetConnTaskInterval(gardenMonitor_t  *gmon, unsigned int new_interval);

void  stationNetConnHandlerTaskFn(void* params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_NETWORK_H
