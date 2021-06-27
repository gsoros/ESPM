#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <Preferences.h>

#define BATTERY_PIN 35

class Battery {
   public:
    Preferences *preferences;
    const char *preferencesNS;
    float corrF = 1.0;
    float voltage = 0.0;
    uint8_t level = 0;
    ulong lastUpdate = 0;

    void setup(Preferences *p,
               const char *preferencesNS = "Battery") {
        preferences = p;
        this->preferencesNS = preferencesNS;
        this->loadCalibration();
    }

    void loop(const ulong t) {
        if (lastUpdate < t - 1000) {
            voltage = measureVoltage();
            level = map(voltage * 1000, 3200, 4200, 0, 100000) / 1000;
            lastUpdate = t;
        }
    }

    float measureVoltage(bool useCorrection = true) {
        ulong t = millis();
        ulong lt = t;
        uint8_t samplesNeeded = 10;
        uint8_t sampleCount = 0;
        uint32_t sum = 0;
        while (sampleCount < samplesNeeded) {
            lt = millis();
            if (lt >= t + 5) {
                sum += analogRead(BATTERY_PIN);
                sampleCount++;
                t = lt;
            }
            yield();
        }
        uint32_t readMax = ((2 ^ 12) - 1) * samplesNeeded;  // 12bit adc
        if (sum == readMax) log_e("overflow");
        float uncorrected =
            map(
                sum,
                0,
                readMax,
                0,
                330 * samplesNeeded) /  // 3.3V
            samplesNeeded /
            20000.0;  // float division
        //Serial.printf("Batt pin measured: %f\n", uncorrected);
        if (!useCorrection)
            return uncorrected;
        return uncorrected * corrF;
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