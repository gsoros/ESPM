#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <Preferences.h>

#include "definitions.h"
#include "haspreferences.h"
#include "task.h"

#ifndef BATTERY_PIN
#define BATTERY_PIN GPIO_NUM_35
#endif

class Battery : public Task, public HasPreferences {
   public:
    float corrF = 1.0;
    float voltage = 0.0;
    float pinVoltage = 0.0;
    uint8_t level = 0;

    void setup(Preferences *p);
    void loop();
    int calculateLevel();
    float measureVoltage();
    float measureVoltage(bool useCorrection);
    void loadCalibration();
    void calibrateTo(float realVoltage);
    void saveCalibration();
    void printCalibration();
};

#endif