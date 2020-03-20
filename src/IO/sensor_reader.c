#include "station_include.h"

#define  GMON_SENSOR_RECORD_LIST_SZ   (GMON_CFG_NUM_SENSOR_RECORDS_KEEP << 1)

stationSysMsgbox_t  sensor_to_ctrldev_buf;
stationSysMsgbox_t  sensor_to_netconn_buf;
static gmonSensorRecord_t   gmon_avail_record_list[GMON_SENSOR_RECORD_LIST_SZ];
static gmonSensorRecord_t  *gmon_latest_sensor_data;

static gmonSensorRecord_t* staAllocSensorRecord(void)
{
    gmonSensorRecord_t *out = NULL;
    uint16_t  idx = 0;
    for(idx = 0; idx < GMON_SENSOR_RECORD_LIST_SZ; idx++) {
        if(gmon_avail_record_list[idx].flgs.alloc == 0) {
            gmon_avail_record_list[idx].flgs.alloc = 1;
            out = &gmon_avail_record_list[idx];
            break;
        }
    } // end of for loop
    XASSERT(out != NULL); // check whether there's no item available
    return out;
} // end of staAllocSensorRecord


gMonStatus  staFreeSensorRecord(gmonSensorRecord_t* record)
{ // TODO: make it observable to network component, since we need to send collected sensor read data to remote backend server
    gmonSensorRecord_t *addr_start = NULL;
    gmonSensorRecord_t *addr_end  = NULL;
    uint16_t  idx = 0;
    addr_start = &gmon_avail_record_list[0];
    addr_end   = &gmon_avail_record_list[GMON_SENSOR_RECORD_LIST_SZ];
    if((record < addr_start) || (addr_end <= record)) {
        return GMON_RESP_ERRMEM;
    } // memory out of range
    for(idx = 0; idx < GMON_SENSOR_RECORD_LIST_SZ; idx++) {
        addr_start = &gmon_avail_record_list[idx];
        addr_end   = &gmon_avail_record_list[idx + 1];
        if((record > addr_start) && (record < addr_end)) {
            return GMON_RESP_ERRMEM; // memory not aligned
        } else if (record == addr_start) {
            if(addr_start->flgs.alloc != 0) {
                addr_start->flgs.alloc = 0;
            }
            break;
        }
    } // end of for loop
    return GMON_RESP_OK;
} // end of staFreeSensorRecord


gMonStatus  staCpySensorRecord(gmonSensorRecord_t  *dst, gmonSensorRecord_t *src, size_t  sz)
{
    unsigned int  idx = 0;
    gMonStatus status = GMON_RESP_OK;
    if(dst == NULL || src == NULL || sz < 0) {
        return GMON_RESP_ERRARGS;
    }
    for(idx = 0; idx < sz; idx++) {
        if(dst[idx].flgs.alloc == 0 || src[idx].flgs.alloc == 0) {
            status = GMON_RESP_ERRMEM;
            break;
        }
    } // end of for lopp
    if(status == GMON_RESP_OK) {
        XMEMCPY(dst, src, sizeof(gmonSensorRecord_t) * sz);
    }
    return status;
} // end of staCpySensorRecord


static gMonStatus  staAddNewRecordToMsgBuf(stationSysMsgbox_t msgbuf, gmonSensorRecord_t *msg, uint32_t block_time)
{
    gMonStatus status = GMON_RESP_OK;
    status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
    if(status != GMON_RESP_OK) {
        // if message box is full, abandon the oldest item of the message box, ensure all
        // available items in the message box are up-to-date.
        gmonSensorRecord_t  *oldest_record = NULL;
        status = staSysMsgBoxGet(msgbuf, (void **)&oldest_record, block_time);
        XASSERT((status == GMON_RESP_OK) && (oldest_record != NULL));
        staFreeSensorRecord(oldest_record);
        // try adding new record again
        status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
        XASSERT(status == GMON_RESP_OK);
    }
    return status;
} // end of staAddNewRecordToMsgBuf


static gMonStatus  staNotifyOthersWithNewRecord(gmonSensorRecord_t *new_record, uint32_t block_time)
{
    gmonSensorRecord_t  *record_tmp = NULL;
    gMonStatus status = GMON_RESP_OK;
    unsigned char all_data_ready = 1;

    staAddNewRecordToMsgBuf(sensor_to_ctrldev_buf, new_record, block_time);
    if(new_record->flgs.avail_soil_moist != 0) {
        gmon_latest_sensor_data->flgs.avail_soil_moist = new_record->flgs.avail_soil_moist;
        gmon_latest_sensor_data->soil_moist = new_record->soil_moist;
    }
    if(new_record->flgs.avail_air_temp != 0) {
        gmon_latest_sensor_data->flgs.avail_air_temp = new_record->flgs.avail_air_temp;
        gmon_latest_sensor_data->air_temp = new_record->air_temp;
    }
    if(new_record->flgs.avail_air_humid != 0) {
        gmon_latest_sensor_data->flgs.avail_air_humid = new_record->flgs.avail_air_humid;
        gmon_latest_sensor_data->air_humid = new_record->air_humid;
    }
    if(new_record->flgs.avail_lightness != 0) {
        gmon_latest_sensor_data->flgs.avail_lightness = new_record->flgs.avail_lightness;
        gmon_latest_sensor_data->lightness = new_record->lightness;
    }
    gmon_latest_sensor_data->curr_ticks = new_record->curr_ticks;
    gmon_latest_sensor_data->curr_days  = new_record->curr_days ;
    all_data_ready &= gmon_latest_sensor_data->flgs.avail_soil_moist;
    all_data_ready &= gmon_latest_sensor_data->flgs.avail_air_temp ;
    all_data_ready &= gmon_latest_sensor_data->flgs.avail_air_humid;
    all_data_ready &= gmon_latest_sensor_data->flgs.avail_lightness;
    if(all_data_ready != 0) {
        record_tmp = staAllocSensorRecord();
        status = staCpySensorRecord(record_tmp, gmon_latest_sensor_data, (size_t)0x1);
        staAddNewRecordToMsgBuf(sensor_to_netconn_buf, record_tmp, block_time);
        gmon_latest_sensor_data->flgs.avail_soil_moist = 0;
        gmon_latest_sensor_data->flgs.avail_air_temp   = 0;
        gmon_latest_sensor_data->flgs.avail_air_humid  = 0;
        gmon_latest_sensor_data->flgs.avail_lightness  = 0;
    }
    return status;
} // end of staNotifyOthersWithNewRecord


static void staAdjustSensorReadInterval(gardenMonitor_t *gmon)
{
    unsigned int new_refresh_time = 0;
    //// bulb_status = gmon->outdev.bulb.status;
    gMonOutDevStatus  pump_status = gmon->outdev.pump.status;
    gMonOutDevStatus  fan_status  = gmon->outdev.fan.status;
    if(pump_status == GMON_OUT_DEV_STATUS_ON || pump_status == GMON_OUT_DEV_STATUS_PAUSE) {
        new_refresh_time = GMON_SENSOR_READ_INTERVAL_MS_PUMP_ON;
    } else if (fan_status == GMON_OUT_DEV_STATUS_ON || fan_status == GMON_OUT_DEV_STATUS_PAUSE) {
        new_refresh_time = GMON_SENSOR_READ_INTERVAL_MS_FAN_ON;
    } else { // TODO: let users modify default time interval to refresh sensor data through backend service
        new_refresh_time = gmon->sensor_read_interval.default_ms;
    }
    if(new_refresh_time != gmon->sensor_read_interval.curr_ms) {
        gmon->sensor_read_interval.curr_ms     = new_refresh_time;
        gmon->outdev.pump.sensor_read_interval = new_refresh_time;
        gmon->outdev.fan.sensor_read_interval  = new_refresh_time;
    }
} // end of staAdjustSensorReadInterval


gMonStatus  stationIOinit(gardenMonitor_t *gmon)
{
    gMonStatus status = GMON_RESP_OK;
    if(gmon == NULL) { status = GMON_RESP_ERRARGS; goto done;}
    gmon->sensor_read_interval.default_ms = GMON_CFG_SENSOR_READ_INTERVAL_MS;
    status = GMON_SENSOR_INIT_FN_SOIL_MOIST();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_INIT_FN_LIGHT();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_INIT_FN_AIR_TEMP();
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_INIT_FN_PUMP(&gmon->outdev.pump);
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_INIT_FN_FAN(&gmon->outdev.fan);
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_INIT_FN_BULB(&gmon->outdev.bulb);
    if(status < 0) { goto done; }
    staAdjustSensorReadInterval(gmon);
    sensor_to_ctrldev_buf = staSysMsgBoxCreate( GMON_CFG_NUM_SENSOR_RECORDS_KEEP );
    XASSERT(sensor_to_ctrldev_buf != NULL);
    sensor_to_netconn_buf = staSysMsgBoxCreate( GMON_CFG_NUM_SENSOR_RECORDS_KEEP );
    XASSERT(sensor_to_netconn_buf != NULL);
    XMEMSET(&gmon_avail_record_list[0], 0x00, sizeof(gmonSensorRecord_t) * GMON_SENSOR_RECORD_LIST_SZ);
    gmon_latest_sensor_data = staAllocSensorRecord();
done:
    return status;
} // end of stationIOinit


gMonStatus  stationIOdeinit(void)
{
    gMonStatus status = GMON_RESP_OK;
    staSysMsgBoxDelete( &sensor_to_ctrldev_buf );
    XASSERT(sensor_to_ctrldev_buf == NULL);
    staSysMsgBoxDelete( &sensor_to_netconn_buf ); // TODO: verify
    XASSERT(sensor_to_netconn_buf == NULL);
    status = GMON_OUTDEV_DEINIT_FN_BULB();
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_DEINIT_FN_FAN();
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_DEINIT_FN_PUMP();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_DEINIT_FN_SOIL_MOIST();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_DEINIT_FN_LIGHT();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_DEINIT_FN_AIR_TEMP();
done:
    return status;
} // end of stationIOdeinit


void  stationSensorReaderTaskFn(void* params)
{
    gardenMonitor_t    *gmon = NULL;
    gmonSensorRecord_t *new_record = NULL;
    const uint32_t  block_time = 0;
    unsigned int    soil_moist = 0;
    unsigned int    lightness = 0;
    float  air_temp   = 0.f;
    float  air_humid  = 0.f;
    gMonStatus status = GMON_RESP_OK;

    gmon = (gardenMonitor_t *)params;
    while(1) {
        new_record = staAllocSensorRecord();
        // read value from sensors to the message queue (for communicating device-controller task)
        status = GMON_SENSOR_READ_FN_SOIL_MOIST(&soil_moist);
        if(status == GMON_RESP_OK) {
            new_record->soil_moist = soil_moist;
            new_record->flgs.avail_soil_moist = 1;
        } else {
            new_record->flgs.avail_soil_moist = 0;
        }
        status = staDaylightTrackRefreshSensorData(&lightness);
        if(status == GMON_RESP_OK) {
            new_record->lightness = lightness;
            new_record->flgs.avail_lightness  = 1;
        } else {
            new_record->flgs.avail_lightness  = 0;
        }
        status = staAirCondTrackRefreshSensorData(&air_temp, &air_humid);
        if(status == GMON_RESP_OK) {
            new_record->air_temp  = air_temp ;
            new_record->air_humid = air_humid;
            new_record->flgs.avail_air_temp   = 1;
            new_record->flgs.avail_air_humid  = 1;
        } else {
            new_record->flgs.avail_air_temp   = 0;
            new_record->flgs.avail_air_humid  = 0;
        } // end of if all read data are OK from sensor
        new_record->curr_ticks = stationGetTicksPerDay();
        new_record->curr_days  = stationGetDays();
        status = staNotifyOthersWithNewRecord(new_record, block_time);
        stationSysDelayMs(gmon->sensor_read_interval.curr_ms); // sleep for a while
        // check any of output device is working, then reduce the daley time of this task
        // e.g. pump is working, so it needs to refresh data from soil moisture sensor more frequently.
        staAdjustSensorReadInterval(gmon);
    } // end of loop
} // end of stationSensorReaderTaskFn


