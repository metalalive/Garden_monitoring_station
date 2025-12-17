#include "station_include.h"

// This application prints multiple lines of follwoing strings :
// * data actually read from sensor
//       [Last Record]: Soil moisture: 1024, Air Temp: 34.5'C, Air Humidity: 101, Lightness: 1001.
// * threshold of each sensor to trigger output control device
//       [Thresold]: Soil moisture: 1001, Air Temp: 30.5 C, Lightness: 0085.
// * output control device status
//       [Output Device] Pump: (ON/OFF), Cooling Fan: (ON/OFF), Bulb: (ON/OFF)
// * network status
//       [Network]: records log sent: (succeed/failed), user control received: (succeed/skipped/failed),
//                  Last Update at 23:59:58, 99999 day(s) after system boot.
#define GMON_PRINT_STRING_SENSOR_RECORD \
    (const char *)&( \
        "[Last Record]: Soil moisture: 1024, Air Temp: 134.5'C, Air Humidity: 101.5, Lightness: 1001." \
    )
#define GMON_PRINT_STRING_SENSOR_THRSHOLD \
    (const char *)&("[Thresold]: Soil moisture: 1001, Air Temp: 130.5'C, Lightness: 0085.")
#define GMON_PRINT_STRING_ACTUATOR_STATUS \
    (const char *)&("[Actuator] Pump: PAUSE, Cooling Fan: PAUSE, Bulb: PAUSE.")
#define GMON_PRINT_STRING_NETCONN_STATUS \
    (const char *)&("[Network]: records log sent: succeed, user control received: succeed, Last Update at " \
                    "23:59:58, 99999 day(s) after system boot.")

#define GMON_PRINT_WORDS_DISCONNECTED "Disconneted"
#define GMON_PRINT_WORDS_CONNECTING   "Connecting"
#define GMON_PRINT_WORDS_SUCCEED      "Succeed"
#define GMON_PRINT_WORDS_SKIPPED      "Skipped"
#define GMON_PRINT_WORDS_FAILED       "Failed"
#define GMON_PRINT_WORDS_ON           "ON"
#define GMON_PRINT_WORDS_OFF          "OFF"
#define GMON_PRINT_WORDS_PAUSE        "PAUSE"

extern const unsigned short gmon_txt_font_bitmap_11x18[];

static uint16_t
displayVerticalScroll(uint16_t curr_cnt, uint16_t max_cnt, gMonDisplayBlock_t *dblks, size_t len) {
    if (curr_cnt > max_cnt) {
        // switch vertical lines after some amount of time passed
        uint16_t tmp = dblks[0].content.posy;
        for (size_t idx = 1; idx < len; idx++) {
            dblks[idx - 1].content.posy = dblks[idx].content.posy;
        }
        dblks[len - 1].content.posy = tmp;
        return 0;
    } else {
        return ++curr_cnt;
    }
}

static void displayHorizontalScroll(gmonPrintInfo_t *info, uint16_t scr_width, uint16_t pxl_move) {
    int end_pos_x = info->font->width * info->str.len + info->posx;
    if (end_pos_x > 0) {
        info->posx -= pxl_move;
    } else { // go back & print beginning of the text lines again
        info->posx = scr_width;
    }
    GMON_DISPLAY_DEV_PRINT_STRING_FN(info);
}

static void staInitPrintTxtVarPtr(gmonStr_t *str, const short *fx_content_idx, unsigned char **out) {
    unsigned char *var_ptr = NULL;
    uint8_t        idx = 0;
    XASSERT(str != NULL);
    XASSERT(str->data != NULL);
    XASSERT(fx_content_idx != NULL);
    XASSERT(out != NULL);

    var_ptr = str->data;
    for (idx = 0; fx_content_idx[idx] != 0; idx++) {
        if ((idx & 0x1) == 0x1) {
            out[(idx >> 1)] = var_ptr;
        }
        var_ptr += fx_content_idx[idx];
    } // end of for loop
    XASSERT(var_ptr == &str->data[str->len]);
}

static gMonStatus staUpdatePrintStrSensorData(gmonPrintInfo_t *content, void *app_ctx) {
    gmonEvent_t   *new_evt = app_ctx;
    unsigned char *dst_buf = NULL;
    uint8_t        num_chr = 0;
    // TODO, expand multi-sensor values in display
    const short fix_content_idx[] = {
        30, 4, 12, 5, 18, 5, 13, 4, 1, 0,
    };
    unsigned char *var_content_ptr[4] = {0};
    staInitPrintTxtVarPtr(&content->str, &fix_content_idx[0], &var_content_ptr[0]);
    stationSysEnterCritical();
    switch (new_evt->event_type) {
    case GMON_EVENT_SOIL_MOISTURE_UPDATED:
        dst_buf = var_content_ptr[0];
        XMEMSET(dst_buf, 0x20, fix_content_idx[1]);
        num_chr = staCvtUNumToStr(dst_buf, ((unsigned int *)new_evt->data)[0]);
        XASSERT(num_chr <= fix_content_idx[1]);
        break;
    case GMON_EVENT_AIR_TEMP_UPDATED:
        dst_buf = var_content_ptr[1];
        XMEMSET(dst_buf, 0x20, fix_content_idx[3]);
        num_chr = staCvtFloatToStr(dst_buf, ((gmonAirCond_t *)new_evt->data)[0].temporature, 0x1);
        XASSERT(num_chr <= fix_content_idx[3]);
        dst_buf = var_content_ptr[2];
        XMEMSET(dst_buf, 0x20, fix_content_idx[5]);
        num_chr = staCvtFloatToStr(dst_buf, ((gmonAirCond_t *)new_evt->data)[0].humidity, 0x1);
        XASSERT(num_chr <= fix_content_idx[5]);
        break;
    case GMON_EVENT_LIGHTNESS_UPDATED:
        dst_buf = var_content_ptr[3];
        XMEMSET(dst_buf, 0x20, fix_content_idx[7]);
        num_chr = staCvtUNumToStr(dst_buf, ((unsigned int *)new_evt->data)[0]);
        XASSERT(num_chr <= fix_content_idx[7]);
        break;
    default:
        break;
    }
    stationSysExitCritical();
    return GMON_RESP_OK;
} // end of staUpdatePrintStrSensorData

static gMonStatus staUpdatePrintStrThreshold(
    gmonPrintInfo_t *content, void *app_ctx
) { // this should be refreshed every time after data exchange from remote backend
    gardenMonitor_t *gmon = (gardenMonitor_t *)app_ctx;
    unsigned char   *dst_buf = NULL;
    uint8_t          num_chr = 0;

    const short fix_content_idx[] = {
        27, 4, 12, 5, 15, 4, 1, 0,
    };
    unsigned char *var_content_ptr[3];
    staInitPrintTxtVarPtr(&content->str, &fix_content_idx[0], &var_content_ptr[0]);
    stationSysEnterCritical();
    {
        dst_buf = var_content_ptr[0];
        XMEMSET(dst_buf, 0x20, fix_content_idx[1]);
        num_chr = staCvtUNumToStr(dst_buf, gmon->actuator.pump.threshold);
        XASSERT(num_chr <= fix_content_idx[1]);

        dst_buf = var_content_ptr[1];
        XMEMSET(dst_buf, 0x20, fix_content_idx[3]);
        num_chr = staCvtUNumToStr(dst_buf, gmon->actuator.fan.threshold);
        XASSERT(num_chr <= fix_content_idx[3]);

        dst_buf = var_content_ptr[2];
        XMEMSET(dst_buf, 0x20, fix_content_idx[5]);
        num_chr = staCvtUNumToStr(dst_buf, gmon->actuator.bulb.threshold);
        XASSERT(num_chr <= fix_content_idx[5]);
    }
    stationSysExitCritical();
    return GMON_RESP_OK;
}

static const char *staCvtActuatorStatusToStr(gMonActuatorStatus in) {
    const char *out = NULL;
    switch (in) {
    case GMON_OUT_DEV_STATUS_OFF:
        out = GMON_PRINT_WORDS_OFF;
        break;
    case GMON_OUT_DEV_STATUS_ON:
        out = GMON_PRINT_WORDS_ON;
        break;
    case GMON_OUT_DEV_STATUS_PAUSE:
        out = GMON_PRINT_WORDS_PAUSE;
        break;
    default:
        break;
    }
    return out;
}

static const char *staCvtGMonStatusToStr(gMonStatus in) {
    const char *out = NULL;
    switch (in) {
    case GMON_RESP_OK:
        out = GMON_PRINT_WORDS_SUCCEED;
        break;
    case GMON_RESP_SKIP:
        out = GMON_PRINT_WORDS_SKIPPED;
        break;
    default:
        out = GMON_PRINT_WORDS_FAILED;
        break;
        break;
    }
    return out;
}

static gMonStatus staUpdatePrintStrActuatorStatus(gmonPrintInfo_t *content, void *app_ctx) {
    gardenMonitor_t *gmon = (gardenMonitor_t *)app_ctx;
    unsigned char   *dst_buf = NULL;
    const char      *label_buf = NULL;

    const short fix_content_idx[] = {
        17, 5, 15, 5, 8, 5, 1, 0,
    };
    unsigned char *var_content_ptr[3] = {0};
    staInitPrintTxtVarPtr(&content->str, &fix_content_idx[0], &var_content_ptr[0]);
    stationSysEnterCritical();
    {
        dst_buf = var_content_ptr[0];
        label_buf = staCvtActuatorStatusToStr(gmon->actuator.pump.status);
        XMEMSET(dst_buf, 0x20, fix_content_idx[1]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf = var_content_ptr[1];
        label_buf = staCvtActuatorStatusToStr(gmon->actuator.fan.status);
        XMEMSET(dst_buf, 0x20, fix_content_idx[3]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf = var_content_ptr[2];
        label_buf = staCvtActuatorStatusToStr(gmon->actuator.bulb.status);
        XMEMSET(dst_buf, 0x20, fix_content_idx[5]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));
    }
    stationSysExitCritical();
    return GMON_RESP_OK;
}

static gMonStatus staUpdatePrintStrNetConn(gmonPrintInfo_t *content, void *app_ctx) {
    gardenMonitor_t *gmon = (gardenMonitor_t *)app_ctx;
    unsigned char   *dst_buf = NULL;
    const char      *label_buf = NULL;
    unsigned int     time_tmp = 0;
    uint8_t          num_chr = 0;

    const short fix_content_idx[] = {
        29, 7, 25, 7, 17, 8, 2, 5, 26, 0,
    };
    unsigned char *var_content_ptr[4] = {0};
    staInitPrintTxtVarPtr(&content->str, &fix_content_idx[0], &var_content_ptr[0]);

    stationSysEnterCritical();
    {
        dst_buf = var_content_ptr[0];
        label_buf = staCvtGMonStatusToStr(gmon->netconn.status.sent);
        XMEMSET(dst_buf, 0x20, fix_content_idx[1]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf = var_content_ptr[1];
        label_buf = staCvtGMonStatusToStr(gmon->netconn.status.recv);
        XMEMSET(dst_buf, 0x20, fix_content_idx[3]);
        XMEMCPY(dst_buf, label_buf, XSTRLEN(label_buf));

        dst_buf = var_content_ptr[2];
        XMEMSET(dst_buf, 0x20, fix_content_idx[5]);
        time_tmp = gmon->user_ctrl.last_update.ticks * GMON_NUM_MILLISECONDS_PER_TICK;
        dst_buf += staCvtUNumToStr(dst_buf, (time_tmp / 3600000));
        *dst_buf++ = ':';
        dst_buf += staCvtUNumToStr(dst_buf, ((time_tmp / 60000) % 60));
        *dst_buf++ = ':';
        dst_buf += staCvtUNumToStr(dst_buf, ((time_tmp / 1000) % 60));
        XASSERT((dst_buf - var_content_ptr[2]) <= fix_content_idx[5]);

        dst_buf = var_content_ptr[3];
        XMEMSET(dst_buf, 0x20, fix_content_idx[7]);
        time_tmp = gmon->user_ctrl.last_update.days;
        num_chr = staCvtUNumToStr(dst_buf, time_tmp);
        XASSERT(num_chr <= fix_content_idx[7]);
    }
    stationSysExitCritical();
    return GMON_RESP_OK;
} // end of staUpdatePrintStrNetConn

gMonStatus staDisplayInit(gardenMonitor_t *gmon) {
#ifdef GMON_CFG_ENABLE_DISPLAY
    if (gmon == NULL) {
        return GMON_RESP_ERRARGS;
    }
    const struct {
        const char *template;
        gMonBlockType_t btype;
        gMonRenderFn_t  render_fn;
    } print_info[GMON_DISPLAY_NUM_PRINT_STRINGS] = {
        {
            .template = GMON_PRINT_STRING_SENSOR_RECORD,
            .btype = GMON_BLOCK_SENSOR_RECORD,
            .render_fn = staUpdatePrintStrSensorData,
        },
        {
            .template = GMON_PRINT_STRING_SENSOR_THRSHOLD,
            .btype = GMON_BLOCK_SENSOR_THRESHOLD,
            .render_fn = staUpdatePrintStrThreshold,
        },
        {
            .template = GMON_PRINT_STRING_ACTUATOR_STATUS,
            .btype = GMON_BLOCK_ACTUATOR_STATUS,
            .render_fn = staUpdatePrintStrActuatorStatus,
        },
        {
            .template = GMON_PRINT_STRING_NETCONN_STATUS,
            .btype = GMON_BLOCK_NETCONN_STATUS,
            .render_fn = staUpdatePrintStrNetConn,
        },
    };
    gMonDisplayBlock_t *dblk = NULL;
    size_t              total_len = 0;
    uint8_t             idx = 0;

    gMonDisplayContext_t *display_ctx = &gmon->display;
    display_ctx->fonts[0].width = 11;
    display_ctx->fonts[0].height = 18;
    display_ctx->fonts[0].bitmap = gmon_txt_font_bitmap_11x18;

    display_ctx->config.refresh_rate_ms = GMON_CFG_DISPLAY_SCREEN_REFRESH_TIME_MS;
    display_ctx->config.scroll_speed = 4;                     // As per instruction
    display_ctx->num_blocks = GMON_DISPLAY_NUM_PRINT_STRINGS; // As per instruction

    for (idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        dblk = &display_ctx->blocks[idx];
        dblk->content.str.len = XSTRLEN(print_info[idx].template);
        total_len += dblk->content.str.len;
    }
    unsigned char *buf = XMALLOC(sizeof(char) * total_len);

    for (idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        dblk = &display_ctx->blocks[idx];
        dblk->content.str.data = buf;
        buf += dblk->content.str.len;
        XMEMCPY(dblk->content.str.data, print_info[idx].template, dblk->content.str.len);

        // will be reordered later in the task...
        dblk->content.posx = 0;
        dblk->content.posy = (display_ctx->fonts[0].height + 2) * idx;
        dblk->content.font = &display_ctx->fonts[0];
        dblk->btype = print_info[idx].btype;
        dblk->render = print_info[idx].render_fn;
    }
    dblk = &display_ctx->blocks[GMON_BLOCK_SENSOR_THRESHOLD];
    dblk->render(&dblk->content, gmon);
    return GMON_DISPLAY_DEV_INIT_FN();
#else
    return GMON_RESP_SKIP;
#endif // end of GMON_CFG_ENABLE_DISPLAY
} // end of staDisplayInit

gMonStatus staDisplayDeInit(gardenMonitor_t *gmon) {
#ifdef GMON_CFG_ENABLE_DISPLAY
    if (gmon == NULL) {
        return GMON_RESP_ERRARGS;
    }
    uint8_t idx = 0;
    XMEMFREE((void *)gmon->display.blocks[0].content.str.data);
    for (idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        gmon->display.blocks[idx].content.str.data = NULL;
    }
    return GMON_DISPLAY_DEV_DEINIT_FN();
#else
    return GMON_RESP_SKIP;
#endif
}

void stationDisplayTaskFn(void *params) {
    gmonEvent_t *new_evt = NULL;
    gMonStatus   status = GMON_RESP_OK;
    uint16_t     screen_width = 0, switch_lines_cnt = 0;
    uint8_t      idx = 0;

    const uint32_t        block_time = 0; // GMON_MAX_BLOCKTIME_SYS_MSGBOX;
    const uint16_t        maxnum_lines_cnt = 1000;
    gardenMonitor_t      *gmon = (gardenMonitor_t *)params;
    gMonDisplayContext_t *display_ctx = &gmon->display;

    // Get screen size of low-level display device, figure out number of lines of string
    // can be printed on the screen every time.
    screen_width = GMON_DISPLAY_DEV_GET_SCR_WIDTH();

    while (1) {
        status = staSysMsgBoxGet(gmon->msgpipe.sensor2display, (void **)&new_evt, block_time);
        if (status == GMON_RESP_OK && new_evt != NULL) {
            gMonDisplayBlock_t *dblk = &display_ctx->blocks[GMON_BLOCK_SENSOR_RECORD];
            // Invoke rendering functions for relevant blocks
            dblk->render(&dblk->content, new_evt);
            dblk = &display_ctx->blocks[GMON_BLOCK_ACTUATOR_STATUS];
            dblk->render(&dblk->content, gmon);
            staFreeSensorEvent(&gmon->sensors.event, new_evt);
            new_evt = NULL;
        }
        switch_lines_cnt = displayVerticalScroll(
            switch_lines_cnt, maxnum_lines_cnt, display_ctx->blocks, display_ctx->num_blocks
        );
        for (idx = 0; idx < display_ctx->num_blocks; idx++) {
            displayHorizontalScroll(
                &display_ctx->blocks[idx].content, screen_width, display_ctx->config.scroll_speed
            );
        }
        GMON_DISPLAY_DEV_REFRESH_SCREEN_FN();
        stationSysDelayMs(display_ctx->config.refresh_rate_ms);
    } // end of while loop
} // end of stationDisplayTaskFn
