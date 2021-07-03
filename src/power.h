#ifndef POWER_H
#define POWER_H

#include <Arduino.h>

#include "haspreferences.h"
#include "mpu.h"
#include "strain.h"
#include "task.h"

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
        /*
        double deltaT = (t - previousT) / 1000.0;  // t(s)
        float rpm = filterNegative(mpu->rpm(), reverseMPU);
        float force = filterNegative(strain->measurement(true), reverseStrain) * 9.80665;  // F(N)   = m(Kg) * G(m/s/s)
        float diameter = 2.0 * crankLength * PI / 1000.0;                                  // d(m)   = 2 * r(mm) * π / 1000
        double distance = diameter * rpm / 60.0 * deltaT;                                  // s(m)   = d(m) * rev/s * t(s)
        double velocity = distance / deltaT;                                               // v(m/s) = s(m) / t(s)
        double work = force * velocity;                                                    // W(J)   = F(N) * v(m)
        power = work / deltaT;                                                             // P(W)   = W(J) / t(s)
                                                                                           // P = F d RPM / 60 / t
        */
        _power = filterNegative(strain->measurement(true), reverseStrain) * 9.80665 *
                 2.0 * crankLength * PI / 1000.0 *
                 filterNegative(mpu->rpm(), reverseMPU) / 60.0 /
                 (t - previousT) / 1000.0;
    }

    double power() {
        return _power;
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
    double _power = 0.0;

    float filterNegative(float value, bool reverse = false) {
        if (reverse)
            return 0.0 < value ? 0.0 : value;
        return 0.0 > value ? 0.0 : value;
    }
};

#endif