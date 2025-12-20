#include "station_include.h"

gMonStatus staSetNumAirSensor(gMonSensorMeta_t *s, unsigned char new_val) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    unsigned int temp_num_items = 0;
    gMonStatus   status =
        staSetUintInRange(&temp_num_items, (unsigned int)new_val, (unsigned int)GMON_MAXNUM_AIR_SENSORS, 0U);
    if (status == GMON_RESP_OK)
        s->num_items = (unsigned char)temp_num_items;
    return status;
}
gMonStatus staSetNumResamplesAirSensor(gMonSensorMeta_t *s, unsigned char new_val) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    unsigned int temp_num_resamples = 0;
    gMonStatus   status = staSetUintInRange(
        &temp_num_resamples, (unsigned int)new_val, (unsigned int)GMON_MAX_OVERSAMPLES_AIR_SENSORS, 1U
    );
    if (status == GMON_RESP_OK)
        s->num_resamples = (unsigned char)temp_num_resamples;
    return status;
}

gMonStatus staSensorInitAirTemp(gMonSensorMeta_t *s) {
    gMonStatus status = staSensorSetReadInterval(s, GMON_CFG_SENSOR_READ_INTERVAL_MS);
    if (status != GMON_RESP_OK)
        return status;
    status = staSetNumAirSensor(s, GMON_CFG_NUM_AIR_SENSORS);
    if (status != GMON_RESP_OK)
        return status;
    status = staSetNumResamplesAirSensor(s, GMON_CFG_AIR_SENSOR_NUM_OVERSAMPLE);
    if (status != GMON_RESP_OK)
        return status;
    s->outlier_threshold = GMON_AIR_SENSOR_OUTLIER_THRESHOLD;
    s->mad_threshold = GMON_AIR_SENSOR_MAD_THRESHOLD;
    return staSensorPlatformInitAirTemp(s);
}

gMonStatus staSensorDeInitAirTemp(gMonSensorMeta_t *s) { return staSensorPlatformDeInitAirTemp(s); }

// Helper function to measure the duration of a pin state
static gMonStatus measureVerifyPulse(
    void *signal_pin, uint8_t expect_init_state, uint16_t refpoint_pulse_us, uint16_t deviation_us,
    uint16_t *actual_pulse_us
) {
    uint16_t pulse_us_read = 0;
    uint8_t  state_low2high = 0, state_expected = 0, pulse_range_fit = 0;
    XASSERT(refpoint_pulse_us > deviation_us);
    gMonStatus status = staPlatformMeasurePulse(signal_pin, &state_low2high, &pulse_us_read);
    if (status == GMON_RESP_OK) {
        uint16_t maxlimit = refpoint_pulse_us + deviation_us;
        uint16_t minlimit = refpoint_pulse_us - deviation_us;
        state_expected = (expect_init_state != state_low2high);
        pulse_range_fit = (pulse_us_read >= minlimit && pulse_us_read <= maxlimit);
        if (state_expected && pulse_range_fit) {
            if (actual_pulse_us) {
                *actual_pulse_us = pulse_us_read;
            }
        } else {
            status = GMON_RESP_ERR_MSG_DECODE;
        }
    }
    return status;
}

/**
 * @brief Reads 40-bit data from the DHT11 sensor.
 *
 * This function interacts with the DHT11 sensor through the `signal_pin`
 * to read the 40-bit data stream (5 bytes). It measures the duration of
 * HIGH pulses to differentiate between '0' and '1' bits according to the
 * DHT11 protocol.
 *
 * @param data A 5-element array of uint8_t to store the received 5 bytes of data.
 *             The array will be filled with humidity integer, humidity decimal,
 *             temperature integer, temperature decimal, and checksum bytes.
 * @return GMON_RESP_OK if data is read successfully,
 *         GMON_RESP_TIMEOUT if a signal transition timeout occurs,
 *         or other gMonStatus error codes from platform functions.
 */
static gMonStatus readDht11SensorData(void *signal_pin, uint8_t data[5]) {
    // After MCU pulls the line HIGH for 20-40us and releases it (sets pin to input mode),
    // the DHT11 should respond by pulling the line LOW for ~80us.
    gMonStatus status = GMON_RESP_OK;
    uint16_t   pulse_us = 0, idx;
    // 1. check whether to sample DHT11's LOW acknowledgment pulse with
    // positive-integer length time.
    status = measureVerifyPulse(signal_pin, GMON_PLATFORM_PIN_RESET, 80, 40, NULL);
    if (status != GMON_RESP_OK)
        return status;

    // 2. Check for DHT11's 80us HIGH acknowledgment pulse
    status = measureVerifyPulse(signal_pin, GMON_PLATFORM_PIN_SET, 80, 8, NULL);
    if (status != GMON_RESP_OK)
        return status;

    // 3. Read 40 bits of data
    for (idx = 0; idx < 40; idx++) {
        // Each data bit transmission starts with a 50us LOW pulse
        status = measureVerifyPulse(signal_pin, GMON_PLATFORM_PIN_RESET, 50, 7, NULL);
        if (status != GMON_RESP_OK)
            return status;
        // The length of the subsequent HIGH pulse determines the bit value
        status = measureVerifyPulse(signal_pin, GMON_PLATFORM_PIN_SET, 45, 35, &pulse_us);
        if (status != GMON_RESP_OK)
            return status;
        // Decode bit based on HIGH pulse duration:
        //   - 15-30us indicates '0'
        //   - 62~70us indicates '1'
        if (pulse_us >= 60 && pulse_us <= 78) {
            // valid duration for data bit 1 , set the bit
            data[idx >> 3] |= (1 << (7 - (idx & 0x7)));
        } else if (pulse_us >= 13 && pulse_us <= 30) {
            // valid duration for data bit '0' , Bit is already 0, nothing to do
        } else {
            return GMON_RESP_SENSOR_FAIL;
        }
    }
    // 4. Check for final 50us LOW pulse from DHT11 at the end of transmission
    status = measureVerifyPulse(signal_pin, GMON_PLATFORM_PIN_RESET, 50, 7, NULL);
    // After this, the bus is passively pulled HIGH by a resistor.
    return status;
} // end of readDht11SensorData

static gMonStatus sensorReadAirIteration(void *signal_pin, gmonAirCond_t *out) {
    gMonStatus status = GMON_RESP_OK;
    uint16_t   sum_data_bits = 0;
    uint8_t    record_dht_data[5] = {0};
    // Cast data pointer to gmonAirCond_t for storing humidity and temperature
    if (out == NULL)
        return GMON_RESP_ERRMEM;
    staPlatformPinSetDirection(signal_pin, GMON_PLATFORM_PIN_DIRECTION_OUT);
    staPlatformWritePin(signal_pin, GMON_PLATFORM_PIN_RESET);
    stationSysDelayMs(18); // keep start-out signal from MCU for at least 18 ms
    // setup critical section, in order to prevent the running task from being preempted
    // (or time tick interrupt since using up its time)
    stationSysEnterCritical();
    {
        staPlatformWritePin(signal_pin, GMON_PLATFORM_PIN_SET);
        // datasheet suggests 20 to 40us waiting time on MCU side, but DHT can still acknowledgment
        // shorter high-voltage pulse and respond. TODO: test the behavior
        stationSysDelayUs(2);
        status = staPlatformPinSetDirection(signal_pin, GMON_PLATFORM_PIN_DIRECTION_IN);
        if (status != GMON_RESP_OK)
            goto done;
        // check response signal from DHT11
        status = readDht11SensorData(signal_pin, record_dht_data);
        if (status != GMON_RESP_OK)
            goto done;

        sum_data_bits =
            (record_dht_data[0] + record_dht_data[1] + record_dht_data[2] + record_dht_data[3]) & 0xff;
        if (sum_data_bits != record_dht_data[4]) {
            status = GMON_RESP_SENSOR_FAIL;
            goto done;
        }
        out->humidity = record_dht_data[0] + record_dht_data[1] / 10.f;
        out->temporature = record_dht_data[2] + record_dht_data[3] / 10.f;
    }
done:
    stationSysExitCritical();
    return status;
}

// TODO, FIXME :
// - modify function signature for introducing external reliable reference positive-integer
//   which can calibrate the sensor here .
// - set up power gate to this device
gMonStatus staSensorReadAirTemp(gMonSensorMeta_t *sensor, gmonSensorSample_t *out) {
    if (sensor == NULL || sensor->lowlvl == NULL || sensor->num_items == 0 || out == NULL)
        return GMON_RESP_ERRARGS;
    gMonStatus final_status = GMON_RESP_OK;
    void      *signal_pin = sensor->lowlvl;
    stationSysDelayMs(1000); // Wait 1 second after power on without sending any data
    // Iterate through each 'air sensor' item (if num_items > 1)
    for (unsigned char item_idx = 0; item_idx < sensor->num_items; ++item_idx) {
        // Ensure the data buffer for this specific sample is allocated
        // and is of the expected type (gmonAirCond_t) with sufficient length
        if (out[item_idx].data == NULL || out[item_idx].dtype != GMON_SENSOR_DATA_TYPE_AIRCOND ||
            out[item_idx].len < 1) {
            return GMON_RESP_ERRMEM;
        }
        gMonStatus current_item_status = GMON_RESP_SENSOR_FAIL;
        // Perform resampling for the current sensor item
        for (unsigned char resample_idx = 0; resample_idx < sensor->num_resamples; ++resample_idx) {
            current_item_status =
                sensorReadAirIteration(signal_pin, &((gmonAirCond_t *)out[item_idx].data)[resample_idx]);
        }
        if (current_item_status != GMON_RESP_OK) {
            // If after all resamples, this item still failed, propagate the error.
            final_status = current_item_status;
            break; // No need to try other items if one failed persistently
        }
    }
    return final_status;
} // end of staSensorReadAirTemp
