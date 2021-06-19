#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

#define BATTERY_PIN 35

class Battery
{
public:
    const float correctionFactor = 4.16 / 3.92;
    float voltage = 0.0;
    uint8_t level = 0;
    ulong lastUpdate = 0;

    void setup()
    {
    }

    void loop(const ulong t)
    {
        if (lastUpdate < t - 1000) {
            voltage = measureVoltage();
            level = map(voltage * 1000, 3200, 4200, 0, 100000) / 1000;
            lastUpdate = t;
        }
    }

    float measureVoltage()
    {
        int sum = 0;
        for (int i = 0; i < 10; i++) {
            sum += analogRead(BATTERY_PIN);
        }
        return map(sum, 0, 40950, 0, 33000) / 10000.0 * 2 * correctionFactor;
    }
};

#endif