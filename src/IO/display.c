#include "station_include.h"

// This application prints multiple lines of follwoing strings :
// * data actually read from sensor
//       [Last Record]: Soil moisture: 1024, Air Temp: 34.5'C, Air Humidity: 101, Lightness: 1001.
// * threshold of each sensor to trigger output control device
//       [Thresold]: Soil moisture: 1001, Air Temp: 30.5 C, Lightness: 0085.
// * output control device status
//       [Output Device] Pump: (ON/OFF), Cooling Fan: (ON/OFF), Bulb: (ON/OFF)
// * network status
//       [Network]: records log sent: (succeed/failed), user control received: (succeed/skipped/failed), Last
//       Update at 23:59:58, 99999 day(s) after system boot.

// For sensor blocks, these templates will be used to extract prefixes, but the full string is dynamically
// built.
#define GMON_PRINT_ACTUATOR_TRIGGER_THRSHOLD "[Thresold]: Pump: 1001, Fan: 130.5, Bulb: 0085."
#define GMON_PRINT_STRING_ACTUATOR_STATUS    "[Actuator] Pump: PAUSE, Cooling Fan: PAUSE, Bulb: PAUSE."
#define GMON_PRINT_STRING_NETCONN_STATUS \
    "[Network]: records log sent: succeed, user control received: succeed, Last Update at " \
    "23:59:58, 99999 day(s) after system boot."

// Fixed prefixes and separators for dynamic string construction
#define SOIL_SENSOR_PREFIX     "[Sensor Log]: Soil moisture: "
#define AIR_SENSOR_PREFIX      "[Sensor Log]: Air Temp: "
#define AIR_SENSOR_MID_PART    "'C, Air Humidity: "
#define LIGHT_SENSOR_PREFIX    "[Sensor Log]: Lightness: "
#define SENSOR_VALUE_SEPARATOR ", "
#define SENSOR_VALUE_SUFFIX    "."

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

// Maximum number of characters for a single unsigned integer value less than 10000
#define MAX_UINT_STR_LEN 4
// Maximum number of characters for a single float value (e.g., -123.4) for 1 decimal precision.
// Assuming sign, 4 digits before decimal, decimal point, 1 digit after decimal, e.g., "-1234.5". Max 6.
#define MAX_FLOAT_STR_LEN 6

#define GMON_TEMP_UINT_STR_BUF_SIZE  (MAX_UINT_STR_LEN + 1)
#define GMON_TEMP_FLOAT_STR_BUF_SIZE (MAX_FLOAT_STR_LEN + 1)

// Helper function for safe unsigned integer to string conversion
// Returns GMON_RESP_OK on success, GMON_RESP_ERRMEM if converted string exceeds max_allowed_len or output
// buffer is too small.
static gMonStatus staRenderSafeCvtUNumToStr(
    unsigned char *out_buf, size_t out_buf_remaining_len, unsigned int num, unsigned char *chars_written
) {
    unsigned char temp_str_buf[GMON_TEMP_UINT_STR_BUF_SIZE] = {0};
    unsigned char num_chr = staCvtUNumToStr(temp_str_buf, num);
    if (num_chr > MAX_UINT_STR_LEN || num_chr > out_buf_remaining_len)
        return GMON_RESP_ERRMEM;
    XMEMCPY(out_buf, temp_str_buf, num_chr);
    *chars_written = num_chr;
    return GMON_RESP_OK;
}

// Helper function for safe float to string conversion
// Returns GMON_RESP_OK on success, GMON_RESP_ERRMEM if converted string exceeds max_allowed_len or output
// buffer is too small.
static gMonStatus staRenderSafeCvtFloatToStr(
    unsigned char *out_buf, size_t out_buf_remaining_len, float num, unsigned short precision,
    unsigned char *chars_written
) {
    unsigned char temp_str_buf[GMON_TEMP_FLOAT_STR_BUF_SIZE] = {0};
    unsigned int  num_chr_u = staCvtFloatToStr(temp_str_buf, num, precision);
    unsigned char num_chr = (unsigned char)num_chr_u;
    if (num_chr_u > MAX_FLOAT_STR_LEN || num_chr > out_buf_remaining_len)
        return GMON_RESP_ERRMEM;
    XMEMCPY(out_buf, temp_str_buf, num_chr);
    *chars_written = num_chr;
    return GMON_RESP_OK;
}

static gMonStatus staRenderAllocOrResizeBuffer(gmonStr_t *str_info, unsigned short new_required_len) {
    if (str_info == NULL || new_required_len == 0)
        return GMON_RESP_ERRARGS;
    // Reuse existing buffer if its allocated size matches the new required size
    if (str_info->data != NULL && str_info->len == new_required_len)
        return GMON_RESP_OK;
    // Free old buffer if it exists (and size is different or no buffer previously)
    if (str_info->data != NULL) {
        XMEMFREE(str_info->data);
        *str_info = (gmonStr_t){0};
    }
    str_info->data = XMALLOC(new_required_len);
    if (str_info->data == NULL) {
        *str_info = (gmonStr_t){0};
        return GMON_RESP_ERRMEM;
    }
    str_info->len = new_required_len; // Store the newly allocated size
    return GMON_RESP_OK;
}

static gMonStatus staUpdatePrintStrSoilSensorData(gmonPrintInfo_t *content, void *app_ctx) {
    gMonStatus     status = GMON_RESP_OK;
    gmonEvent_t   *new_evt = app_ctx;
    unsigned int  *soil_data = (unsigned int *)new_evt->data, current_len = 0;
    unsigned char *dst_buf = NULL;
    // Calculate new_required_len (allocated size): Prefix + (N * (MaxDigits + Separator)) + Suffix (includes
    // null terminator) Example: "[Sensor Log]: Soil moisture: " (29) + (N * (10 + 2)) + 1 ('.') + 1 (NULL)
    unsigned short new_required_len =
        sizeof(SOIL_SENSOR_PREFIX) - 1 +
        (new_evt->num_active_sensors * (MAX_UINT_STR_LEN + sizeof(SENSOR_VALUE_SEPARATOR) - 1)) +
        sizeof(SENSOR_VALUE_SUFFIX);

    stationSysEnterCritical();
    // Reallocate/resize buffer as needed
    status = staRenderAllocOrResizeBuffer(&content->str, new_required_len);
    if (status != GMON_RESP_OK)
        goto done_critical_section;
    dst_buf = content->str.data;     // Assign dst_buf after successful allocation or reuse
    content->str.nbytes_written = 0; // Reset written bytes as we're rebuilding the string

    current_len = sizeof(SOIL_SENSOR_PREFIX) - 1;
    XMEMCPY(dst_buf, SOIL_SENSOR_PREFIX, current_len);

    for (unsigned char i = 0; i < new_evt->num_active_sensors; ++i) {
        uint8_t num_chr = 0;
        // Pass remaining buffer capacity including space for null terminator
        status = staRenderSafeCvtUNumToStr(
            &dst_buf[current_len], content->str.len - current_len, soil_data[i], &num_chr
        );
        if (status != GMON_RESP_OK)
            goto done_critical_section;
        current_len += num_chr;
        if (i < new_evt->num_active_sensors - 1) {
            XMEMCPY(&dst_buf[current_len], SENSOR_VALUE_SEPARATOR, sizeof(SENSOR_VALUE_SEPARATOR) - 1);
            current_len += sizeof(SENSOR_VALUE_SEPARATOR) - 1;
        }
    }
    XMEMCPY(&dst_buf[current_len], SENSOR_VALUE_SUFFIX, sizeof(SENSOR_VALUE_SUFFIX) - 1); // Copy string part
    current_len += sizeof(SENSOR_VALUE_SUFFIX) - 1;
    dst_buf[current_len] = '\0'; // Null-terminate
    content->str.nbytes_written = current_len;
    status = GMON_RESP_OK;
done_critical_section:
    stationSysExitCritical();
    return status;
}

static gMonStatus staUpdatePrintStrAirSensorData(gmonPrintInfo_t *content, void *app_ctx) {
    gMonStatus     status = GMON_RESP_OK;
    gmonEvent_t   *new_evt = app_ctx;
    gmonAirCond_t *air_data = (gmonAirCond_t *)new_evt->data;
    unsigned char *dst_buf = NULL;
    unsigned int   current_len = 0;
    // Calculate required_len:
    // Prefix ("[Sensor Log]: Air Temp: ") (24)
    // + N * (MaxFloatTemp + MidPart + MaxFloatHumid + Separator)
    unsigned short per_sensor_block_len = sizeof(AIR_SENSOR_PREFIX) - sizeof("[Sensor Log]: ") +
                                          MAX_FLOAT_STR_LEN + sizeof(AIR_SENSOR_MID_PART) - 1 +
                                          MAX_FLOAT_STR_LEN; // 10+7+17+7 = 41
    unsigned short required_len =
        sizeof("[Sensor Log]: ") - 1 + // Base prefix for consistency
        (new_evt->num_active_sensors * per_sensor_block_len) +
        ((new_evt->num_active_sensors > 0)
             ? (new_evt->num_active_sensors - 1) * (sizeof(SENSOR_VALUE_SEPARATOR) - 1)
             : 0) +
        sizeof(SENSOR_VALUE_SUFFIX); // including NULL terminator at the end

    stationSysEnterCritical();
    // Reallocate/resize buffer as needed
    status = staRenderAllocOrResizeBuffer(&content->str, required_len);
    if (status != GMON_RESP_OK)
        goto done_critical_section;
    dst_buf = content->str.data;     // Assign dst_buf after successful allocation or reuse
    content->str.nbytes_written = 0; // Reset written bytes as we're rebuilding the string

    // Start with "[Sensor Log]: "
    current_len = sizeof("[Sensor Log]: ") - 1;
    XMEMCPY(dst_buf, "[Sensor Log]: ", current_len);
    for (unsigned char i = 0; i < new_evt->num_active_sensors; ++i) {
        uint8_t num_chr;
        // Add "Air Temp: "
        XMEMCPY(
            &dst_buf[current_len], AIR_SENSOR_PREFIX + sizeof("[Sensor Log]: ") - 1,
            sizeof(AIR_SENSOR_PREFIX) - sizeof("[Sensor Log]: ")
        ); // Skip the common prefix
        current_len += sizeof(AIR_SENSOR_PREFIX) - sizeof("[Sensor Log]: ");
        // Add temperature value
        status = staRenderSafeCvtFloatToStr(
            &dst_buf[current_len], required_len - current_len, air_data[i].temporature, 0x1, &num_chr
        );
        if (status != GMON_RESP_OK) // Check for internal conversion error (buffer too small)
            goto done_critical_section;
        current_len += num_chr;
        // Add "'C, Air Humidity: "
        XMEMCPY(&dst_buf[current_len], AIR_SENSOR_MID_PART, sizeof(AIR_SENSOR_MID_PART) - 1);
        current_len += sizeof(AIR_SENSOR_MID_PART) - 1;
        // Add humidity value
        status = staRenderSafeCvtFloatToStr(
            &dst_buf[current_len], required_len - current_len, air_data[i].humidity, 0x1, &num_chr
        );
        if (status != GMON_RESP_OK) // Check for internal conversion error (buffer too small)
            goto done_critical_section;
        current_len += num_chr;
        if (i < new_evt->num_active_sensors - 1) {
            XMEMCPY(&dst_buf[current_len], SENSOR_VALUE_SEPARATOR, sizeof(SENSOR_VALUE_SEPARATOR) - 1);
            current_len += sizeof(SENSOR_VALUE_SEPARATOR) - 1;
        }
    } // end of loop
    XMEMCPY(&dst_buf[current_len], SENSOR_VALUE_SUFFIX, sizeof(SENSOR_VALUE_SUFFIX) - 1);
    current_len += sizeof(SENSOR_VALUE_SUFFIX) - 1;
    dst_buf[current_len] = '\0'; // Null-terminator
    content->str.nbytes_written = current_len;
    status = GMON_RESP_OK;
done_critical_section:
    stationSysExitCritical();
    return status;
}

static gMonStatus staUpdatePrintStrLightSensorData(gmonPrintInfo_t *content, void *app_ctx) {
    gMonStatus     status = GMON_RESP_OK;
    gmonEvent_t   *new_evt = app_ctx;
    unsigned int  *light_data = (unsigned int *)new_evt->data, current_len = 0;
    unsigned char *dst_buf = NULL;

    // Calculate required_len: Prefix + (N * (MaxDigits + Separator)) + Suffix + NULL
    // Example: "[Sensor Log]: Lightness: " (25) + (N * (10 + 2)) + 1 ('.') + 1 (NULL)
    unsigned short required_len =
        sizeof(LIGHT_SENSOR_PREFIX) - 1 +
        (new_evt->num_active_sensors * (MAX_UINT_STR_LEN + sizeof(SENSOR_VALUE_SEPARATOR) - 1)) +
        sizeof(SENSOR_VALUE_SUFFIX);

    stationSysEnterCritical();
    status = staRenderAllocOrResizeBuffer(&content->str, required_len);
    if (status != GMON_RESP_OK)
        goto done_critical_section;
    dst_buf = content->str.data;     // Assign dst_buf after successful allocation or reuse
    content->str.nbytes_written = 0; // Reset written bytes as we're rebuilding the string
    current_len = sizeof(LIGHT_SENSOR_PREFIX) - 1;
    XMEMCPY(dst_buf, LIGHT_SENSOR_PREFIX, current_len);

    for (unsigned char i = 0; i < new_evt->num_active_sensors; ++i) {
        uint8_t num_chr = 0;
        status = staRenderSafeCvtUNumToStr(
            &dst_buf[current_len], required_len - current_len, light_data[i], &num_chr
        );
        if (status != GMON_RESP_OK)
            goto done_critical_section;
        current_len += num_chr;
        if (i < new_evt->num_active_sensors - 1) {
            XMEMCPY(&dst_buf[current_len], SENSOR_VALUE_SEPARATOR, sizeof(SENSOR_VALUE_SEPARATOR) - 1);
            current_len += sizeof(SENSOR_VALUE_SEPARATOR) - 1;
        }
    }
    XMEMCPY(&dst_buf[current_len], SENSOR_VALUE_SUFFIX, sizeof(SENSOR_VALUE_SUFFIX) - 1);
    current_len += sizeof(SENSOR_VALUE_SUFFIX) - 1;
    dst_buf[current_len] = '\0'; // Null-terminate
    content->str.nbytes_written = current_len;
    status = GMON_RESP_OK;
done_critical_section:
    stationSysExitCritical();
    return status;
}

static gMonStatus staUpdatePrintStrActuatorThreshold(
    gmonPrintInfo_t *content, void *app_ctx
) { // this should be refreshed every time after data exchange from remote backend
    gardenMonitor_t *gmon = (gardenMonitor_t *)app_ctx;
    unsigned char   *dst_buf = NULL;
    unsigned short   num_chr = 0;

    const short    fix_content_idx[] = {18, 4, 7, 5, 8, 4, 1, 0};
    unsigned char *var_content_ptr[3];
    staInitPrintTxtVarPtr(&content->str, &fix_content_idx[0], &var_content_ptr[0]);
    stationSysEnterCritical();
    {
        dst_buf = var_content_ptr[0];
        XMEMSET(dst_buf, 0x20, fix_content_idx[1]);
        num_chr = staCvtUNumToStr(dst_buf, (unsigned int)gmon->actuator.pump.threshold);
        XASSERT(num_chr <= fix_content_idx[1]);

        dst_buf = var_content_ptr[1];
        XMEMSET(dst_buf, 0x20, fix_content_idx[3]);
        num_chr = staCvtFloatToStr(dst_buf, (float)gmon->actuator.fan.threshold, 0x1);
        XASSERT(num_chr <= fix_content_idx[3]);

        dst_buf = var_content_ptr[2];
        XMEMSET(dst_buf, 0x20, fix_content_idx[5]);
        num_chr = staCvtUNumToStr(dst_buf, (unsigned int)gmon->actuator.bulb.threshold);
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
    }
    return out;
}

static gMonStatus staUpdatePrintStrActuatorStatus(gmonPrintInfo_t *content, void *app_ctx) {
    gardenMonitor_t *gmon = (gardenMonitor_t *)app_ctx;
    unsigned char   *dst_buf = NULL;
    const char      *label_buf = NULL;

    const short    fix_content_idx[] = {17, 5, 15, 5, 8, 5, 1, 0};
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

    const short    fix_content_idx[] = {29, 7, 25, 7, 17, 8, 2, 5, 26, 0};
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
        const char *template_str; // For non-sensor blocks, this is the initial content; for sensors, a base.
        gMonBlockType_t btype;
        gMonRenderFn_t  render_fn; // Render function for this block
    } print_info[GMON_DISPLAY_NUM_PRINT_STRINGS] = {
        [GMON_BLOCK_SENSOR_SOIL_RECORD] =
            {
                .btype = GMON_BLOCK_SENSOR_SOIL_RECORD,
                .render_fn = staUpdatePrintStrSoilSensorData,
            },
        [GMON_BLOCK_SENSOR_AIR_RECORD] =
            {
                .btype = GMON_BLOCK_SENSOR_AIR_RECORD,
                .render_fn = staUpdatePrintStrAirSensorData,
            },
        [GMON_BLOCK_SENSOR_LIGHT_RECORD] =
            {
                .btype = GMON_BLOCK_SENSOR_LIGHT_RECORD,
                .render_fn = staUpdatePrintStrLightSensorData,
            },
        [GMON_BLOCK_ACTUATOR_THRESHOLD] =
            {
                .template_str = GMON_PRINT_ACTUATOR_TRIGGER_THRSHOLD,
                .btype = GMON_BLOCK_ACTUATOR_THRESHOLD,
                .render_fn = staUpdatePrintStrActuatorThreshold,
            },
        [GMON_BLOCK_ACTUATOR_STATUS] =
            {
                .template_str = GMON_PRINT_STRING_ACTUATOR_STATUS,
                .btype = GMON_BLOCK_ACTUATOR_STATUS,
                .render_fn = staUpdatePrintStrActuatorStatus,
            },
        [GMON_BLOCK_NETCONN_STATUS] =
            {
                .template_str = GMON_PRINT_STRING_NETCONN_STATUS,
                .btype = GMON_BLOCK_NETCONN_STATUS,
                .render_fn = staUpdatePrintStrNetConn,
            },
    };
    gMonDisplayBlock_t *dblk = NULL;
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
        dblk->content.str.nbytes_written = 0; // Reset written bytes
        // will be reordered later in the task...
        dblk->content.posx = 0;
        dblk->content.posy = (display_ctx->fonts[0].height + 2) * idx;
        dblk->content.font = &display_ctx->fonts[0];
        dblk->btype = print_info[idx].btype;
        dblk->render = print_info[idx].render_fn;

        // Allocate buffer for non-sensor blocks here, copy initial content.
        // Sensor blocks' data will be allocated by their respective render functions.
        switch (dblk->btype) {
        case GMON_BLOCK_SENSOR_SOIL_RECORD:
        case GMON_BLOCK_SENSOR_AIR_RECORD:
        case GMON_BLOCK_SENSOR_LIGHT_RECORD:
            dblk->content.str.data = NULL; // Initially NULL, memory allocated in render function
            dblk->content.str.len = 0;     // Initial length is 0
            break;
        default: // GMON_BLOCK_ACTUATOR_THRESHOLD, GMON_BLOCK_ACTUATOR_STATUS, GMON_BLOCK_NETCONN_STATUS
            dblk->content.str.len = XSTRLEN(print_info[idx].template_str);
            dblk->content.str.data = XMALLOC(dblk->content.str.len + 1); // +1 for null terminator
            if (dblk->content.str.data == NULL) {
                // Handle error: Free any already allocated buffers and return
                for (uint8_t i = 0; i < idx; ++i) {
                    if (display_ctx->blocks[i].content.str.data != NULL) {
                        XMEMFREE(display_ctx->blocks[i].content.str.data);
                    }
                }
                return GMON_RESP_ERRMEM;
            }
            XMEMCPY(dblk->content.str.data, print_info[idx].template_str, dblk->content.str.len);
            dblk->content.str.data[dblk->content.str.len] = '\0'; // Null-terminate the string
            break;
        }
    }
    dblk = &display_ctx->blocks[GMON_BLOCK_ACTUATOR_THRESHOLD];
    dblk->render(&dblk->content, gmon);
    return GMON_DISPLAY_DEV_INIT_FN();
#else
    return GMON_RESP_SKIP;
#endif // end of GMON_CFG_ENABLE_DISPLAY
} // end of staDisplayInit

gMonStatus staDisplayDeInit(gardenMonitor_t *gmon) {
#ifdef GMON_CFG_ENABLE_DISPLAY
    if (gmon == NULL)
        return GMON_RESP_ERRARGS;
    uint8_t idx;
    for (idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        if (gmon->display.blocks[idx].content.str.data != NULL) {
            XMEMFREE(gmon->display.blocks[idx].content.str.data);
            gmon->display.blocks[idx].content.str.data = NULL; // Prevent double-free issues
        }
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
    gMonDisplayBlock_t   *dblk = NULL;

    // Get screen size of low-level display device, figure out number of lines of string
    // can be printed on the screen every time.
    screen_width = GMON_DISPLAY_DEV_GET_SCR_WIDTH();

    while (1) {
        status = staSysMsgBoxGet(gmon->msgpipe.sensor2display, (void **)&new_evt, block_time);
        if (status == GMON_RESP_OK && new_evt != NULL) {
            if (new_evt->data != NULL) { // FIXME , figure out why event data is lost
                // Invoke rendering functions for relevant sensor blocks based on event type
                switch (new_evt->event_type) {
                case GMON_EVENT_SOIL_MOISTURE_UPDATED:
                    dblk = &display_ctx->blocks[GMON_BLOCK_SENSOR_SOIL_RECORD];
                    dblk->render(&dblk->content, new_evt);
                    break;
                case GMON_EVENT_AIR_TEMP_UPDATED:
                    dblk = &display_ctx->blocks[GMON_BLOCK_SENSOR_AIR_RECORD];
                    dblk->render(&dblk->content, new_evt);
                    break;
                case GMON_EVENT_LIGHTNESS_UPDATED:
                    dblk = &display_ctx->blocks[GMON_BLOCK_SENSOR_LIGHT_RECORD];
                    dblk->render(&dblk->content, new_evt);
                    break;
                default:
                    break;
                }
            } // FIXME , figure out why event data is lost
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
