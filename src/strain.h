#ifndef STRAIN_H
#define STRAIN_H

#include <Arduino.h>
#include <HX711_ADC.h>
#include <CircularBuffer.h>
#include <Preferences.h>
#include <driver/rtc_io.h>

#include "haspreferences.h"
#include "task.h"

#ifndef STRAIN_RINGBUF_SIZE
#define STRAIN_RINGBUF_SIZE 512  // circular buffer size
#endif

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

    float value(bool clearBuffer = false);
    bool dataReady();
    void sleep();
    int calibrateTo(float knownMass);  // calibrate to a known mass in kg
    void printCalibration();
    void loadCalibration();
    void saveCalibration();
    void tare();

   private:
    CircularBuffer<float, STRAIN_RINGBUF_SIZE> _measurementBuf;
};

#endif