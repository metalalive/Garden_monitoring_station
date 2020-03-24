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
    // firstly, process the integral part
    tmp = (unsigned int) num;
    while(tmp > 0) {
        *curr_out_p++ = GMON_NUMTOCHAR(tmp % 10);
        tmp /= 10;
    }
    tmp = (unsigned int)(curr_out_p - outstr); // number of chars written to the integral part
    if(tmp == 0) {
        tmp = 1;
        *curr_out_p++ = GMON_NUMTOCHAR(0);
    } else {
        staReverseString(outstr, tmp);
    }
    num_chr += tmp;
    // secondly, check if any digit in the fraction part to be processed.
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


gMonStatus   staChkIntFromStr(unsigned char *str, size_t sz)
{
    const uint8_t  base = 10;
    uint8_t  diff = 0;
    size_t   idx = 0;
    gMonStatus status = GMON_RESP_OK;
    if(str == NULL || sz == 0) {
        return status;
    }
    if(str[0] == '-') {
        str++;  sz--;
    }
    for(idx = 0; idx < sz; idx++) {
        diff = str[idx] - '0';
        if(diff >= base) {
            status = GMON_RESP_MALFORMED_DATA;
            break;
        }
    } // end of for loop
    return status;
} // end of staChkIntFromStr


int   staCvtIntFromStr(unsigned char *str, size_t sz)
{
    const uint8_t  base = 10;
    int      out = 0;
    size_t   idx = 0;
    uint8_t  diff = 0;
    uint8_t  negate = 1;
    if(str == NULL || sz == 0) {
        return out;
    }
    if(str[0] == '-') {
        str++;  sz--;
        negate = -1;
    }
    for(idx = 0; idx < sz; idx++) {
        diff = str[idx] - '0';
        if(diff >= base) {
            out = -1;
            break;
        } else {
            out = out * base + diff;
        }
    } // end of for loop
    out = out * negate;
    return out;
} // end of staCvtIntFromStr

