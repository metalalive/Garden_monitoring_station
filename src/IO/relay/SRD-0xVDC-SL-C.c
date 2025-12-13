#include "station_include.h"

static void *srd0xvdc_trig_pin_pump;
static void *srd0xvdc_trig_pin_fan;
static void *srd0xvdc_trig_pin_bulb;

gMonStatus staActuatorInitPump(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    srd0xvdc_trig_pin_pump = NULL;
    if (dev == NULL) {
        status = GMON_RESP_ERRARGS;
        goto done;
    }
    status = staActuatorInitGenericPump(dev);
    if (status != GMON_RESP_OK)
        goto done;
    status = staActuatorPlatformInitPump(&srd0xvdc_trig_pin_pump);
    XASSERT(srd0xvdc_trig_pin_pump != NULL);
    if (status != GMON_RESP_OK)
        goto done;
    status = staPlatformPinSetDirection(srd0xvdc_trig_pin_pump, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus staActuatorDeinitPump(void) { return staActuatorDeinitGenericPump(); }

gMonStatus staActuatorInitFan(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    srd0xvdc_trig_pin_fan = NULL;
    if (dev == NULL) {
        status = GMON_RESP_ERRARGS;
        goto done;
    }
    status = staActuatorInitGenericFan(dev);
    if (status != GMON_RESP_OK)
        goto done;
    status = staActuatorPlatformInitFan(&srd0xvdc_trig_pin_fan);
    XASSERT(srd0xvdc_trig_pin_fan != NULL);
    if (status != GMON_RESP_OK)
        goto done;
    status = staPlatformPinSetDirection(srd0xvdc_trig_pin_fan, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus staActuatorDeinitFan(void) { return staActuatorDeinitGenericFan(); }

gMonStatus staActuatorInitBulb(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    srd0xvdc_trig_pin_bulb = NULL;
    if (dev == NULL) {
        status = GMON_RESP_ERRARGS;
        goto done;
    }
    status = staActuatorInitGenericBulb(dev);
    if (status != GMON_RESP_OK)
        goto done;
    status = staActuatorPlatformInitBulb(&srd0xvdc_trig_pin_bulb);
    XASSERT(srd0xvdc_trig_pin_bulb != NULL);
    if (status != GMON_RESP_OK)
        goto done;
    status = staPlatformPinSetDirection(srd0xvdc_trig_pin_bulb, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus staActuatorDeinitBulb(void) { return staActuatorDeinitGenericBulb(); }

gMonStatus staActuatorTrigPump(gMonActuator_t *dev, gmonEvent_t *evt, gMonSensorMeta_t *sensor) {
    gMonStatus status = GMON_RESP_OK;
    if (dev == NULL || evt == NULL || sensor == NULL) {
        return GMON_RESP_ERRARGS;
    } else if (evt->event_type != GMON_EVENT_SOIL_MOISTURE_UPDATED) {
        return GMON_RESP_ERRARGS;
    }
    int soil_moist = 0;
    status = staActuatorAggregateU32(evt, dev, &soil_moist);
    // output device starts working until either max working time reached or actual read
    // value lesser than threshold larger input value means dry soil
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (dev->threshold < soil_moist))
                                        ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (dev->status != dev_status) {
        dev->status = dev_status;
        uint8_t pin_state =
            (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET : GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(srd0xvdc_trig_pin_pump, pin_state);
    }
    return status;
}

gMonStatus staActuatorTrigFan(gMonActuator_t *dev, gmonEvent_t *evt, gMonSensorMeta_t *sensor) {
    gMonStatus status = GMON_RESP_OK;
    if (dev == NULL || evt == NULL || sensor == NULL) {
        return GMON_RESP_ERRARGS;
    } else if (evt->event_type != GMON_EVENT_AIR_TEMP_UPDATED) {
        return GMON_RESP_ERRARGS;
    }
    int air_cond = 0;
    status = staActuatorAggregateAirCond(evt, dev, &air_cond);
    // output device starts working until either max working time reached or actual read
    // value lesser than threshold
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (dev->threshold < air_cond))
                                        ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (dev->status != dev_status) {
        dev->status = dev_status;
        uint8_t pin_state =
            (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET : GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(srd0xvdc_trig_pin_fan, pin_state);
    }
    return status;
}

gMonStatus staActuatorTrigBulb(gMonActuator_t *dev, gmonEvent_t *evt, gMonSensorMeta_t *sensor) {
    if (dev == NULL || evt == NULL || sensor == NULL) {
        return GMON_RESP_ERRARGS;
    } else if (evt->event_type != GMON_EVENT_LIGHTNESS_UPDATED) {
        return GMON_RESP_ERRARGS;
    }
    gMonStatus status = GMON_RESP_OK;
    // TODO: finish implementation, maximum working time per day for a bulb must be estimate,
    // in case that the plant you're growing still needs more growing light of a day.
    int lightness = 0;
    status = staActuatorAggregateU32(evt, dev, &lightness);
    // smaller value means less natural light
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (dev->threshold > lightness))
                                        ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (dev->status != dev_status) {
        dev->status = dev_status;
        uint8_t pin_state =
            (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET : GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(srd0xvdc_trig_pin_bulb, pin_state);
    }
    return status;
}
