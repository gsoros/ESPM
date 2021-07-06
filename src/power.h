#ifndef POWER_H
#define POWER_H

#include <Arduino.h>
//#include <driver/rtc_io.h>

#include "haspreferences.h"
#include "task.h"
#include "CircularBuffer.h"

#ifndef POWER_RINGBUF_SIZE
#define POWER_RINGBUF_SIZE 96  // circular buffer size
#endif

class Power : public Task, public HasPreferences {
   public:
    float crankLength;
    bool reverseMPU;
    bool reverseStrain;

    void setup(Preferences *p);
    void loop(const ulong t);
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