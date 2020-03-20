#ifndef STATION_DEFAULT_CONFIG_H
#define STATION_DEFAULT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
// ------------ DO NOT modify the settings below ------------

#define  GMON_NUM_MILLISECONDS_PER_DAY   0x5265c00 // 1000 * 60 * 60 * 24


#ifndef   GMON_CFG_SENSOR_READ_INTERVAL_MS
    #define GMON_CFG_SENSOR_READ_INTERVAL_MS   10000 // 10 * 1000 ms
#elif     (GMON_CFG_SENSOR_READ_INTERVAL_MS > GMON_NUM_MILLISECONDS_PER_DAY)
    #error "GMON_CFG_SENSOR_READ_INTERVAL_MS must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#elif     (GMON_CFG_SENSOR_READ_INTERVAL_MS < 50)
    #error "GMON_CFG_SENSOR_READ_INTERVAL_MS must NOT be lesser than 50 milliseconds."
#endif // end of GMON_CFG_SENSOR_READ_INTERVAL_MS

#ifndef   GMON_CFG_NETCONN_START_INTERVAL_MS
    #define GMON_CFG_NETCONN_START_INTERVAL_MS   600000 // 10 minutes
#elif     (GMON_CFG_NETCONN_START_INTERVAL_MS > GMON_NUM_MILLISECONDS_PER_DAY)
    #error "GMON_CFG_NETCONN_START_INTERVAL_MS must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#elif     (GMON_CFG_NETCONN_START_INTERVAL_MS < 60000) // 1 minutes
    #error "GMON_CFG_NETCONN_START_INTERVAL_MS must NOT be lesser than 60 seconds."
#endif // end of GMON_CFG_NETCONN_START_INTERVAL_MS


#ifdef   GMON_CFG_ENABLE_SENSOR_SOIL_MOIST
    #define  GMON_SENSOR_INIT_FN_SOIL_MOIST()         staSensorInitSoilMoist()
    #define  GMON_SENSOR_DEINIT_FN_SOIL_MOIST()       staSensorDeInitSoilMoist()
    #define  GMON_SENSOR_READ_FN_SOIL_MOIST(readout)  staSensorReadSoilMoist((readout))
#else
    #define  GMON_SENSOR_INIT_FN_SOIL_MOIST()         GMON_RESP_OK
    #define  GMON_SENSOR_DEINIT_FN_SOIL_MOIST()       GMON_RESP_OK
    #define  GMON_SENSOR_READ_FN_SOIL_MOIST(readout)  GMON_RESP_OK
#endif // end of GMON_CFG_ENABLE_SENSOR_SOIL_MOIST

#ifdef GMON_CFG_ENABLE_SENSOR_AIR_TEMP
    #define  GMON_SENSOR_INIT_FN_AIR_TEMP()                      staSensorInitAirTemp()
    #define  GMON_SENSOR_DEINIT_FN_AIR_TEMP()                    staSensorDeInitAirTemp()
    #define  GMON_SENSOR_READ_FN_AIR_TEMP(air_temp, air_humid)   staSensorReadAirTemp((air_temp), (air_humid))
#else
    #define  GMON_SENSOR_INIT_FN_AIR_TEMP()                      GMON_RESP_OK
    #define  GMON_SENSOR_DEINIT_FN_AIR_TEMP()                    GMON_RESP_OK
    #define  GMON_SENSOR_READ_FN_AIR_TEMP(air_temp, air_humid)   GMON_RESP_OK
#endif // end of GMON_CFG_ENABLE_SENSOR_AIR_TEMP

#ifdef  GMON_CFG_ENABLE_LIGHT_SENSOR
    #define  GMON_SENSOR_INIT_FN_LIGHT()            staSensorInitLight()
    #define  GMON_SENSOR_DEINIT_FN_LIGHT()          staSensorDeInitLight()
    #define  GMON_SENSOR_READ_FN_LIGHT(lightness)   staSensorReadLight((lightness))
#else
    #define  GMON_SENSOR_INIT_FN_LIGHT()           GMON_RESP_OK
    #define  GMON_SENSOR_DEINIT_FN_LIGHT()         GMON_RESP_OK
    #define  GMON_SENSOR_READ_FN_LIGHT(lightness)  GMON_RESP_OK
#endif // end of GMON_CFG_ENABLE_LIGHT_SENSOR

#ifdef  GMON_CFG_ENABLE_OUTDEV_PUMP
    #define  GMON_OUTDEV_INIT_FN_PUMP(dev)         staOutdevInitPump((dev))
    #define  GMON_OUTDEV_DEINIT_FN_PUMP()          staOutdevDeinitPump()
    #define  GMON_OUTDEV_TRIG_FN_PUMP(dev, soil_moist)   staOutdevTrigPump((dev), (soil_moist))
#else
    #define  GMON_OUTDEV_INIT_FN_PUMP(dev)             GMON_RESP_OK
    #define  GMON_OUTDEV_DEINIT_FN_PUMP()              GMON_RESP_OK
    #define  GMON_OUTDEV_TRIG_FN_PUMP(dev, soil_moist) GMON_RESP_OK
#endif // end of  GMON_CFG_ENABLE_OUTDEV_PUMP

#ifdef  GMON_CFG_ENABLE_OUTDEV_FAN
    #define  GMON_OUTDEV_INIT_FN_FAN(dev)         staOutdevInitFan((dev))
    #define  GMON_OUTDEV_DEINIT_FN_FAN()          staOutdevDeinitFan()
    #define  GMON_OUTDEV_TRIG_FN_FAN(dev, air_temp)   staOutdevTrigFan((dev), (air_temp))
#else
    #define  GMON_OUTDEV_INIT_FN_FAN(dev)           GMON_RESP_OK
    #define  GMON_OUTDEV_DEINIT_FN_FAN()            GMON_RESP_OK
    #define  GMON_OUTDEV_TRIG_FN_FAN(dev, air_temp) GMON_RESP_OK
#endif // end of GMON_CFG_ENABLE_OUTDEV_FAN
 
#ifdef  GMON_CFG_ENABLE_OUTDEV_BULB
    #define  GMON_OUTDEV_INIT_FN_BULB(dev)         staOutdevInitBulb((dev))
    #define  GMON_OUTDEV_DEINIT_FN_BULB()          staOutdevDeinitBulb()
    #define  GMON_OUTDEV_TRIG_FN_BULB(dev, lightness)   staOutdevTrigBulb((dev), (lightness))
#else
    #define  GMON_OUTDEV_INIT_FN_BULB(dev)            GMON_RESP_OK
    #define  GMON_OUTDEV_DEINIT_FN_BULB()             GMON_RESP_OK
    #define  GMON_OUTDEV_TRIG_FN_BULB(dev, lightness) GMON_RESP_OK
#endif // end of GMON_CFG_ENABLE_OUTDEV_BULB


#define  GMON_OUTDEV_MAX_WORKTIME_PER_DAY  GMON_NUM_MILLISECONDS_PER_DAY
#define  GMON_OUTDEV_MAX_RESTTIME_PER_DAY  GMON_NUM_MILLISECONDS_PER_DAY


#ifndef   GMON_CFG_OUTDEV_TRIG_THRESHOLD_BULB
    #define  GMON_CFG_OUTDEV_TRIG_THRESHOLD_BULB  60
#endif // end of GMON_CFG_OUTDEV_TRIG_THRESHOLD_BULB

#ifndef   GMON_CFG_OUTDEV_MAX_WORKTIME_BULB
    #define  GMON_CFG_OUTDEV_MAX_WORKTIME_BULB  36000000 // 10 * 60 * 60 * 1000 ms
#elif     (GMON_CFG_OUTDEV_MAX_WORKTIME_BULB > GMON_OUTDEV_MAX_WORKTIME_PER_DAY)
    #error "GMON_CFG_OUTDEV_MAX_WORKTIME_BULB must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#endif // end of GMON_CFG_OUTDEV_MAX_WORKTIME_BULB

#ifndef   GMON_CFG_OUTDEV_MIN_RESTTIME_BULB
    #define  GMON_CFG_OUTDEV_MIN_RESTTIME_BULB  GMON_OUTDEV_MAX_RESTTIME_PER_DAY
#elif     (GMON_CFG_OUTDEV_MIN_RESTTIME_BULB > GMON_OUTDEV_MAX_RESTTIME_PER_DAY)
    #error "GMON_CFG_OUTDEV_MIN_RESTTIME_BULB must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#endif // end of GMON_CFG_OUTDEV_MIN_RESTTIME_BULB

#ifndef   GMON_CFG_OUTDEV_TRIG_THRESHOLD_PUMP
    #define  GMON_CFG_OUTDEV_TRIG_THRESHOLD_PUMP  950
#endif // end if GMON_CFG_OUTDEV_TRIG_THRESHOLD_PUMP

#ifndef   GMON_CFG_OUTDEV_MAX_WORKTIME_PUMP
    #define  GMON_CFG_OUTDEV_MAX_WORKTIME_PUMP  (4 * 1000)
#elif     (GMON_CFG_OUTDEV_MAX_WORKTIME_PUMP > GMON_OUTDEV_MAX_WORKTIME_PER_DAY)
    #error "GMON_CFG_OUTDEV_MAX_WORKTIME_PUMP must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#endif // end if GMON_CFG_OUTDEV_MAX_WORKTIME_PUMP

#ifndef   GMON_CFG_OUTDEV_MIN_RESTTIME_PUMP
    #define  GMON_CFG_OUTDEV_MIN_RESTTIME_PUMP  (1500)
#elif     (GMON_CFG_OUTDEV_MIN_RESTTIME_PUMP > GMON_OUTDEV_MAX_RESTTIME_PER_DAY)
    #error "GMON_CFG_OUTDEV_MIN_RESTTIME_PUMP must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#endif // end of GMON_CFG_OUTDEV_MIN_RESTTIME_PUMP

#ifndef   GMON_CFG_OUTDEV_TRIG_THRESHOLD_FAN
    #define  GMON_CFG_OUTDEV_TRIG_THRESHOLD_FAN  28
#endif // end if GMON_CFG_OUTDEV_TRIG_THRESHOLD_FAN

#ifndef   GMON_CFG_OUTDEV_MAX_WORKTIME_FAN
    #define  GMON_CFG_OUTDEV_MAX_WORKTIME_FAN   (11 * 1000)
#elif     (GMON_CFG_OUTDEV_MAX_WORKTIME_FAN > GMON_OUTDEV_MAX_WORKTIME_PER_DAY)
    #error "GMON_CFG_OUTDEV_MAX_WORKTIME_FAN must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#endif // end if GMON_CFG_OUTDEV_MAX_WORKTIME_FAN

#ifndef   GMON_CFG_OUTDEV_MIN_RESTTIME_FAN
    #define  GMON_CFG_OUTDEV_MIN_RESTTIME_FAN   (6 * 1000)
#elif     (GMON_CFG_OUTDEV_MIN_RESTTIME_FAN > GMON_OUTDEV_MAX_RESTTIME_PER_DAY)
    #error "GMON_CFG_OUTDEV_MIN_RESTTIME_FAN must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#endif // end of GMON_CFG_OUTDEV_MIN_RESTTIME_FAN


#ifndef  GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS
    #define GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS  (21600000) // 6 * 3600 * 1000 ms
#elif    (GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS > GMON_OUTDEV_MAX_RESTTIME_PER_DAY)
    #error "GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS must NOT be greater than 1000 * 60 * 60 * 24 milliseconds."
#endif // end of GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS


#define  GMON_CFG_MIN_NUM_SENSOR_RECORDS_KEEP  2
#define  GMON_CFG_MAX_NUM_SENSOR_RECORDS_KEEP  8

#ifndef  GMON_CFG_NUM_SENSOR_RECORDS_KEEP
    #define  GMON_CFG_NUM_SENSOR_RECORDS_KEEP  4
#elif   (GMON_CFG_NUM_SENSOR_RECORDS_KEEP < GMON_CFG_MIN_NUM_SENSOR_RECORDS_KEEP)
    #error "GMON_CFG_NUM_SENSOR_RECORDS_KEEP shouldn't be lesser than GMON_CFG_MIN_NUM_SENSOR_RECORDS_KEEP, recheck your configuration"
#elif (GMON_CFG_NUM_SENSOR_RECORDS_KEEP > GMON_CFG_MAX_NUM_SENSOR_RECORDS_KEEP)
    #error "GMON_CFG_NUM_SENSOR_RECORDS_KEEP shouldn't be greater than GMON_CFG_MAX_NUM_SENSOR_RECORDS_KEEP, recheck your configuration"
#endif

#define  GMON_SENSOR_READ_INTERVAL_MS_PUMP_ON  (GMON_CFG_SENSOR_READ_INTERVAL_MS <  400 ? GMON_CFG_SENSOR_READ_INTERVAL_MS: 400)
#define  GMON_SENSOR_READ_INTERVAL_MS_FAN_ON   (GMON_CFG_SENSOR_READ_INTERVAL_MS < 1500 ? GMON_CFG_SENSOR_READ_INTERVAL_MS: 1500)



#ifdef __cplusplus
}
#endif
#endif // end of STATION_DEFAULT_CONFIG_H
