#ifndef STATION_APPMSG_H
#define STATION_APPMSG_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus  staAppMsgInit(gardenMonitor_t *);

gMonStatus  staAppMsgDeinit(gardenMonitor_t *);

gmonStr_t*  staGetAppMsgOutflight(gardenMonitor_t *);

gmonStr_t*  staGetAppMsgInflight(gardenMonitor_t *);

gMonStatus  staDecodeAppMsgInflight(gardenMonitor_t *);

gmonSensorRecord_t staUpdateLastRecord(gmonSensorRecord_t *, gmonEvent_t *);

void  stationSensorDataAggregatorTaskFn(void* params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_APPMSG_H
