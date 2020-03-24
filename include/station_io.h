#ifndef STATION_IO_H
#define STATION_IO_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus  stationIOinit(gardenMonitor_t *gmon);
gMonStatus  stationIOdeinit(void);

gMonStatus  staFreeSensorRecord(gmonSensorRecord_t* record);

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
gMonStatus  staOutdevTrigPump(gMonOutDev_t *dev, unsigned int soil_moist);

gMonStatus  staOutdevInitFan(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitFan(void);
gMonStatus  staOutdevTrigFan(gMonOutDev_t *dev, float air_temp);

gMonStatus  staOutdevInitBulb(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitBulb(void);
gMonStatus  staOutdevTrigBulb(gMonOutDev_t *dev, unsigned int lightness);

// generic functions to init device
gMonStatus  staOutdevInitGenericPump(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitGenericPump(void);

gMonStatus  staOutdevInitGenericFan(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitGenericFan(void);

gMonStatus  staOutdevInitGenericBulb(gMonOutDev_t *dev);
gMonStatus  staOutdevDeinitGenericBulb(void);

gMonOutDevStatus  staOutDevMeasureWorkingTime(gMonOutDev_t *dev);

gMonStatus  staSetDefaultSensorReadInterval(gardenMonitor_t *gmon, unsigned int new_interval);
gMonStatus  staSetTrigThresholdPump(gMonOutDev_t *dev, unsigned int new_val);
gMonStatus  staSetTrigThresholdFan(gMonOutDev_t *dev, unsigned int new_val);
gMonStatus  staSetTrigThresholdBulb(gMonOutDev_t *dev, unsigned int new_val);


void  stationSensorReaderTaskFn(void* params);

void  stationOutDevCtrlTaskFn(void* params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_IO_H
