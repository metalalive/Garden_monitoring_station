#ifndef STATION_IO_H
#define STATION_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#define  GMON_NUM_SENSOR_EVENTS   (GMON_CFG_NUM_SENSOR_RECORDS_KEEP * 3)

typedef struct {
    uint16_t      width;
    uint16_t      height;
    const unsigned short *bitmap;
} gmonPrintFont_t;


typedef struct {
    gmonStr_t  str;
    gmonPrintFont_t  *font;
    short  posx;
    short  posy;
} gmonPrintInfo_t;


gMonStatus  stationIOinit(gardenMonitor_t *);
gMonStatus  stationIOdeinit(gardenMonitor_t *);

gmonEvent_t* staAllocSensorEvent(gardenMonitor_t *);
gMonStatus  staFreeSensorEvent(gardenMonitor_t *, gmonEvent_t *);
gMonStatus  staCpySensorEvent(gmonEvent_t  *dst, gmonEvent_t *src, size_t  sz);
gMonStatus  staNotifyOthersWithEvent(gardenMonitor_t *, gmonEvent_t *, uint32_t block_time);

gMonStatus  staSensorInitSoilMoist(void);
gMonStatus  staSensorDeInitSoilMoist(void);
gMonStatus  staSensorReadSoilMoist(unsigned int *out);

gMonStatus  staSensorInitAirTemp(void);
gMonStatus  staSensorDeInitAirTemp(void);
gMonStatus  staSensorReadAirTemp(float *air_temp, float *air_humid);

gMonStatus  staSensorInitLight(void);
gMonStatus  staSensorDeInitLight(void);
gMonStatus  staSensorReadLight(unsigned int *out);

// device-specific functions
gMonStatus  staOutdevInitPump(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitPump(void);
gMonStatus  staOutdevTrigPump(gMonOutDev_t *, unsigned int soil_moist, gMonSensor_t *);

gMonStatus  staOutdevInitFan(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitFan(void);
gMonStatus  staOutdevTrigFan(gMonOutDev_t *, float air_temp, gMonSensor_t *);

gMonStatus  staOutdevInitBulb(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitBulb(void);
gMonStatus  staOutdevTrigBulb(gMonOutDev_t *, unsigned int lightness, gMonSensor_t *);

gMonStatus  staDisplayDevInit(void);
gMonStatus  staDisplayDevDeInit(void);
gMonStatus  staDisplayRefreshScreen(void);
unsigned short  staDisplayDevGetScreenWidth(void);
unsigned short  staDisplayDevGetScreenHeight(void);
gMonStatus  staDiplayDevPrintString(gmonPrintInfo_t *printinfo);
gMonStatus  staDiplayDevPrintCustomPattern(gmonPrintInfo_t *printinfo); // TODO

// generic functions to init device
gMonStatus  staOutdevInitGenericPump(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitGenericPump(void);

gMonStatus  staOutdevInitGenericFan(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitGenericFan(void);

gMonStatus  staOutdevInitGenericBulb(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitGenericBulb(void);

gMonStatus  staDisplayInit(gardenMonitor_t *);
gMonStatus  staDisplayDeInit(gardenMonitor_t *);
void        staUpdatePrintStrSensorData(gardenMonitor_t  *, gmonEvent_t *);
void        staUpdatePrintStrOutDevStatus(gardenMonitor_t  *);
void        staUpdatePrintStrThreshold(gardenMonitor_t *);

void  staUpdatePrintStrNetConn(gardenMonitor_t *);

gMonOutDevStatus  staOutDevMeasureWorkingTime(gMonOutDev_t *dev, unsigned int time_elapsed_ms);

gMonStatus  staSetDefaultSensorReadInterval(gardenMonitor_t *gmon, unsigned int new_interval);
gMonStatus  staSetTrigThresholdPump(gMonOutDev_t *dev, unsigned int new_val);
gMonStatus  staSetTrigThresholdFan(gMonOutDev_t *dev, unsigned int new_val);
gMonStatus  staSetTrigThresholdBulb(gMonOutDev_t *dev, unsigned int new_val);

gMonStatus  staPauseWorkingRealtimeOutdevs(gardenMonitor_t *gmon);

void  stationDisplayTaskFn(void* params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_IO_H
