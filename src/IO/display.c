#include "station_include.h"

// This application prints multiple lines of follwoing strings :
// * data actually read from sensor
//       [Last Record]: Soil moisture: 1024(dry/moist/wet), Air Temp: 34.5'C, Air Humidity: 101, Lightness: 1001.
// * threshold of each sensor to trigger output control device
//       [Thresold]: Soil moisture: 1001, Air Temp: 30.5 C, Lightness: 0085.
// * output control device status
//       [Output Device] Pump: (ON/OFF), Cooling Fan: (ON/OFF), Bulb: (ON/OFF)
// * network status
//       [Network]: records log sent: (succeed/failed), user control received: (succeed/skipped/failed),
//                  Last Update at 23:59:58, 99999 day(s) after system boot.
#define  GMON_PRINT_STRING_SENSOR_RECORD    (const char *)&("[Last Record]: Soil moisture: 1024(moist), Air Temp: 134.5'C, Air Humidity: 101, Lightness: 1001.")
#define  GMON_PRINT_STRING_SENSOR_THRSHOLD  (const char *)&("[Thresold]: Soil moisture: 1001, Air Temp: 130.5'C, Lightness: 0085.")
#define  GMON_PRINT_STRING_OUTDEV_STATUS    (const char *)&("[Output Device] Pump: PAUSE, Cooling Fan: PAUSE, Bulb: PAUSE.")
#define  GMON_PRINT_STRING_NETCONN_STATUS   (const char *)&("[Network]: records log sent: succeed, user control received: succeed, Last Update at 23:59:58, 99999 day(s) after system boot.")


#define  GMON_PRINT_WORDS_DISCONNECTED   "Disconneted"
#define  GMON_PRINT_WORDS_CONNECTING     "Connecting"
#define  GMON_PRINT_WORDS_SUCCEED        "Succeed"
#define  GMON_PRINT_WORDS_SKIPPED        "Skipped"
#define  GMON_PRINT_WORDS_FAILED         "Failed"
#define  GMON_PRINT_WORDS_ON             "ON"
#define  GMON_PRINT_WORDS_OFF            "OFF"
#define  GMON_PRINT_WORDS_PAUSE          "PAUSE"
#define  GMON_PRINT_WORDS_DRY            "dry"
#define  GMON_PRINT_WORDS_MOIST          "moist"
#define  GMON_PRINT_WORDS_WET            "wet"


static const short gmon_print_sensor_record_fix_content_idx[]   = {30, 4, 1, 5, 13, 5, 18, 3, 13, 4, 1, 0,};
static const short gmon_print_sensor_thrshold_fix_content_idx[] = {27, 4, 12, 5, 15, 4, 1, 0,};
static const short gmon_print_outdev_status_fix_content_idx[]   = {22, 5, 15, 5, 8, 5, 1, 0,};
static const short gmon_print_netconn_status_fix_content_idx[]  = {29, 7, 25, 7, 17, 8, 2, 5, 26, 0,};

static unsigned char *gmon_print_sensor_record_var_content_ptr[5];
static unsigned char *gmon_print_sensor_thrshold_var_content_ptr[3];
static unsigned char *gmon_print_outdev_status_var_content_ptr[3];
static unsigned char *gmon_print_netconn_status_var_content_ptr[4];

extern const unsigned short gmon_txt_font_bitmap_11x18[];

static gmonPrintFont_t   gmon_txt_font_11x18;
static gmonPrintInfo_t   gmon_print_info[GMON_DISPLAY_NUM_PRINT_STRINGS];



static void staInitPrintTxtVarPtr(gmonStr_t *str, const short *fx_content_idx, unsigned char **out)
{
    unsigned char *var_ptr = NULL;
    uint8_t  idx = 0;
    XASSERT(str != NULL);
    XASSERT(str->data != NULL);
    XASSERT(fx_content_idx != NULL);
    XASSERT(out != NULL);

    var_ptr = str->data;
    for(idx = 0; fx_content_idx[idx] != 0; idx++) {
        if((idx & 0x1) == 0x1) {
            out[(idx >> 1)] = var_ptr;
        }
        var_ptr += fx_content_idx[idx];
    } // end of for loop
    XASSERT(var_ptr == &str->data[str->len]);
} // end of staInitPrintTxtVarPtr



// process record data from sensor or output contorl device
// then feed the processed data to low-level display device
gMonStatus  staDisplayInit(gardenMonitor_t *gmon)
{
#ifdef  GMON_CFG_ENABLE_DISPLAY
    if(gmon == NULL) { return GMON_RESP_ERRARGS; }
    const char *print_txt_list[GMON_DISPLAY_NUM_PRINT_STRINGS];
    gMonDisplay_t *display = NULL;
    unsigned char  *buf = NULL;
    size_t    total_len = 0;
    uint8_t   idx = 0;

    print_txt_list[0] = GMON_PRINT_STRING_SENSOR_RECORD  ;
    print_txt_list[1] = GMON_PRINT_STRING_SENSOR_THRSHOLD;
    print_txt_list[2] = GMON_PRINT_STRING_OUTDEV_STATUS  ;
    print_txt_list[3] = GMON_PRINT_STRING_NETCONN_STATUS ;
    XASSERT(4 <= GMON_DISPLAY_NUM_PRINT_STRINGS);

    gmon_txt_font_11x18.width  = 11;
    gmon_txt_font_11x18.height = 18;
    gmon_txt_font_11x18.bitmap = gmon_txt_font_bitmap_11x18;

    display = &gmon->display;
    display->interval_ms = GMON_CFG_DISPLAY_SCREEN_REFRESH_TIME_MS;
    for(idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        display->print_str[idx].len = XSTRLEN(print_txt_list[idx]);
        total_len += display->print_str[idx].len;
    } // end of for loop
    buf = (unsigned char *) XMALLOC(sizeof(char) * total_len);
    for(idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        display->print_str[idx].data = buf;
        buf += display->print_str[idx].len;
        XMEMCPY(display->print_str[idx].data, print_txt_list[idx], display->print_str[idx].len);
        gmon_print_info[idx].str.len  = display->print_str[idx].len;
        gmon_print_info[idx].str.data = display->print_str[idx].data;
        gmon_print_info[idx].posx = 0;
        gmon_print_info[idx].posy = (gmon_txt_font_11x18.height + 2) * idx; // will be reordeed later in the task...
        gmon_print_info[idx].font = &gmon_txt_font_11x18;
    } // end of for loop
    staInitPrintTxtVarPtr(&display->print_str[0],
                          &gmon_print_sensor_record_fix_content_idx[0],
                          &gmon_print_sensor_record_var_content_ptr[0] );
    staInitPrintTxtVarPtr(&display->print_str[1],
                          &gmon_print_sensor_thrshold_fix_content_idx[0],
                          &gmon_print_sensor_thrshold_var_content_ptr[0] );
    staInitPrintTxtVarPtr(&display->print_str[2],
                          &gmon_print_outdev_status_fix_content_idx[0],
                          &gmon_print_outdev_status_var_content_ptr[0] );
    staInitPrintTxtVarPtr(&display->print_str[3],
                          &gmon_print_netconn_status_fix_content_idx[0],
                          &gmon_print_netconn_status_var_content_ptr[0] );

    staUpdatePrintStrThreshold(gmon);
    staUpdatePrintStrOutDevStatus(gmon);
    return  GMON_DISPLAY_DEV_INIT_FN();
#else
    return  GMON_RESP_SKIP;
#endif // end of GMON_CFG_ENABLE_DISPLAY
} // end of staDisplayInit


gMonStatus  staDisplayDeInit(gardenMonitor_t *gmon)
{
#ifdef  GMON_CFG_ENABLE_DISPLAY
    if(gmon == NULL) { return GMON_RESP_ERRARGS; }
    uint8_t  idx = 0;
    XMEMFREE((void *)gmon->display.print_str[0].data);
    for(idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        gmon->display.print_str[idx].data = NULL;
    } // end of for loop
    return  GMON_DISPLAY_DEV_DEINIT_FN();
#else
    return  GMON_RESP_SKIP;
#endif // end of GMON_CFG_ENABLE_DISPLAY
} // end of staDisplayDeInit




void  staUpdatePrintStrSensorData(gardenMonitor_t  *gmon, gmonEvent_t *new_evt)
{
    unsigned char  *dst_buf = NULL;
    const    char  *label_buf = NULL;
    uint8_t  num_chr = 0;

    stationSysEnterCritical();
    if (new_evt->event_type == GMON_EVENT_SOIL_MOISTURE_UPDATED) {
        dst_buf = gmon_print_sensor_record_var_content_ptr[0];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_record_fix_content_idx[1]);
        num_chr = staCvtUNumToStr(dst_buf, new_evt->data.soil_moist);
        XASSERT(num_chr <= gmon_print_sensor_record_fix_content_idx[1]);
        // describe moisture in textual way
        if(new_evt->data.soil_moist > gmon->outdev.pump.threshold) {
            label_buf = GMON_PRINT_WORDS_DRY;
        } else if(new_evt->data.soil_moist > (gmon->outdev.pump.threshold >> 1)) {
            label_buf = GMON_PRINT_WORDS_MOIST;
        } else {
            label_buf = GMON_PRINT_WORDS_WET;
        }
        dst_buf = gmon_print_sensor_record_var_content_ptr[1];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_record_fix_content_idx[3]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));
    }
    if (new_evt->event_type == GMON_EVENT_AIR_TEMP_UPDATED) {
        dst_buf = gmon_print_sensor_record_var_content_ptr[2];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_record_fix_content_idx[5]);
        num_chr = staCvtFloatToStr(dst_buf, new_evt->data.air_temp, 0x1);
        XASSERT(num_chr <= gmon_print_sensor_record_fix_content_idx[5]);
        dst_buf = gmon_print_sensor_record_var_content_ptr[3];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_record_fix_content_idx[7]);
        num_chr = staCvtFloatToStr(dst_buf, new_evt->data.air_humid, 0x1);
        XASSERT(num_chr <= gmon_print_sensor_record_fix_content_idx[7]);
    }
    if (new_evt->event_type == GMON_EVENT_LIGHTNESS_UPDATED) {
        dst_buf = gmon_print_sensor_record_var_content_ptr[4];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_record_fix_content_idx[9]);
        num_chr = staCvtUNumToStr(dst_buf, new_evt->data.lightness);
        XASSERT(num_chr <= gmon_print_sensor_record_fix_content_idx[9]);
    }
    stationSysExitCritical();
} // end of staUpdatePrintStrSensorData



void  staUpdatePrintStrThreshold(gardenMonitor_t *gmon)
{ // this should be refreshed every time after data exchange from remote backend
    unsigned char  *dst_buf = NULL;
    uint8_t  num_chr = 0;

    stationSysEnterCritical();
    {
        dst_buf = gmon_print_sensor_thrshold_var_content_ptr[0];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_thrshold_fix_content_idx[1]);
        num_chr = staCvtUNumToStr(dst_buf, gmon->outdev.pump.threshold);
        XASSERT(num_chr <= gmon_print_sensor_thrshold_fix_content_idx[1]);

        dst_buf = gmon_print_sensor_thrshold_var_content_ptr[1];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_thrshold_fix_content_idx[3]);
        num_chr = staCvtUNumToStr(dst_buf, gmon->outdev.fan.threshold);
        XASSERT(num_chr <= gmon_print_sensor_thrshold_fix_content_idx[3]);

        dst_buf = gmon_print_sensor_thrshold_var_content_ptr[2];
        XMEMSET(dst_buf, 0x20, gmon_print_sensor_thrshold_fix_content_idx[5]);
        num_chr = staCvtUNumToStr(dst_buf, gmon->outdev.bulb.threshold);
        XASSERT(num_chr <= gmon_print_sensor_thrshold_fix_content_idx[5]);
    }
    stationSysExitCritical();
} // end of staUpdatePrintStrThreshold


static const  char* staCvtOutDevStatusToStr(gMonOutDevStatus in)
{
    const  char* out = NULL;
    switch(in) {
        case GMON_OUT_DEV_STATUS_OFF  :
            out = GMON_PRINT_WORDS_OFF; break;
        case GMON_OUT_DEV_STATUS_ON   :
            out = GMON_PRINT_WORDS_ON; break;
        case GMON_OUT_DEV_STATUS_PAUSE:
            out = GMON_PRINT_WORDS_PAUSE; break;
        default:
            break;
    }
    return out;
} // end of staCvtOutDevStatusToStr


static const  char* staCvtGMonStatusToStr(gMonStatus in)
{
    const  char* out = NULL;
    switch(in) {
        case GMON_RESP_OK:
            out = GMON_PRINT_WORDS_SUCCEED; break;
        case GMON_RESP_SKIP:
            out = GMON_PRINT_WORDS_SKIPPED; break;
        default:
            out = GMON_PRINT_WORDS_FAILED; break;
            break;
    }
    return out;
} // end of staCvtGMonStatusToStr



void        staUpdatePrintStrOutDevStatus(gardenMonitor_t  *gmon)
{
    unsigned char  *dst_buf = NULL;
    const    char  *label_buf = NULL;

    stationSysEnterCritical();
    {
        dst_buf   = gmon_print_outdev_status_var_content_ptr[0];
        label_buf = staCvtOutDevStatusToStr(gmon->outdev.pump.status);
        XMEMSET(dst_buf, 0x20, gmon_print_outdev_status_fix_content_idx[1]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf   = gmon_print_outdev_status_var_content_ptr[1];
        label_buf = staCvtOutDevStatusToStr(gmon->outdev.fan.status);
        XMEMSET(dst_buf, 0x20, gmon_print_outdev_status_fix_content_idx[3]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf   = gmon_print_outdev_status_var_content_ptr[2];
        label_buf = staCvtOutDevStatusToStr(gmon->outdev.bulb.status);
        XMEMSET(dst_buf, 0x20, gmon_print_outdev_status_fix_content_idx[5]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));
    }
    stationSysExitCritical();
} // end of staUpdatePrintStrOutDevStatus



void  staUpdatePrintStrNetConn(gardenMonitor_t  *gmon)
{
    unsigned char  *dst_buf = NULL;
    const    char  *label_buf = NULL;
    unsigned int    time_tmp = 0;
    uint8_t  num_chr = 0;

    stationSysEnterCritical();
    {
        dst_buf = gmon_print_netconn_status_var_content_ptr[0];
        label_buf = staCvtGMonStatusToStr(gmon->netconn.status.sent);
        XMEMSET(dst_buf, 0x20, gmon_print_netconn_status_fix_content_idx[1]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf = gmon_print_netconn_status_var_content_ptr[1];
        label_buf = staCvtGMonStatusToStr(gmon->netconn.status.recv);
        XMEMSET(dst_buf, 0x20, gmon_print_netconn_status_fix_content_idx[3]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf = gmon_print_netconn_status_var_content_ptr[2];
        XMEMSET(dst_buf, 0x20, gmon_print_netconn_status_fix_content_idx[5]);
        time_tmp = gmon->user_ctrl.last_update.ticks * GMON_NUM_MILLISECONDS_PER_TICK;
        dst_buf  += staCvtUNumToStr(dst_buf, (time_tmp / 3600000));
       *dst_buf++ = ':';
        dst_buf  += staCvtUNumToStr(dst_buf, ((time_tmp / 60000) % 60));
       *dst_buf++ = ':';
        dst_buf  += staCvtUNumToStr(dst_buf, ((time_tmp / 1000) % 60));
        XASSERT((dst_buf - gmon_print_netconn_status_var_content_ptr[2]) <= gmon_print_netconn_status_fix_content_idx[5]);

        dst_buf = gmon_print_netconn_status_var_content_ptr[3];
        XMEMSET(dst_buf, 0x20, gmon_print_netconn_status_fix_content_idx[7]);
        time_tmp = gmon->user_ctrl.last_update.days ;
        num_chr  = staCvtUNumToStr(dst_buf, time_tmp);
        XASSERT(num_chr <= gmon_print_netconn_status_fix_content_idx[7]);
    }
    stationSysExitCritical();
} // end of staUpdatePrintStrNetConn




void  stationDisplayTaskFn(void* params)
{
    gardenMonitor_t  *gmon = NULL;
    uint16_t  screen_width = 0;
    const uint16_t  maxnum_lines_cnt = 1000; // TODO: determine when to switch lines to show these information
    uint16_t  switch_lines_cnt = 0;
    uint8_t   idx = 0;

    gmon = (gardenMonitor_t *)params;
    // Get screen size of low-level display device, figure out number of lines of string
    // can be printed on the screen every time.
    screen_width = GMON_DISPLAY_DEV_GET_SCR_WIDTH();

    while(1) {
        if(switch_lines_cnt > maxnum_lines_cnt) { // switch lines after some amount of time passed
            uint16_t  tmp = gmon_print_info[0].posy;
            for(idx = 1; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
                gmon_print_info[idx - 1].posy = gmon_print_info[idx].posy;
            } // end of for loop
            gmon_print_info[GMON_DISPLAY_NUM_PRINT_STRINGS - 1].posy = tmp;
            switch_lines_cnt = 0;
        } else {
            switch_lines_cnt++;
        }
        for(idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
            int end_pos_x = gmon_print_info[idx].font->width * gmon_print_info[idx].str.len + gmon_print_info[idx].posx;
            if(end_pos_x > 0) {
                gmon_print_info[idx].posx -= 3;
            } else { // go back & print beginning of the text lines again
                gmon_print_info[idx].posx = screen_width;
            }
            GMON_DISPLAY_DEV_PRINT_STRING_FN(&gmon_print_info[idx]);
        }
        GMON_DISPLAY_DEV_REFRESH_SCREEN_FN();
        stationSysDelayMs(gmon->display.interval_ms);
    } // end of while loop
} // end of stationDisplayTaskFn

