#ifndef STATION_APPMSG_H
#define STATION_APPMSG_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus  staAppMsgInit(void);

gMonStatus  staAppMsgDeinit(void);

gmonStr_t*  staGetAppMsgOutflight(void);

gmonStr_t*  staGetAppMsgInflight(void);

gMonStatus  staRefreshAppMsgOutflight(gardenMonitor_t *);

gMonStatus  staDecodeAppMsgInflight(gardenMonitor_t *);

void  stationSensorDataAggregatorTaskFn(void* params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_APPMSG_H
