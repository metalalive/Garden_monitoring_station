#include "station_include.h"

static void *srd0xvdc_trig_pin_pump;
static void *srd0xvdc_trig_pin_fan;
static void *srd0xvdc_trig_pin_bulb;

gMonStatus  staActuatorInitPump(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    srd0xvdc_trig_pin_pump = NULL;
    if(dev == NULL) { status = GMON_RESP_ERRARGS; goto done; }
    status = staActuatorInitGenericPump(dev);
    if(status != GMON_RESP_OK) { goto done; }
    status = staActuatorPlatformInitPump(&srd0xvdc_trig_pin_pump);
    XASSERT(srd0xvdc_trig_pin_pump != NULL);
    if(status != GMON_RESP_OK) { goto done; }
    status = staPlatformPinSetDirection(srd0xvdc_trig_pin_pump, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus  staActuatorDeinitPump(void) {
    return  staActuatorDeinitGenericPump();
}

gMonStatus  staActuatorInitFan(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    srd0xvdc_trig_pin_fan = NULL;
    if(dev == NULL) { status = GMON_RESP_ERRARGS; goto done; }
    status = staActuatorInitGenericFan(dev);
    if(status != GMON_RESP_OK) { goto done; }
    status = staActuatorPlatformInitFan(&srd0xvdc_trig_pin_fan);
    XASSERT(srd0xvdc_trig_pin_fan != NULL);
    if(status != GMON_RESP_OK) { goto done; }
    status = staPlatformPinSetDirection(srd0xvdc_trig_pin_fan, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus  staActuatorDeinitFan(void) {
    return  staActuatorDeinitGenericFan();
}

gMonStatus  staActuatorInitBulb(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    srd0xvdc_trig_pin_bulb = NULL;
    if(dev == NULL) { status = GMON_RESP_ERRARGS; goto done; }
    status = staActuatorInitGenericBulb(dev);
    if(status != GMON_RESP_OK) { goto done; }
    status = staActuatorPlatformInitBulb(&srd0xvdc_trig_pin_bulb);
    XASSERT(srd0xvdc_trig_pin_bulb != NULL);
    if(status != GMON_RESP_OK) { goto done; }
    status = staPlatformPinSetDirection(srd0xvdc_trig_pin_bulb, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus  staActuatorDeinitBulb(void) {
    return  staActuatorDeinitGenericBulb();
}

gMonStatus  staActuatorTrigPump(gMonActuator_t *dev, unsigned int soil_moist, gMonSensor_t *sensor) {
    gMonStatus status = GMON_RESP_OK;
    gMonActuatorStatus dev_status = GMON_OUT_DEV_STATUS_OFF;
    uint8_t  pin_state = 0;
    XASSERT(dev != NULL);
    // output device starts working until either max working time reached or actual read value lesser than threshold
    // larger input value means dry soil
    dev_status = (dev->threshold < soil_moist) ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms): GMON_OUT_DEV_STATUS_OFF;
    if(dev->status != dev_status) {
        dev->status = dev_status;
        pin_state = (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET: GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(srd0xvdc_trig_pin_pump, pin_state);
    }
    return status;
}

gMonStatus  staActuatorTrigFan(gMonActuator_t *dev, float air_temp, gMonSensor_t *sensor) {
    gMonStatus status = GMON_RESP_OK;
    gMonActuatorStatus dev_status = GMON_OUT_DEV_STATUS_OFF;
    float    threshold = 0.f;
    uint8_t  pin_state = 0;
    XASSERT(dev != NULL);
    threshold = (float)dev->threshold  * 1.f;
    // output device starts working until either max working time reached or actual read value lesser than threshold
    dev_status = (threshold < air_temp) ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms): GMON_OUT_DEV_STATUS_OFF;
    if(dev->status != dev_status) {
        dev->status = dev_status;
        pin_state = (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET: GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(srd0xvdc_trig_pin_fan, pin_state);
    }
    return status;
}

gMonStatus  staActuatorTrigBulb(gMonActuator_t *dev, unsigned int lightness, gMonSensor_t *sensor) {
    // TODO: finish implementation, maximum working time per day for a bulb must be estimate,
    // in case that the plant you're growing still needs more growing light of a day.
    gMonStatus status = GMON_RESP_OK;
    gMonActuatorStatus dev_status = GMON_OUT_DEV_STATUS_OFF;
    uint8_t  pin_state = 0;
    XASSERT(dev != NULL);
    // smaller value means less natural light
    dev_status = (dev->threshold > lightness) ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms): GMON_OUT_DEV_STATUS_OFF;
    if(dev->status != dev_status) {
        dev->status = dev_status;
        pin_state = (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET: GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(srd0xvdc_trig_pin_bulb, pin_state);
    }
    return status;
}
