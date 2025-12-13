#ifndef STATION_UTIL_H
#define STATION_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define GMON_NUM_MILLISECONDS_PER_TICK (GMON_SYS_TICK_RATE_HZ / 1000)

#define GMON_NUM_TICKS_PER_DAY (GMON_NUM_MILLISECONDS_PER_DAY / GMON_NUM_MILLISECONDS_PER_TICK)

#define GMON_NUMTOCHAR(x) ('0' + (x))

// approximate ratio convert from Standard Deviation (SD) to
// Median Absolute Deviation (MAD)
#define GMON_STATS_SD2MAD_RATIO 0.67449f

unsigned int staAbsInt(int a);

void staSetBitFlag(unsigned char *list, unsigned short idx, char value);
char staGetBitFlag(unsigned char *list, unsigned short idx);

// get current number of time ticks per day
unsigned int stationGetTicksPerDay(gmonTick_t *);
// get number of days since the target hardware platform started working
unsigned int stationGetDays(gmonTick_t *);

void staReverseString(unsigned char *str, unsigned int sz);

unsigned int staCvtFloatToStr(unsigned char *outstr, float num, unsigned short precision);

gMonStatus staChkIntFromStr(unsigned char *str, size_t sz);

int staCvtIntFromStr(unsigned char *str, size_t sz);

unsigned short staPartitionIntArray(unsigned int *list, unsigned short len);

unsigned int staQuickSelect(unsigned int *list, unsigned short len, unsigned short k);

unsigned int staFindMedian(unsigned int *list, unsigned short len);

unsigned int staMedianAbsDeviation(unsigned int median, unsigned int *list, unsigned short len);

int staExpMovingAvg(int new, int old, unsigned char lambda);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_UTIL_H
