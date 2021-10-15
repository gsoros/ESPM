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
    int mdmStrainThreshold = MDM_STRAIN_DEFAULT_THRESHOLD;
    int mdmStrainThresLow = MDM_STRAIN_DEFAULT_THRES_LOW;

    void setup(const gpio_num_t doutPin,
               const gpio_num_t sckPin,
               Preferences *p,
               const char *preferencesNS = "STRAIN");

    void loop();

    float value(bool clearBuffer = false);
    float liveValue();
    bool dataReady();
    void sleep();
    void setMdmStrainThreshold(int threshold);
    void setMdmStrainThresLow(int threshold);
    int calibrateTo(float knownMass);  // calibrate to a known mass in kg
    void printCalibration();
    void loadCalibration();
    void saveCalibration();
    void tare();

   private:
    CircularBuffer<float, STRAIN_RINGBUF_SIZE> _measurementBuf;
    bool _halfRevolution = false;
};

#endif