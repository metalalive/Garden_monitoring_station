#include "station_include.h"

// when a DHT11 starts to transmit 40-bit data, each bit includes:
// LOW-voltage-level that lasts 50 us, then transitioned to HIGH-voltage-level
// * that remains for 26 to 28 us (for transmitting data bit 0),
// * or remains for 70 us (for transmitting data bit 1).
// The parameter below indicates threshold of length of the HIGH-voltage-level (in microseconds),
// which can be used to traslate the signal changes to bit 0 or bit 1.
#define GMON_DHT11_BIT1_SIGNAL_LOW_REMAIN_US   40

// 40-bit data, each bit includes 2 signal transition (keep LOW for around 50 us, HIGH for 26-28 us, or 70 us)
// 3 signal transitions for response from DHT11
#define MAX_NUM_SIGNAL_TRANSITIONS  85

static void *dht11_signal_pin;

gMonStatus  staSensorInitAirTemp(void)
{
    dht11_signal_pin = NULL;
    return staSensorPlatformInitAirTemp(&dht11_signal_pin);
}

gMonStatus  staSensorDeInitAirTemp(void)
{
    return  staSensorPlatformDeInitAirTemp();
}

//// static volatile uint32_t  record_dht_signals[MAX_NUM_SIGNAL_TRANSITIONS];

gMonStatus  staSensorReadAirTemp(float *air_temp, float *air_humid)
{
    if(air_temp == NULL || air_humid == NULL) {
        return GMON_RESP_ERRARGS;
    }
    uint16_t sum_data_bits;
    uint8_t  record_dht_data[5];
    uint8_t  currstate;
    uint8_t  laststate;
    uint8_t  idx = 0;
    uint8_t  jdx = 0;
    gMonStatus status = GMON_RESP_OK;
    staPlatformPinSetDirection(dht11_signal_pin, GMON_PLATFORM_PIN_DIRECTION_OUT);
    staPlatformWritePin(dht11_signal_pin, GMON_PLATFORM_PIN_RESET);
    stationSysDelayMs(18); // stationSysDelayUs(18000);
    staPlatformWritePin(dht11_signal_pin, GMON_PLATFORM_PIN_SET);
    stationSysDelayUs(40);
    // setup critical section, in order to prevent the running task from being preempted
    // (or time tick interrupt since using up its time)
    stationSysEnterCritical();
    {
        status = staPlatformPinSetDirection(dht11_signal_pin, GMON_PLATFORM_PIN_DIRECTION_IN);
        if(status != GMON_RESP_OK) { goto done; }
        // check response signal from DHT11
        laststate = GMON_PLATFORM_PIN_SET;
        for(idx = 0; idx < MAX_NUM_SIGNAL_TRANSITIONS; idx++) {
            jdx = 0;
            while((currstate = staPlatformReadPin(dht11_signal_pin)) == laststate) {
                stationSysDelayUs(1);
                if(jdx++ >= 0xf0) { break; }
            }
            if(jdx >= 0xf0) { break; }
            laststate = currstate;
            //// record_dht_signals[idx] = jdx;
            if((idx >= 4) && ((idx % 2) == 0)) {
                record_dht_data[(idx - 4) >> 4] <<= 1;
                if(jdx > GMON_DHT11_BIT1_SIGNAL_LOW_REMAIN_US) {
                    record_dht_data[(idx - 4) >> 4] |= 1;
                }
            }
        } // end of for loop
        //  check received bytes, write to output if succeeded
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

