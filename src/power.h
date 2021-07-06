#ifndef POWER_H
#define POWER_H

#include <Arduino.h>
//#include <driver/rtc_io.h>

#include "haspreferences.h"
#include "mpu.h"
#include "strain.h"
#include "task.h"
#include "CircularBuffer.h"

#ifndef POWER_RINGBUF_SIZE
#define POWER_RINGBUF_SIZE 96  // circular buffer size
#endif

class Power : public Task, public HasPreferences {
   public:
    MPU *mpu;
    Strain *strain;
    float crankLength;
    bool reverseMPU;
    bool reverseStrain;

    void setup(MPU *m,
               Strain *s,
               Preferences *p,
               const char *preferencesNS = "POWER") {
        mpu = m;
        strain = s;
        preferencesSetup(p, preferencesNS);
        loadSettings();
    }

    void loop(const ulong t) {
        static ulong previousT = t;
        if (t == previousT) return;
        if (!strain->dataReady()) {
            //log_e("strain not ready, skipping loop at %d, SPS=%f", t, strain->device->getSPS());
            return;
        }

        double deltaT = (t - previousT) / 1000.0;                                          // t(s)
        float rpm = filterNegative(mpu->rpm(), reverseMPU);                                //
        float force = filterNegative(strain->measurement(true), reverseStrain) * 9.80665;  // F(N)   = m(Kg) * G(m/s/s)
        float diameter = 2.0 * crankLength * PI / 1000.0;                                  // d(m)   = 2 * r(mm) * π / 1000
        double distance = diameter * rpm / 60.0 * deltaT;                                  // s(m)   = d(m) * rev/s * t(s)
        double velocity = distance / deltaT;                                               // v(m/s) = s(m) / t(s)
        double work = force * velocity;                                                    // W(J)   = F(N) * v(m)
        double power = work / deltaT;                                                      // P(W)   = W(J) / t(s)
                                                                                           // P      = F d RPM / 60 / t

        /*
        _power = filterNegative(strain->measurement(true), reverseStrain) * 9.80665 *
                 2.0 * crankLength * PI / 1000.0 *
                 filterNegative(mpu->rpm(), reverseMPU) / 60.0 /
                 (t - previousT) / 1000.0;
        */

        /*
        _power = filterNegative(strain->measurement(true), reverseStrain) *
                 filterNegative(mpu->rpm(), reverseMPU) *
                 crankLength / 
                 (t - previousT) *
                 0.000001026949987;
        */

        //if (_powerBuf.isFull()) log_e("power buffer is full");
        _powerBuf.push((float)power);
        previousT = t;
    }

    // Returns the average power since the previous call, emptying the buffer by default.
    // Measurements are added to the buffer at the speed of taskFreq.
    // To avoid data loss, POWER_RINGBUF_SIZE should be large enough to hold the
    // measurements between calls
    float power(bool clearBuffer = true) {
        float power = 0;
        if (_powerBuf.isEmpty()) return power;
        for (decltype(_powerBuf)::index_t i = 0; i < _powerBuf.size(); i++) {
            power += _powerBuf[i] / _powerBuf.size();
        }
        if (clearBuffer) _powerBuf.clear();
        return power;
    }

    // Put ESP into deep sleep // TODO move this
    void deepSleep() {
        Serial.println("Preparing for deep sleep");
        /*
        rtc_gpio_init(MPU_WOM_INT_PIN);
        rtc_gpio_set_direction(MPU_WOM_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_hold_dis(MPU_WOM_INT_PIN);
        rtc_gpio_set_level(MPU_WOM_INT_PIN, LOW);
        rtc_gpio_pulldown_en(MPU_WOM_INT_PIN);
        rtc_gpio_hold_en(MPU_WOM_INT_PIN);
        */
        strain->sleep();
        mpu->enableWomSleep();
        //pinMode(MPU_WOM_INT_PIN, INPUT_PULLDOWN);
        Serial.println("Entering deep sleep");
        Serial.flush();
        delay(1000);
        esp_sleep_enable_ext0_wakeup(MPU_WOM_INT_PIN, HIGH);
        esp_deep_sleep_start();
    }

    void loadSettings() {
        if (!preferencesStartLoad()) return;
        crankLength = preferences->getFloat("crankLength", 172.5);
        reverseMPU = preferences->getBool("reverseMPU", false);
        reverseStrain = preferences->getBool("reverseStrain", false);
        preferencesEnd();
    }

    void saveSettings() {
        if (!preferencesStartSave()) return;
        preferences->putFloat("crankLength", crankLength);
        preferences->putBool("reverseMPU", reverseMPU);
        preferences->putBool("reverseStrain", reverseStrain);
        preferencesEnd();
    }

    void printSettings() {
        Serial.printf("Crank length: %.2fmm\nStrain is %sreversed\nMPU is %sreversed\n",
                      crankLength, reverseStrain ? "" : "not ", reverseMPU ? "" : "not ");
    }

   private:
    CircularBuffer<float, POWER_RINGBUF_SIZE> _powerBuf;

    float filterNegative(float value, bool reverse = false) {
        if (reverse) value *= -1;
        if (value < 0.0)
            return 0.0;
        else
            return value;
    }
};

#endif