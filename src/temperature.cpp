#ifdef FEATURE_TEMPERATURE
#include "temperature.h"
#include "board.h"

#ifndef TEMPERATURE_PIN
#error TEMPERATURE_PIN missing
#endif

Temperature::Temperature() {}

Temperature::~Temperature() {
    if (crankSensor) delete crankSensor;
#ifdef FEATURE_TEMPERATURE_COMPENSATION
    if (tc) delete tc;
#endif
}

#ifdef FEATURE_TEMPERATURE_COMPENSATION
void Temperature::setup(TC *tc)
#else
void Temperature::setup()
#endif  // FEATURE_TEMPERATURE_COMPENSATION
{
    uint32_t heap = esp_get_free_heap_size();
    int stack = (int)uxTaskGetStackHighWaterMark(NULL);
    crankSensor = new Sensor(                                    // single sensor: exclusive mode
        TEMPERATURE_PIN,                                         // pin
        "tCrank",                                                // label
        11,                                                      // resolution
        0.2f,                                                    // update frequency
        [this](Sensor *sensor) { onSensorValueChange(sensor); }  // callback
    );
#ifdef FEATURE_BLE_SERVER
    crankSensor->addBleService(&board.bleServer);
#endif
#ifdef FEATURE_TEMPERATURE_COMPENSATION
    this->tc = tc;
#endif
    log_d("heap used: %d, stack used: %d",
          heap - esp_get_free_heap_size(),
          stack - (int)uxTaskGetStackHighWaterMark(NULL));
}

void Temperature::begin() {
    if (!crankSensor)
        log_e("no crank sensor");
    else
        crankSensor->begin();
}

void Temperature::onSensorValueChange(Sensor *sensor) {
#ifdef FEATURE_TEMPERATURE_COMPENSATION
    if (sensor->address == crankSensor->address)
        setCompensation(sensor->value);
    log_i("%s: %.2f°C, compensation: %.1fkg%s",
          sensor->label,
          sensor->value,
          getCompensation(),
          tc->enabled ? "" : " (disabled)");
#else
    log_i("%s: %.2f°C",
          sensor->label,
          sensor->value);
#endif
}

#ifdef FEATURE_TEMPERATURE_COMPENSATION
/// @brief set offset to current compensation value, e.g. after tare
/// @return
bool Temperature::setCompensationOffset() {
    compensationOffset = compensation;
    return true;
}

void Temperature::setCompensation(float temperature) {
    if (!tc) {
        log_e("no table");
        compensation = 0.0f;
        return;
    }
    if (!tc->enabled) {
        // log_d("tc not enabled");
        compensation = 0.0f;
        return;
    }
    float keyResolution = tc->getKeyResolution();
    if (keyResolution <= 0.0f) {
        log_e("invalid key resolution");
        compensation = 0.0f;
        return;
    }
    // TODO interpolate to neighbors etc
    // index = (temp - offset) / resolution
    uint16_t index = (uint16_t)((temperature - (float)tc->getKeyOffset()) / keyResolution);
    if (!tc->validIndex(index)) {
        log_e("could not find index for %.2f˚C", temperature);
        return;
    }
    int8_t skew = tc->getValue(index);
    log_d("index: %d, skew: %d", index, skew);
    compensation = (float)skew * tc->getValueResolution();
}

/// @brief Get current TC value for the weight scale, if TC is enabled
/// @return value in kg
float Temperature::getCompensation() {
    if (!tc->enabled) return 0.0f;
    // if (!crankSensor) return 0.0f;
    // TODO check crankSensor->lastUpdate etc
    return compensation - compensationOffset;
}
#endif  // FEATURE_TEMPERATURE_COMPENSATION

#endif  // FEATURE_TEMPERATURE