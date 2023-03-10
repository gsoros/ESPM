#ifdef FEATURE_TEMPERATURE

#include "temperature_compensation.h"

TemperatureCompensation::TemperatureCompensation(
    uint16_t size,
    int16_t keyOffset,
    float keyResolution,
    float valueResolution) {
    setSize(size);
    setKeyOffset(keyOffset);
    setKeyResolution(keyResolution);
    setValueResolution(valueResolution);
    for (uint16_t i = 0; i < getSize(); i++)
        unsetValue(i);
    // setValue(i, (int8_t)random(valueUnset, INT8_MAX + 1));
}

TemperatureCompensation::~TemperatureCompensation() {
    free(values);
}

bool TemperatureCompensation::setSize(uint16_t size) {
    if (nullptr == values)
        values = (int8_t *)malloc(size * sizeof(int8_t));
    else
        values = (int8_t *)realloc(values, size * sizeof(int8_t));
    if (nullptr == values) {
        log_e("memory allocation failed");
        this->size = 0;
        return false;
    }
    this->size = size;
    return true;
}

uint16_t TemperatureCompensation::getSize() {
    return size;
}

int16_t TemperatureCompensation::getKeyOffset() {
    return keyOffset;
}

bool TemperatureCompensation::setKeyOffset(int16_t offset) {
    keyOffset = offset;
    return true;
}

float TemperatureCompensation::getKeyResolution() {
    return keyResolution;
}

bool TemperatureCompensation::setKeyResolution(float resolution) {
    keyResolution = resolution;
    return true;
}

float TemperatureCompensation::getValueResolution() {
    return valueResolution;
}

bool TemperatureCompensation::setValueResolution(float resolution) {
    valueResolution = resolution;
    return true;
}

bool TemperatureCompensation::validIndex(uint16_t index) {
    return index < getSize();
}

int8_t TemperatureCompensation::getValue(uint16_t index) {
    if (!validIndex(index)) {
        log_e("index %d out of range", index);
        return valueUnset;
    }
    return values[index];
}

bool TemperatureCompensation::setValue(uint16_t index, int8_t value) {
    if (!validIndex(index)) {
        log_e("index %d out of range", index);
        return false;
    }
    values[index] = value;
    return true;
}

bool TemperatureCompensation::unsetValue(uint16_t index) {
    return setValue(index, valueUnset);
}

#endif  // FEATURE_TEMPERATURE