#ifndef STATION_IO_H
#define STATION_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#define GMON_NUM_SENSOR_EVENTS \
    ((GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP + GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP + \
      GMON_CFG_NUM_LIGHT_SENSOR_RECORDS_KEEP) \
     << 1)

typedef struct {
    gmonSensorSample_t *entries;
    unsigned short      total_nbytes;
} gmonSensorSamples_t;

gMonStatus stationIOinit(gardenMonitor_t *);
gMonStatus stationIOdeinit(gardenMonitor_t *);

gmonSensorSamples_t staAllocSensorSampleBuffer(gmonSensorSamples_t, gMonSensorMeta_t *, gmonSensorDataType_t);

gmonEvent_t *staAllocSensorEvent(gMonEvtPool_t *, gmonEventType_t, unsigned char num_sensors);
gMonStatus   staFreeSensorEvent(gMonEvtPool_t *, gmonEvent_t *);
gMonStatus   staCpySensorEvent(gmonEvent_t *dst, gmonEvent_t *src);
gMonStatus   staNotifyOthersWithEvent(gardenMonitor_t *, gmonEvent_t *, uint32_t block_time);

gMonStatus staSensorInitSoilMoist(gMonSoilSensorMeta_t *);
gMonStatus staSensorDeInitSoilMoist(gMonSoilSensorMeta_t *);
gMonStatus staSensorReadSoilMoist(gMonSoilSensorMeta_t *, gmonSensorSample_t *);
gMonStatus staSetNumSoilSensor(gMonSensorMeta_t *, unsigned char new_val);
gMonStatus staSetNumResamplesSoilSensor(gMonSensorMeta_t *, unsigned char new_val);

gMonStatus staSensorInitAirTemp(gMonSensorMeta_t *);
gMonStatus staSensorDeInitAirTemp(gMonSensorMeta_t *);
gMonStatus staSensorReadAirTemp(gMonSensorMeta_t *, gmonSensorSample_t *);
gMonStatus staSetNumAirSensor(gMonSensorMeta_t *, unsigned char new_val);
gMonStatus staSetNumResamplesAirSensor(gMonSensorMeta_t *, unsigned char new_val);

gMonStatus staSensorInitLight(gMonSensorMeta_t *);
gMonStatus staSensorDeInitLight(gMonSensorMeta_t *);
gMonStatus staSensorReadLight(gMonSensorMeta_t *, gmonSensorSample_t *);
gMonStatus staSetNumLightSensor(gMonSensorMeta_t *, unsigned char new_val);
gMonStatus staSetNumResamplesLightSensor(gMonSensorMeta_t *, unsigned char new_val);

gMonStatus staSensorSetReadInterval(gMonSensorMeta_t *, unsigned int new_val);
gMonStatus staSensorSetOutlierThreshold(gMonSensorMeta_t *, float new_val);
gMonStatus staSensorSetMinMAD(gMonSensorMeta_t *, float new_val);

// currently the 3 functions below are implemented only for soil sensor polling rate
gMonStatus   staSensorFastPollToggle(gMonSoilSensorMeta_t *, gMonActuator_t *);
gMonStatus   staSensorRefreshFastPollRatio(gMonSoilSensorMeta_t *);
unsigned int staSensorReadInterval(gMonSoilSensorMeta_t *);
char         staSensorPollEnabled(gMonSoilSensorMeta_t *, unsigned short idx);

gMonStatus staSensorDetectNoise(gMonSensorMeta_t *, gmonSensorSample_t *);
gMonStatus staSensorSampleToEvent(gmonEvent_t *, gmonSensorSample_t *);

// ------ actuators ------
gMonStatus staActuatorInitPump(gMonActuator_t *);
gMonStatus staActuatorDeinitPump(void);
gMonStatus staActuatorTrigPump(gMonActuator_t *, gmonEvent_t *, gMonSoilSensorMeta_t *);

gMonStatus staActuatorInitFan(gMonActuator_t *);
gMonStatus staActuatorDeinitFan(void);
gMonStatus staActuatorTrigFan(gMonActuator_t *, gmonEvent_t *, gMonSensorMeta_t *);

gMonStatus staActuatorInitBulb(gMonActuator_t *);
gMonStatus staActuatorDeinitBulb(void);
gMonStatus staActuatorTrigBulb(gMonActuator_t *, gmonEvent_t *, gMonSensorMeta_t *);

// generic functions to init device
gMonStatus staActuatorInitGenericPump(gMonActuator_t *);
gMonStatus staActuatorDeinitGenericPump(void);

gMonStatus staActuatorInitGenericFan(gMonActuator_t *);
gMonStatus staActuatorDeinitGenericFan(void);

gMonStatus staActuatorInitGenericBulb(gMonActuator_t *);
gMonStatus staActuatorDeinitGenericBulb(void);

gMonStatus staActuatorAggregateU32(gmonEvent_t *, gMonActuator_t *, int *value);
gMonStatus staActuatorAggregateAirCond(gmonEvent_t *, gMonActuator_t *, int *value);

gMonActuatorStatus staActuatorMeasureWorkingTime(gMonActuator_t *, unsigned int time_elapsed_ms);

gMonStatus staSetDefaultSensorReadInterval(gardenMonitor_t *, unsigned int new_interval);
gMonStatus staSetTrigThresholdPump(gMonActuator_t *, unsigned int new_val);
gMonStatus staSetTrigThresholdFan(gMonActuator_t *, unsigned int new_val);
gMonStatus staSetTrigThresholdBulb(gMonActuator_t *, unsigned int new_val);

gMonStatus staPauseWorkingActuators(gardenMonitor_t *);
gMonStatus staEmergencyShutdownAllActuators(gardenMonitor_t *);
gMonStatus staTurnOffActuator(gMonActuator_t *);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_IO_H
