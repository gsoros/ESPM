#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <Preferences.h>

#include "task.h"

#define BATTERY_PIN 35

class Battery : public Task {
   public:
    Preferences *preferences;
    const char *preferencesNS;
    float corrF = 1.0;
    float voltage = 0.0;
    float pinVoltage = 0.0;
    uint8_t level = 0;

    void setup(Preferences *p,
               const char *preferencesNS = "Battery") {
        preferences = p;
        this->preferencesNS = preferencesNS;
        this->loadCalibration();
    }

    void loop(const ulong t) {
        measureVoltage();
        calculateLevel();
    }

    int calculateLevel() {
        level = map(voltage * 1000, 3200, 4200, 0, 100000) / 1000;
        return level;
    }

    float measureVoltage(bool useCorrection = true) {
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

    void loadCalibration() {
        if (!preferences->begin(preferencesNS, true))  // try ro mode
        {
            if (!preferences->begin(preferencesNS, false))  // open in rw mode to create ns
            {
                log_e("Preferences begin failed for '%s'\n", preferencesNS);
                return;
            }
        }
        if (!preferences->getBool("calibrated", false)) {
            log_e("Not calibrated");
            preferences->end();
            return;
        }
        corrF = preferences->getFloat("corrF", 1.0);
        preferences->end();
    }

    void calibrateTo(float realVoltage) {
        float measuredVoltage = measureVoltage(false);
        if (-0.001 < measuredVoltage && measuredVoltage < 0.001)
            return;
        corrF = realVoltage / measuredVoltage;
    }

    void saveCalibration() {
        if (!preferences->begin(preferencesNS, false)) {
            log_e("Preferences begin failed for '%s'\n", preferencesNS);
            return;
        }
        preferences->putFloat("corrF", corrF);
        preferences->putBool("calibrated", true);
        preferences->end();
    }

    void printCalibration() {
        Serial.printf("Battery correction factor: %f\n", corrF);
    }
};

#endif