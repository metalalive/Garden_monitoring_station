#ifndef STATION_APPMSG_H
#define STATION_APPMSG_H

#ifdef __cplusplus
extern "C" {
#endif

#define GMON_APPMSG_DATA_NAME_SOILMOIST "soilmoist"
#define GMON_APPMSG_DATA_NAME_AIRTEMP   "airtemp"
#define GMON_APPMSG_DATA_NAME_LIGHT     "light"

#define GMON_NUM_JSON_TOKEN_DECODE 84

typedef struct {
    gmonStr_t *msg;
    gMonStatus status;
} gmonAppMsgOutflightResult_t;

// Using a sufficiently large fixed buffer for incoming control JSON messages.
// 384 bytes should be ample to accommodate various configuration updates.
#define staAppMsgInflightCalcRequiredBufSz() (unsigned short)520

gMonStatus staAppMsgInit(gardenMonitor_t *);
gMonStatus staAppMsgDeinit(gardenMonitor_t *);

gMonStatus staAppMsgOutResetAllRecords(gardenMonitor_t *);
gMonStatus staAppMsgReallocBuffer(gardenMonitor_t *);

gMonStatus
staAppMsgSerializeAppendStr(unsigned char **buf_ptr, unsigned short *remaining_len, const char *str);
gMonStatus staAppMsgSerializeUInt(
    unsigned char **buf_ptr, unsigned short *remaining_len, unsigned int val, unsigned int max_nbytes_used
);
gMonStatus staAppMsgSerializeFloat(
    unsigned char **buf_ptr, unsigned short *remaining_len, float val, unsigned short precision,
    unsigned int max_nbytes_used
);

gmonAppMsgOutflightResult_t staGetAppMsgOutflight(gardenMonitor_t *);

gmonStr_t *staGetAppMsgInflight(gardenMonitor_t *);

gMonStatus staDecodeAppMsgInflight(gardenMonitor_t *);

gmonEvent_t *staUpdateLastRecord(gmonSensorRecord_t *, gmonEvent_t *);

void stationSensorDataLogTaskFn(void *params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_APPMSG_H
