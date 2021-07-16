#ifndef STRAIN_H
#define STRAIN_H

#include <Arduino.h>
#include <HX711_ADC.h>
#include <Preferences.h>
#include <driver/rtc_io.h>

#include "haspreferences.h"
#include "task.h"

class Strain : public Task, public HasPreferences {
   public:
    HX711_ADC *device;
    gpio_num_t doutPin;
    gpio_num_t sckPin;

    void setup(const gpio_num_t doutPin,
               const gpio_num_t sckPin,
               Preferences *p,
               const char *preferencesNS = "STRAIN");

    void loop();

    float measurement(bool unsetDataReadyFlag = false);
    bool dataReady();
    void sleep();
    int calibrateTo(float knownMass);  // calibrate to a known mass in kg
    void printCalibration();
    void loadCalibration();
    void saveCalibration();
    void tare();

   private:
    float _measurement = 0.0;
    bool _dataReady = false;
};

#endif