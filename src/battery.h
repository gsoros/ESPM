#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include <Preferences.h>
#include <CircularBuffer.h>

#include "definitions.h"
#include "atoll_preferences.h"
#include "atoll_task.h"

#ifndef BATTERY_PIN
#define BATTERY_PIN GPIO_NUM_35
#endif

#ifndef BATTERY_RINGBUF_SIZE
#define BATTERY_RINGBUF_SIZE 10  // circular buffer size
#endif

#ifndef BATTERY_EMPTY
#define BATTERY_EMPTY 3.2
#endif

#ifndef BATTERY_FULL
#define BATTERY_FULL 4.1
#endif

class Battery : public Atoll::Task, public Atoll::Preferences {
   public:
    const char *taskName() { return "Battery"; }
    float corrF = 1.0;
    float voltage = 0.0;
    float pinVoltage = 0.0;
    uint8_t level = 0;

    void setup(::Preferences *p);
    void loop();
    int calculateLevel();
    float measureVoltage();
    float measureVoltage(bool useCorrection);
    void loadSettings();
    void calibrateTo(float realVoltage);
    void saveSettings();
    void printSettings();

   private:
    CircularBuffer<float, BATTERY_RINGBUF_SIZE> _voltageBuf;
};

#endif