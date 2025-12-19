#include "station_include.h"

unsigned int staAbsInt(int a) {
    int mask = a >> (sizeof(int) * 8 - 1);
    int b = (a ^ mask) + (mask & 1);
    return (unsigned int)b;
}

void staSetBitFlag(unsigned char *list, unsigned short idx, char value) {
    if (list == NULL)
        return;
    unsigned short byte_idx = idx >> 3; // divide by 8
    unsigned char  bit_pos = idx & 0x7; // modulo by 8
    if (value) {
        list[byte_idx] |= (1 << bit_pos);
    } else {
        list[byte_idx] &= ~(1 << bit_pos);
    }
}

char staGetBitFlag(unsigned char *list, unsigned short idx) {
    if (list == NULL)
        return 0;
    unsigned short byte_idx = idx >> 3; // divide by 8
    unsigned char  bit_pos = idx & 0x7; // modulo by 8
    char           out = list[byte_idx] >> bit_pos;
    return out & 0x1;
}

static void staRefreshTimeTicks(gmonTick_t *tick_info) {
    unsigned int curr_tick = stationSysGetTickCount();
    // automatically handle number wrapping around from UINT_MAX to 0
    unsigned int diff = curr_tick - tick_info->last_read;
    tick_info->ticks_per_day += diff;
    if (tick_info->ticks_per_day >= GMON_NUM_TICKS_PER_DAY) {
        tick_info->ticks_per_day = tick_info->ticks_per_day % GMON_NUM_TICKS_PER_DAY;
        tick_info->days++;
    }
    tick_info->last_read = curr_tick;
}

unsigned int stationGetTicksPerDay(gmonTick_t *tick_info) {
    staRefreshTimeTicks(tick_info);
    return tick_info->ticks_per_day;
}

unsigned int stationGetDays(gmonTick_t *tick_info) {
    staRefreshTimeTicks(tick_info);
    return tick_info->days;
}

// Hoare Partition Scheme implementation,
// partition given integer array with given pivot
unsigned short staPartitionIntArray(unsigned int *list, unsigned short len) {
    if (len < 2)  // for both empty list and single-element array
        return 0; // always point to first item
    unsigned short lo = 0, hi = len - 1;
    unsigned int   pivot = list[0];
    while (1) {
        while (pivot > list[lo])
            lo++;
        while (pivot < list[hi])
            hi--;
        if (lo >= hi)
            break;
        list[lo] = list[lo] ^ list[hi];
        list[hi] = list[hi] ^ list[lo];
        list[lo] = list[lo] ^ list[hi];
        lo++;
        hi--;
    }
    // index to final item of left-end partition
    return hi;
}

// get Kth. largest integer
unsigned int staQuickSelect(unsigned int *list, unsigned short len, unsigned short k) {
    if (list == NULL || len == 0 || (k >= len))
        return 0;
    unsigned short lo = 0, hi = len - 1;
    while (lo < hi) {
        unsigned short rel_partition_idx = staPartitionIntArray(&list[lo], hi - lo + 1);
        unsigned short lowpart_idx = lo + rel_partition_idx;
        if (k > lowpart_idx) {
            lo = lowpart_idx + 1;
        } else {
            hi = lowpart_idx;
        }
    }
    return list[lo]; // lo == hi
}

unsigned int staFindMedian(unsigned int *list, unsigned short len) {
    if (list == NULL || len == 0)
        return 0;
    unsigned short half_idx = len >> 1;
    unsigned int   out = staQuickSelect(list, len, half_idx);
    if ((len & 0x1) == 0x0) { // even
        unsigned int mid2 = staQuickSelect(list, len, half_idx - 1);
        out = (out + mid2) >> 1;
    }
    return out;
}

// Median Absolute Deviation (MAD) implementation
unsigned int staMedianAbsDeviation(unsigned int median, unsigned int *list, unsigned short len) {
    if (median == 0 || list == NULL || len == 0)
        return 0;
    unsigned int v[len];
    for (unsigned short idx = 0; idx < len; idx++) {
        int diff = list[idx] - median;
        v[idx] = staAbsInt(diff);
    }
    return staFindMedian(v, len);
}

int staExpMovingAvg(int new, int old, unsigned char lambda) {
    int out = lambda * new + (100 - lambda) * old;
    return out / 100;
}

void staReverseString(unsigned char *str, unsigned int sz) {
    unsigned int idx = 0, jdx = 0;
    if (str != NULL && sz > 0) {
        for (idx = 0, jdx = sz - 1; idx < jdx; idx++, jdx--) {
            str[idx] = str[idx] ^ str[jdx];
            str[jdx] = str[jdx] ^ str[idx];
            str[idx] = str[idx] ^ str[jdx];
        }
    }
}

unsigned int staCvtFloatToStr(unsigned char *outstr, float num, unsigned short precision) {
    unsigned char *curr_out_p;
    unsigned int   tmp = 0, num_chr = 0;
    if (num < 0.f) {
        *outstr++ = '-';
        num_chr = 1;
        num = num * -1;
    }
    curr_out_p = outstr;
    // firstly, process the integral part
    tmp = (unsigned int)num;
    while (tmp > 0) {
        *curr_out_p++ = GMON_NUMTOCHAR(tmp % 10);
        tmp /= 10;
    }
    tmp = (unsigned int)(curr_out_p - outstr); // number of chars written to the integral part
    if (tmp == 0) {
        tmp = 1;
        *curr_out_p++ = GMON_NUMTOCHAR(0);
    } else {
        staReverseString(outstr, tmp);
    }
    num_chr += tmp;
    // secondly, check if any digit in the fraction part to be processed.
    tmp = (unsigned int)num;
    num = num - (float)tmp;
    if (num > 0.f && precision > 0) {
        *curr_out_p++ = '.';
        num_chr++;
        while (precision-- > 0) {
            num *= 10;
            tmp = (unsigned int)num;
            *curr_out_p++ = GMON_NUMTOCHAR(tmp);
            num = num - (float)tmp;
            num_chr++;
        }
    }
    return num_chr;
} // end of staCvtFloatToStr

gMonStatus staChkIntFromStr(unsigned char *str, size_t sz) {
    const uint8_t base = 10;
    uint8_t       diff = 0;
    size_t        idx = 0;
    gMonStatus    status = GMON_RESP_OK;
    if (str == NULL || sz == 0) {
        return status;
    }
    if (str[0] == '-') {
        str++;
        sz--;
    }
    for (idx = 0; idx < sz; idx++) {
        diff = str[idx] - '0';
        if (diff >= base) {
            status = GMON_RESP_MALFORMED_DATA;
            break;
        }
    }
    return status;
}

// TODO, consider to reuse `iESPparseFirstNumFromStr` from ESP-AT-parser
int staCvtIntFromStr(unsigned char *str, size_t sz) {
    const uint8_t base = 10;
    int           out = 0, negate = 1;
    size_t        idx = 0;
    uint8_t       diff = 0;
    if (str == NULL || sz == 0) {
        return out;
    }
    if (str[0] == '-') {
        str++;
        sz--;
        negate = -1;
    }
    for (idx = 0; idx < sz; idx++) {
        diff = str[idx] - '0';
        if (diff >= base) {
            out = -1;
            break;
        } else {
            out = out * base + diff;
        }
    } // end of for loop
    // Only apply negation if conversion was successful (out is not -1 indicating an error)
    if (out != -1) {
        out = out * negate;
    }
    return out;
}
