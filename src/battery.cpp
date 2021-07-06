#include "battery.h"
#include "board.h"

void Battery::setup(Preferences *p) {
    preferencesSetup(p, "Battery");
    this->loadCalibration();
}

void Battery::loop(const ulong t) {
    measureVoltage();
    calculateLevel();
    board.ble.batteryLevel = level;
}

int Battery::calculateLevel() {
    level = map(voltage * 1000, 3200, 4200, 0, 100000) / 1000;
    return level;
}

float Battery::measureVoltage() {
    return measureVoltage(true);
}
float Battery::measureVoltage(bool useCorrection) {
    uint32_t sum = 0;
    uint8_t samples;
    for (samples = 1; samples <= 10; samples++) {
        sum += analogRead(BATTERY_PIN);
        vTaskDelay(10);
    }
    uint32_t readMax = 4095 * samples;  // 2^12 - 1 (12bit adc)
    if (sum == readMax) log_e("overflow");
    pinVoltage =
        map(
            sum,
            0,
            readMax,
            0,
            330 * samples) /  // 3.3V
        samples /
        100.0;  // double division
    //log_i("Batt pin measured: (%d) = %fV\n", sum / samples, pinVoltage);
    voltage = pinVoltage * corrF;
    return useCorrection ? voltage : pinVoltage;
}

void Battery::loadCalibration() {
    if (!preferencesStartLoad()) return;
    if (!preferences->getBool("calibrated", false)) {
        log_e("Not calibrated");
        preferencesEnd();
        return;
    }
    corrF = preferences->getFloat("corrF", 1.0);
    preferencesEnd();
}

void Battery::calibrateTo(float realVoltage) {
    float measuredVoltage = measureVoltage(false);
    if (-0.001 < measuredVoltage && measuredVoltage < 0.001)
        return;
    corrF = realVoltage / measuredVoltage;
}

void Battery::saveCalibration() {
    if (!preferencesStartSave()) return;
    preferences->putFloat("corrF", corrF);
    preferences->putBool("calibrated", true);
    preferencesEnd();
}

void Battery::printCalibration() {
    Serial.printf("Battery correction factor: %f\n", corrF);
}
