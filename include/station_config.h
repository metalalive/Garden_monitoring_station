#ifndef STATION_CONFIG_H
#define STATION_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// skip platform initialization code if the third-party software you are using will do the job instead
#define  GMON_CFG_SKIP_PLATFORM_INIT
// number of records read from sensors temporarily kept in the platform before they're sent out through internet
// (e.g. might be remote MQTT broker & another backend server waiting for these data from this station)
#define  GMON_CFG_NUM_SENSOR_RECORDS_KEEP  5
// Time interval (in milliseconds) to read all data from snesors.
#define  GMON_CFG_SENSOR_READ_INTERVAL_MS  2000
// Time interval (in milliseconds) to establish network connection for user/senser data exchange.
#define  GMON_CFG_NETCONN_START_INTERVAL_MS   60000  // 60 seconds

// enable input sensors or output device if any kind of sensor is applied to user's garden
#define  GMON_CFG_ENABLE_SENSOR_SOIL_MOIST
#define  GMON_CFG_ENABLE_SENSOR_AIR_TEMP
#define  GMON_CFG_ENABLE_LIGHT_SENSOR
#define  GMON_CFG_ENABLE_OUTDEV_PUMP
#define  GMON_CFG_ENABLE_OUTDEV_FAN
#define  GMON_CFG_ENABLE_OUTDEV_BULB
#define  GMON_CFG_ENABLE_DISPLAY

#define  GMON_CFG_OUTDEV_TRIG_THRESHOLD_PUMP  920

#define  GMON_CFG_OUTDEV_TRIG_THRESHOLD_FAN   26
#define  GMON_CFG_OUTDEV_MAX_WORKTIME_FAN     (5300)
#define  GMON_CFG_OUTDEV_MIN_RESTTIME_FAN     (2100)

#define  GMON_CFG_OUTDEV_TRIG_THRESHOLD_BULB  70
#define  GMON_CFG_OUTDEV_MAX_WORKTIME_BULB    (7100)
#define  GMON_CFG_OUTDEV_MIN_RESTTIME_BULB    (5700)

#define  GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS  (300000)


#ifdef __cplusplus
}
#endif
#endif // end of STATION_CONFIG_H
