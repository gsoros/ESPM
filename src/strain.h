#ifndef STRAIN_H
#define STRAIN_H

#include <Arduino.h>
#include <HX711_ADC.h>
#include <CircularBuffer.h>
#include <driver/rtc_io.h>

#include "atoll_preferences.h"
#include "atoll_task.h"

#ifndef STRAIN_RINGBUF_SIZE
#define STRAIN_RINGBUF_SIZE 512  // circular buffer size
#endif

class Strain : public Atoll::Task,
               public Atoll::Preferences {
   public:
    const char *taskName() { return "Strain"; }
    HX711_ADC *device;
    gpio_num_t doutPin;
    gpio_num_t sckPin;
    int mdmStrainThreshold = MDM_STRAIN_DEFAULT_THRESHOLD;
    int mdmStrainThresLow = MDM_STRAIN_DEFAULT_THRES_LOW;
    uint8_t negativeTorqueMethod = NEGATIVE_TORQUE_METHOD;

    void setup(const gpio_num_t doutPin,
               const gpio_num_t sckPin,
               ::Preferences *p,
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
    bool getAutoTare();
    void setAutoTare(bool val);
    ulong getAutoTareDelayMs();
    void setAutoTareDelayMs(ulong val);
    uint16_t getAutoTareRangeG();
    void setAutoTareRangeG(uint16_t val);

   private:
    CircularBuffer<float, STRAIN_RINGBUF_SIZE> _measurementBuf;
    bool _halfRevolution = false;
    bool autoTare = AUTO_TARE;
    ulong autoTareDelayMs;
    uint16_t autoTareRangeG = AUTO_TARE_RANGE_G;
    uint16_t autoTareSamples;
    ulong _lastAutoTare = 0;
};

#endif