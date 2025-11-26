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
gMonStatus  staActuatorInitPump(gMonActuator_t *);
gMonStatus  staActuatorDeinitPump(void);
gMonStatus  staActuatorTrigPump(gMonActuator_t *, unsigned int soil_moist, gMonSensor_t *);

gMonStatus  staActuatorInitFan(gMonActuator_t *);
gMonStatus  staActuatorDeinitFan(void);
gMonStatus  staActuatorTrigFan(gMonActuator_t *, float air_temp, gMonSensor_t *);

gMonStatus  staActuatorInitBulb(gMonActuator_t *);
gMonStatus  staActuatorDeinitBulb(void);
gMonStatus  staActuatorTrigBulb(gMonActuator_t *, unsigned int lightness, gMonSensor_t *);

gMonStatus  staDisplayDevInit(void);
gMonStatus  staDisplayDevDeInit(void);
gMonStatus  staDisplayRefreshScreen(void);
unsigned short  staDisplayDevGetScreenWidth(void);
unsigned short  staDisplayDevGetScreenHeight(void);
gMonStatus  staDiplayDevPrintString(gmonPrintInfo_t *printinfo);
gMonStatus  staDiplayDevPrintCustomPattern(gmonPrintInfo_t *printinfo); // TODO

// generic functions to init device
gMonStatus  staActuatorInitGenericPump(gMonActuator_t *);
gMonStatus  staActuatorDeinitGenericPump(void);

gMonStatus  staActuatorInitGenericFan(gMonActuator_t *);
gMonStatus  staActuatorDeinitGenericFan(void);

gMonStatus  staActuatorInitGenericBulb(gMonActuator_t *);
gMonStatus  staActuatorDeinitGenericBulb(void);

gMonStatus  staDisplayInit(gardenMonitor_t *);
gMonStatus  staDisplayDeInit(gardenMonitor_t *);
void        staUpdatePrintStrSensorData(gardenMonitor_t  *, gmonEvent_t *);
void        staUpdatePrintStrActuatorStatus(gardenMonitor_t  *);
void        staUpdatePrintStrThreshold(gardenMonitor_t *);

void  staUpdatePrintStrNetConn(gardenMonitor_t *);

gMonActuatorStatus  staActuatorMeasureWorkingTime(gMonActuator_t *, unsigned int time_elapsed_ms);

gMonStatus  staSetDefaultSensorReadInterval(gardenMonitor_t *, unsigned int new_interval);
gMonStatus  staSetTrigThresholdPump(gMonActuator_t *, unsigned int new_val);
gMonStatus  staSetTrigThresholdFan(gMonActuator_t *, unsigned int new_val);
gMonStatus  staSetTrigThresholdBulb(gMonActuator_t *, unsigned int new_val);

gMonStatus  staPauseWorkingActuators(gardenMonitor_t *);

void  stationDisplayTaskFn(void* params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_IO_H
