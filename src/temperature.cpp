#ifdef FEATURE_TEMPERATURE
#include "temperature.h"
#include "board.h"

#ifndef TEMPERATURE_PIN
#error TEMPERATURE_PIN missing
#endif

Temperature::Temperature() {}

Temperature::~Temperature() {
    if (crankSensor) delete crankSensor;
}

void Temperature::setup() {
    for (uint16_t i = 0; i < tcTableSize(); i++)  // tcTableUnsetValue(i);
        tcTableSetValue(i, (int8_t)random(INT8_MIN, INT8_MAX + 1));

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
}

void Temperature::begin() {
    if (!crankSensor)
        log_e("crank is null");
    else
        crankSensor->begin();
}

void Temperature::onSensorValueChange(Sensor *sensor) {
    if (sensor->address == crankSensor->address)
        tcSetCorrection(sensor->value);
    log_i("Sensor: %s, Temp: %.2f°C, Correction: %.1fkg",
          sensor->label,
          sensor->value,
          tcGetCorrection());
}

size_t Temperature::tcTableSize() {
    return sizeof(tcTable) / sizeof(tcTable[0]);
}

bool Temperature::tcTableValidIndex(uint16_t index) {
    return index < tcTableSize() - 1;
}

int8_t Temperature::tcTableGetValue(uint16_t index) {
    if (!tcTableValidIndex(index)) {
        log_e("index %d out of range", index);
        return TC_VALUE_EMPTY;
    }
    return tcTable[index];
}

bool Temperature::tcTableSetValue(uint16_t index, int8_t value) {
    if (!tcTableValidIndex(index)) {
        log_e("index %d out of range", index);
        return false;
    }
    tcTable[index] = value;
    return true;
}

bool Temperature::tcTableUnsetValue(uint16_t index) {
    return tcTableSetValue(index, TC_VALUE_EMPTY);
}

void Temperature::tcSetCorrection(float temperature) {
    // TODO interpolate to neighbors etc
    uint16_t index = (uint16_t)temperature - TC_TABLE_OFFSET;
    int8_t skew = tcTableGetValue(index);
    log_d("index: %d, skew: %d", index, skew);
    if (skew == TC_VALUE_EMPTY)
        correction = 0.0;
    else
        correction = (float)skew * TC_TABLE_MAGNITUDE;
}

/// @brief Get current temperature correction value for the weight scale.
/// @return value in kg
float Temperature::tcGetCorrection() {
    // if (!crankSensor) return 0.0f;
    // TODO check crankSensor->lastUpdate etc
    return correction;
}

#endif  // FEATURE_TEMPERATURE