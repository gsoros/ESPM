#ifndef LED_H
#define LED_H

#ifndef LED_PIN
#define LED_PIN GPIO_NUM_22  // LED_BUILTIN
#endif

#include <Arduino.h>

#include "task.h"

class Led : public Task {
   public:
    void setup() {
        pinMode(LED_PIN, OUTPUT);
        set();
        defaultMode();
    }

    void loop() {
        const ulong t = millis();
        if (!state) {                      // led is off
            if (lastSet <= t - offTime) {  //
                set(true, t);              // turn on
                if (0 < count)             // in a blink sequence
                    count--;               // decrease remaining blinks counter
            }
            return;
        }
        if (lastSet <= t - onTime) {  // led is on
            set(false, t);            // turn off
            if (0 == count)           // blink sequence done
                defaultMode();        // return to default mode
        }
    }

    void blink() {
        blink(1);
    }
    // call with a negative count parameter to blink indefinitely
    void blink(int8_t count) {
        blink(count, 100, 300);
    }
    // blink count times, onTime and offTime minimums are Led::taskFreq
    void blink(int8_t count, uint16_t onTime, uint16_t offTime) {
        if (0 == count) return;
        if (onTime < 10 || 10000 < onTime) return;
        if (offTime < 10 || 10000 < offTime) return;
        this->count = count;
        this->onTime = onTime;
        this->offTime = offTime;
        set(true);
    }

    void defaultMode() {
        blink(-1, 100, 1900);
    }

   private:
    bool state = true;  // on at boot
    int8_t count;
    uint16_t onTime;
    uint16_t offTime;
    ulong lastSet = 0;

    void set() {
        set(state);
    }
    void set(bool state) {
        set(state, millis());
    }
    void set(bool state, ulong t) {
        this->state = state;
        digitalWrite(LED_PIN, state ? LOW : HIGH);
        lastSet = t;
    }
};

#endif