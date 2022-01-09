#include "battery.h"
#include "board.h"

void Battery::setup(Preferences *p) {
    preferencesSetup(p, "Battery");
    this->loadSettings();
}

void Battery::loop() {
    measureVoltage();
    calculateLevel();
}

int Battery::calculateLevel() {
    float voltageTmp = 0.0;
    for (decltype(_voltageBuf)::index_t i = 0; i < _voltageBuf.size(); i++) {
        voltageTmp += _voltageBuf[i] / _voltageBuf.size();
    }
    if (voltageTmp < 3.2F)
        voltageTmp = 3.2;
    else if (voltageTmp > 4.2F)
        voltageTmp = 4.2;
    level = map(voltageTmp * 1000, 3200, 4200, 0, 100000) / 1000;
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
        delay(1);
    }
    uint32_t readMax = 4095 * samples;  // 2^12 - 1 (12bit adc)
    if (sum == readMax) Serial.printf("[Battery] Overflow");
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
    _voltageBuf.push(voltage);
    return useCorrection ? voltage : pinVoltage;
}

void Battery::loadSettings() {
    if (!preferencesStartLoad()) return;
    if (!preferences->getBool("calibrated", false)) {
        Serial.println("[Battery] Not calibrated");
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

void Battery::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putFloat("corrF", corrF);
    preferences->putBool("calibrated", true);
    preferencesEnd();
}

void Battery::printSettings() {
    Serial.printf("[Battery] Correction factor: %f\n", corrF);
}
