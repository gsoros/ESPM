#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <Preferences.h>

#define BATTERY_PIN 35

class Battery
{
public:
    Preferences *preferences;
    const char *preferencesNS;
    float corrF = 1.0;
    float voltage = 0.0;
    uint8_t level = 0;
    ulong lastUpdate = 0;

    void setup(Preferences *p,
               const char *preferencesNS = "Battery")
    {
        preferences = p;
        this->preferencesNS = preferencesNS;
        this->loadCalibration();
    }

    void loop(const ulong t)
    {
        if (lastUpdate < t - 1000)
        {
            voltage = measureVoltage();
            level = map(voltage * 1000, 3200, 4200, 0, 100000) / 1000;
            lastUpdate = t;
        }
    }

    float measureVoltage(bool useCorrection = true)
    {
        int sum = 0;
        for (int i = 0; i < 10; i++)
        {
            sum += analogRead(BATTERY_PIN);
        }
        float uncorrected = map(sum, 0, 40950, 0, 33000) / 10000.0 * 2;
        if (!useCorrection)
            return uncorrected;
        return uncorrected * corrF;
    }

    void loadCalibration()
    {
        if (!preferences->begin(preferencesNS, true)) // try ro mode
        {
            if (!preferences->begin(preferencesNS, false)) // open in rw mode to create ns
            {
                log_e("Preferences begin failed for '%s'\n", preferencesNS);
                return;
            }
        }
        if (!preferences->getBool("calibrated", false))
        {
            log_e("Not calibrated");
            preferences->end();
            return;
        }
        corrF = preferences->getFloat("corrF", 1.0);
        preferences->end();
    }

    void calibrateTo(float realVoltage)
    {
        if (!preferences->begin(preferencesNS, false))
        {
            log_e("Preferences begin failed for '%s'\n", preferencesNS);
            return;
        }
        float measuredVoltage = measureVoltage(false);
        if (0.0 == measuredVoltage)
            measuredVoltage = 3.8;
        preferences->putFloat("corrF", realVoltage / measuredVoltage);
        preferences->putBool("calibrated", true);
        preferences->end();
    }
};

#endif