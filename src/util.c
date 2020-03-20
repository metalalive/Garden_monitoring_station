#include "station_include.h"

static unsigned int  gmon_daylight_tracker_ticks_per_day = 0;
static unsigned int  gmon_daylight_tracker_days = 0;
static unsigned int  gmon_last_tick_read = 0;

static void staRefreshTimeTicks(void)
{
    unsigned int curr_tick = 0;
    unsigned int diff = 0;
    curr_tick = stationSysGetTickCount();
    diff = curr_tick - gmon_last_tick_read;
    gmon_daylight_tracker_ticks_per_day += diff;
    if(gmon_daylight_tracker_ticks_per_day >= GMON_NUM_TICKS_PER_DAY) {
        gmon_daylight_tracker_ticks_per_day = gmon_daylight_tracker_ticks_per_day % GMON_NUM_TICKS_PER_DAY;
        gmon_daylight_tracker_days++;
    }
    gmon_last_tick_read = curr_tick;
} // end of staRefreshTimeTicks


unsigned int stationGetTicksPerDay(void)
{
    staRefreshTimeTicks();
    return gmon_daylight_tracker_ticks_per_day;
} // end of stationGetTicksPerDay

unsigned int stationGetDays(void)
{
    staRefreshTimeTicks();
    return gmon_daylight_tracker_days;
} // end of stationGetDays


void  staReverseString(unsigned char *str, unsigned int sz)
{
    unsigned int idx = 0, jdx = 0;
    if(str != NULL && sz > 0) {
        for(idx = 0, jdx = sz - 1; idx < jdx; idx++, jdx--) {
            str[idx] = str[idx] ^ str[jdx];
            str[jdx] = str[jdx] ^ str[idx];
            str[idx] = str[idx] ^ str[jdx];
        }
    }
} // end of staReverseString


unsigned int staCvtFloatToStr(unsigned char *outstr, float num, unsigned short precision)
{
    unsigned char *curr_out_p;
    unsigned int   tmp = 0;
    unsigned int   num_chr = 0;
    if(num < 0.f) {
        *outstr++ = '-';
        num_chr = 1;
        num = num * -1;
    }
    curr_out_p = outstr;
    // firstly, process integral parts
    tmp = (unsigned int) num;
    while(tmp > 0) {
        *curr_out_p++ = GMON_NUMTOCHAR(tmp % 10);
        tmp /= 10;
    }
    tmp = (unsigned int)(curr_out_p - outstr); // number of chars written
    if(tmp == 0) {
        tmp = 1;
        *curr_out_p++ = GMON_NUMTOCHAR(0);
    } else {
        staReverseString(outstr, tmp);
    }
    num_chr += tmp;
    // secondly, check if any fraction parts to be processed.
    tmp = (unsigned int) num;
    num = num - (float)tmp;
    if(num > 0.f && precision > 0) {
        *curr_out_p++ = '.';
        num_chr++;
        while(precision-- > 0) {
            num *= 10;
            tmp = (unsigned int) num;
            *curr_out_p++ = GMON_NUMTOCHAR(tmp);
            num = num - (float)tmp;
            num_chr++;
        }
    }
    return num_chr;
} // end of staCvtFloatToStr


