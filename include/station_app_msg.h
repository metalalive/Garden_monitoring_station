#ifndef STATION_APPMSG_H
#define STATION_APPMSG_H

#ifdef __cplusplus
extern "C" {
#endif

#define GMON_APPMSG_DATA_NAME_SOILMOIST "soilmoist"
#define GMON_APPMSG_DATA_NAME_AIRTEMP   "airtemp"
#define GMON_APPMSG_DATA_NAME_LIGHT     "light"

#define GMON_NUM_JSON_TOKEN_DECODE 84

// Using a sufficiently large fixed buffer for incoming control JSON messages.
// 384 bytes should be ample to accommodate various configuration updates.
#define staAppMsgInflightCalcRequiredBufSz() (unsigned short)520

gMonStatus staAppMsgInit(gardenMonitor_t *);
gMonStatus staAppMsgDeinit(gardenMonitor_t *);

gmonStr_t *staGetAppMsgOutflight(gardenMonitor_t *);
gmonStr_t *staGetAppMsgInflight(gardenMonitor_t *);

gMonStatus staDecodeAppMsgInflight(gardenMonitor_t *);

gmonSensorRecord_t staUpdateLastRecord(gmonSensorRecord_t *, gmonEvent_t *);

void stationSensorDataAggregatorTaskFn(void *params);

unsigned short staAppMsgOutflightCalcRequiredBufSz(void);

void staParseFixedPartsOutAppMsg(gmonStr_t *outmsg);
void staAppMsgOutResetAllRecords(gardenMonitor_t *);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_APPMSG_H
