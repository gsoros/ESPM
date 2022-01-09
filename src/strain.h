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

class Strain : public Task,
               public HasPreferences {
   public:
    HX711_ADC *device;
    gpio_num_t doutPin;
    gpio_num_t sckPin;
    int mdmStrainThreshold = MDM_STRAIN_DEFAULT_THRESHOLD;
    int mdmStrainThresLow = MDM_STRAIN_DEFAULT_THRES_LOW;
    uint8_t negativeTorqueMethod = NEGATIVE_TORQUE_METHOD;
    bool autoTare = AUTO_TARE;
    ulong autoTareDelayMs = AUTO_TARE_DELAY_MS;
    uint16_t autoTareRangeG = AUTO_TARE_RANGE_G;
    uint16_t autoTareSamples = AUTO_TARE_DELAY_MS / 1000 * STRAIN_TASK_FREQ;

    void
    setup(const gpio_num_t doutPin,
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
    void printSettings();
    void loadSettings();
    void saveSettings();
    void tare();

   private:
    CircularBuffer<float, STRAIN_RINGBUF_SIZE> _measurementBuf;
    bool _halfRevolution = false;
    ulong _lastAutoTare = 0;
};

#endif