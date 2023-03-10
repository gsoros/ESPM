#ifdef FEATURE_TEMPERATURE
#include "temperature.h"
#include "board.h"

#ifndef TEMPERATURE_PIN
#error TEMPERATURE_PIN missing
#endif

Temperature::Temperature() {}

Temperature::~Temperature() {
    if (crankSensor) delete crankSensor;
    if (tc) delete tc;
}

void Temperature::setup() {
    uint32_t heap = esp_get_free_heap_size();
    int stack = (int)uxTaskGetStackHighWaterMark(NULL);
    crankSensor = new Sensor(                                    // single sensor: exclusive mode
        TEMPERATURE_PIN,                                         // pin
        "crank",                                                 // label
        11,                                                      // resolution
        1.0f,                                                    // update frequency
        [this](Sensor *sensor) { onSensorValueChange(sensor); }  // callback
    );
#ifdef FEATURE_BLE_SERVER
    crankSensor->addBleService(&board.bleServer);
#endif
    tc = new TemperatureCompensation();
    log_d("heap used: %d, stack used: %d",
          heap - esp_get_free_heap_size(),
          stack - (int)uxTaskGetStackHighWaterMark(NULL));
}

void Temperature::begin() {
    if (!crankSensor)
        log_e("crank is null");
    else
        crankSensor->begin();
}

void Temperature::onSensorValueChange(Sensor *sensor) {
    if (sensor->address == crankSensor->address)
        setCompensation(sensor->value);
    log_i("%s temp: %.2f°C, compensation: %.1fkg",
          sensor->label,
          sensor->value,
          getCompensation());
}

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

    float keyResolution = tc->getKeyResolution();
    if (keyResolution <= 0.0) {
        log_e("invalid key resolution");
        compensation = 0.0f;
        return;
    }
    // TODO interpolate to neighbors etc
    // index = (temp - offset) / resolution
    uint16_t index = (uint16_t)((temperature - (float)tc->getKeyOffset()) / keyResolution);
    if (!tc->validIndex(index)) {
        log_e("could not find index for %.2f˚C", temperature);
    }
    int8_t skew = tc->getValue(index);
    log_d("index: %d, skew: %d", index, skew);
    if (skew == tc->valueUnset)
        compensation = 0.0f;
    else
        compensation = (float)skew * tc->getValueResolution();
}

/// @brief Get current temperature compensation value for the weight scale.
/// @return value in kg
float Temperature::getCompensation() {
    // if (!crankSensor) return 0.0f;
    // TODO check crankSensor->lastUpdate etc
    return compensation - compensationOffset;
}

#endif  // FEATURE_TEMPERATURE