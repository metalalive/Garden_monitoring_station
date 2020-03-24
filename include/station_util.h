#ifndef STATION_UTIL_H
#define STATION_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define  GMON_NUM_MILLISECONDS_PER_TICK  (GMON_SYS_TICK_RATE_HZ/1000)

#define  GMON_NUM_TICKS_PER_DAY          (GMON_NUM_MILLISECONDS_PER_DAY / GMON_NUM_MILLISECONDS_PER_TICK)

#define  GMON_NUMTOCHAR(x)  ('0' + (x))

// get current number of time ticks per day
unsigned int stationGetTicksPerDay(void);
// get number of days since the target hardware platform started working
unsigned int stationGetDays(void);

void  staReverseString(unsigned char *str, unsigned int sz);

unsigned int staCvtFloatToStr(unsigned char *outstr, float num, unsigned short precision);

gMonStatus   staChkIntFromStr(unsigned char *str, size_t sz);

int          staCvtIntFromStr(unsigned char *str, size_t sz);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_UTIL_H
