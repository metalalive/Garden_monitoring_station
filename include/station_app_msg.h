#ifndef STATION_APPMSG_H
#define STATION_APPMSG_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus  staAppMsgInit(void);

gMonStatus  staAppMsgDeinit(void);

gMonStatus  staAddRecordToAppMsg(gmonSensorRecord_t  *new_record);

gmonStr_t*  staGetAppMsgOutflight(void);

gmonStr_t*  staGetAppMsgInflight(void);

gMonStatus  staRefreshAppMsgOutflight(void);

gMonStatus  staDecodeAppMsgInflight(gardenMonitor_t *gmon);

//// gMonStatus  staRefreshParamFromAppMsg(void);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_APPMSG_H
