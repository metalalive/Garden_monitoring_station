#include "station_include.h"

gMonStatus staActuatorInitPump(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    if (dev == NULL)
        return GMON_RESP_ERRARGS;
    dev->lowlvl = NULL;
    status = staActuatorInitGenericPump(dev);
    if (status != GMON_RESP_OK)
        goto done;
    status = staActuatorPlatformInitPump(&dev->lowlvl);
    if (status != GMON_RESP_OK)
        goto done;
    XASSERT(dev->lowlvl != NULL);
    status = staPlatformPinSetDirection(dev->lowlvl, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus staActuatorDeinitPump(void) { return staActuatorDeinitGenericPump(); }

gMonStatus staActuatorInitFan(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    if (dev == NULL) {
        status = GMON_RESP_ERRARGS;
        goto done;
    }
    dev->lowlvl = NULL;
    status = staActuatorInitGenericFan(dev);
    if (status != GMON_RESP_OK)
        goto done;
    status = staActuatorPlatformInitFan(&dev->lowlvl);
    XASSERT(dev->lowlvl != NULL);
    if (status != GMON_RESP_OK)
        goto done;
    status = staPlatformPinSetDirection(dev->lowlvl, GMON_PLATFORM_PIN_DIRECTION_OUT);
done:
    return status;
}

gMonStatus staActuatorDeinitFan(void) { return staActuatorDeinitGenericFan(); }

gMonStatus staActuatorInitBulb(gMonActuator_t *dev) {
    gMonStatus status = GMON_RESP_OK;
    if (dev == NULL) {
        status = GMON_RESP_ERRARGS;
        goto done;
    }
    dev->lowlvl = NULL;
    status = staActuatorInitGenericBulb(dev);
    if (status != GMON_RESP_OK)
        goto done;
    status = staActuatorPlatformInitBulb(&dev->lowlvl);
    if (status != GMON_RESP_OK)
        goto done;
    XASSERT(dev->lowlvl != NULL);
    status = staPlatformPinSetDirection(dev->lowlvl, GMON_PLATFORM_PIN_DIRECTION_OUT);
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
        status = staPlatformWritePin(dev->lowlvl, pin_state);
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
        status = staPlatformWritePin(dev->lowlvl, pin_state);
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
        status = staPlatformWritePin(dev->lowlvl, pin_state);
    }
    return status;
}
