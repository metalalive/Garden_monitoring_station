#include <assert.h>
#include "station_include.h"

// when a DHT11 starts to transmit 40-bit data, each bit includes:
// LOW-voltage-level that lasts 50 us, then transitioned to HIGH-voltage-level
// * that remains for 26 to 28 us (for transmitting data bit 0),
// * or remains for 70 us (for transmitting data bit 1).
static void *dht11_signal_pin;

gMonStatus  staSensorInitAirTemp(void) {
    dht11_signal_pin = NULL;
    return staSensorPlatformInitAirTemp(&dht11_signal_pin);
}

gMonStatus  staSensorDeInitAirTemp(void) {
    return  staSensorPlatformDeInitAirTemp();
}

// Helper function to measure the duration of a pin state
static gMonStatus measureVerifyPulse(
    uint8_t expect_init_state, uint16_t refpoint_pulse_us, uint16_t deviation_us, uint16_t *actual_pulse_us
) {
    uint16_t  pulse_us_read = 0;
    uint8_t   state_low2high = 0, state_expected = 0, pulse_range_fit = 0;
    assert(refpoint_pulse_us > deviation_us);
    gMonStatus status = staPlatformMeasurePulse(dht11_signal_pin, &state_low2high, &pulse_us_read);
    if (status == GMON_RESP_OK) {
        uint16_t  maxlimit = refpoint_pulse_us + deviation_us;
        uint16_t  minlimit = refpoint_pulse_us - deviation_us;
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
 * This function interacts with the DHT11 sensor through the `dht11_signal_pin`
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
static gMonStatus readDht11SensorData(uint8_t data[5]) {
    // After MCU pulls the line HIGH for 20-40us and releases it (sets pin to input mode),
    // the DHT11 should respond by pulling the line LOW for ~80us.
    gMonStatus status = GMON_RESP_OK;
    uint16_t pulse_us = 0, idx;
    // 1. check whether to sample DHT11's LOW acknowledgment pulse with
    // positive-integer length time.
    status = measureVerifyPulse(GMON_PLATFORM_PIN_RESET, 80, 40, NULL);
    if (status != GMON_RESP_OK) return status;

    // 2. Check for DHT11's 80us HIGH acknowledgment pulse
    status = measureVerifyPulse(GMON_PLATFORM_PIN_SET, 80, 8, NULL);
    if (status != GMON_RESP_OK) return status;

    // 3. Read 40 bits of data
    for (idx = 0; idx < 40; idx++) {
        // Each data bit transmission starts with a 50us LOW pulse
        status = measureVerifyPulse(GMON_PLATFORM_PIN_RESET, 50, 7, NULL);
        if (status != GMON_RESP_OK) return status;
        // The length of the subsequent HIGH pulse determines the bit value
        status = measureVerifyPulse(GMON_PLATFORM_PIN_SET, 45, 35, &pulse_us);
        if (status != GMON_RESP_OK) return status;
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
    status = measureVerifyPulse(GMON_PLATFORM_PIN_RESET, 50, 7, NULL);
    // After this, the bus is passively pulled HIGH by a resistor.
    return status;
} // end of readDht11SensorData

// TODO, FIXME :
// modify function signature for introducing external reliable reference positive-integer
// which can calibrate the sensor here .
gMonStatus  staSensorReadAirTemp(float *air_temp, float *air_humid)
{
    if(air_temp == NULL || air_humid == NULL) {
        return GMON_RESP_ERRARGS;
    }
    gMonStatus status = GMON_RESP_OK;
    // TODO, set up power gate to this device
    uint16_t sum_data_bits;
    uint8_t  record_dht_data[5] = {0};
    stationSysDelayMs(1000); // Wait 1 second after power on without sending any data
    staPlatformPinSetDirection(dht11_signal_pin, GMON_PLATFORM_PIN_DIRECTION_OUT);
    staPlatformWritePin(dht11_signal_pin, GMON_PLATFORM_PIN_RESET);
    stationSysDelayMs(18); // keep start-out signal from MCU for at least 18 ms
    // setup critical section, in order to prevent the running task from being preempted
    // (or time tick interrupt since using up its time)
    stationSysEnterCritical();
    {
        staPlatformWritePin(dht11_signal_pin, GMON_PLATFORM_PIN_SET);
        // datasheet suggests 20 to 40us waiting time on MCU side, but DHT can still acknowledgment
        // shorter high-voltage pulse and respond. TODO: test the behavior
        stationSysDelayUs(2);
        status = staPlatformPinSetDirection(dht11_signal_pin, GMON_PLATFORM_PIN_DIRECTION_IN);
        if(status != GMON_RESP_OK) { goto done; }
        // check response signal from DHT11
        status = readDht11SensorData(record_dht_data);
        if(status != GMON_RESP_OK) { goto done; }

        sum_data_bits = (record_dht_data[0] + record_dht_data[1] + record_dht_data[2] + record_dht_data[3]) & 0xff;
        if(sum_data_bits != record_dht_data[4]) {
            status = GMON_RESP_SENSOR_FAIL;
            goto done;
        }
        *air_humid = record_dht_data[0] + record_dht_data[1] / 10.f;
        *air_temp  = record_dht_data[2] + record_dht_data[3] / 10.f;
    }
done:
    stationSysExitCritical();
    return status;
} // end of staSensorReadAirTemp

