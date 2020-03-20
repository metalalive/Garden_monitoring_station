#ifndef STATION_NETWORK_H
#define STATION_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus  stationNetConnInit(void **netconn);

gMonStatus  stationNetConnDeinit(void *netconn);

gMonStatus  stationNetConnEstablish(void *netconn);

gMonStatus  stationNetConnClose(void *netconn);

gMonStatus  stationNetConnSend(void *netconn, gmonStr_t *app_msg);

gMonStatus  stationNetConnRecv(void *netconn, gmonStr_t *app_msg, int timeout_ms);


void  stationNetConnHandlerTaskFn(void* params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_NETWORK_H
