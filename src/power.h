#ifndef POWER_H
#define POWER_H

#include <Arduino.h>
//#include <driver/rtc_io.h>

#include "haspreferences.h"
#include "task.h"
#include "CircularBuffer.h"
#include "definitions.h"

#ifndef POWER_RINGBUF_SIZE
#define POWER_RINGBUF_SIZE 96  // circular buffer size
#endif

class Power : public Task, public HasPreferences {
   public:
    float crankLength;  // crank length in mm
    bool reverseMPU;
    bool reverseStrain;
    bool reportDouble;

    void setup(Preferences *p);
    void loop();
    float power();
    float power(bool clearBuffer);
    void loadSettings();
    void saveSettings();
    void printSettings();

   private:
    CircularBuffer<float, POWER_RINGBUF_SIZE> _powerBuf;

    float filterNegative(float value);
    float filterNegative(float value, bool reverse);
};

#endif