#ifndef STATION_IO_H
#define STATION_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#define GMON_NUM_SENSOR_EVENTS (GMON_CFG_NUM_SENSOR_RECORDS_KEEP * 3)

gMonStatus stationIOinit(gardenMonitor_t *);
gMonStatus stationIOdeinit(gardenMonitor_t *);

gmonEvent_t *staAllocSensorEvent(gMonEvtPool_t *, gmonEventType_t, unsigned char num_sensors);
gMonStatus   staFreeSensorEvent(gMonEvtPool_t *, gmonEvent_t *);
gMonStatus   staCpySensorEvent(gmonEvent_t *dst, gmonEvent_t *src);
gMonStatus   staNotifyOthersWithEvent(gardenMonitor_t *, gmonEvent_t *, uint32_t block_time);

gMonStatus staSensorInitSoilMoist(gMonSensor_t *);
gMonStatus staSensorDeInitSoilMoist(gMonSensor_t *);
gMonStatus staSensorReadSoilMoist(gMonSensor_t *, gmonSensorSample_t *);

gMonStatus staSensorInitAirTemp(gMonSensor_t *);
gMonStatus staSensorDeInitAirTemp(gMonSensor_t *);
gMonStatus staSensorReadAirTemp(gMonSensor_t *, gmonSensorSample_t *);

gMonStatus staSensorInitLight(gMonSensor_t *);
gMonStatus staSensorDeInitLight(gMonSensor_t *);
gMonStatus staSensorReadLight(gMonSensor_t *, gmonSensorSample_t *);

gMonStatus
staSensorDetectNoise(float threshold, const gmonSensorSample_t *sensorsamples, unsigned char num_items);

// device-specific functions
gMonStatus staActuatorInitPump(gMonActuator_t *);
gMonStatus staActuatorDeinitPump(void);
gMonStatus staActuatorTrigPump(gMonActuator_t *, unsigned int soil_moist, gMonSensor_t *);

gMonStatus staActuatorInitFan(gMonActuator_t *);
gMonStatus staActuatorDeinitFan(void);
gMonStatus staActuatorTrigFan(gMonActuator_t *, float air_temp, gMonSensor_t *);

gMonStatus staActuatorInitBulb(gMonActuator_t *);
gMonStatus staActuatorDeinitBulb(void);
gMonStatus staActuatorTrigBulb(gMonActuator_t *, unsigned int lightness, gMonSensor_t *);

// generic functions to init device
gMonStatus staActuatorInitGenericPump(gMonActuator_t *);
gMonStatus staActuatorDeinitGenericPump(void);

gMonStatus staActuatorInitGenericFan(gMonActuator_t *);
gMonStatus staActuatorDeinitGenericFan(void);

gMonStatus staActuatorInitGenericBulb(gMonActuator_t *);
gMonStatus staActuatorDeinitGenericBulb(void);

gMonActuatorStatus staActuatorMeasureWorkingTime(gMonActuator_t *, unsigned int time_elapsed_ms);

gMonStatus staSetDefaultSensorReadInterval(gardenMonitor_t *, unsigned int new_interval);
gMonStatus staSetTrigThresholdPump(gMonActuator_t *, unsigned int new_val);
gMonStatus staSetTrigThresholdFan(gMonActuator_t *, unsigned int new_val);
gMonStatus staSetTrigThresholdBulb(gMonActuator_t *, unsigned int new_val);

gMonStatus staPauseWorkingActuators(gardenMonitor_t *);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_IO_H
