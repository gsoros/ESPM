#ifndef POWER_H
#define POWER_H

#include <Arduino.h>

#include "mpu.h"
#include "strain.h"
#include "task.h"

class Power : public Task {
   public:
    MPU *mpu;
    Strain *strain;
    Preferences *preferences;
    const char *preferencesNS;
    float crankLength = 172.5;  // mm
    double power;

    void setup(MPU *m,
               Strain *s,
               Preferences *p,
               const char *preferencesNS = "POWER") {
        mpu = m;
        strain = s;
        preferences = p;
        this->preferencesNS = preferencesNS;
    }

    void loop(const ulong t) {
        static ulong previousT = t;
        if (t == previousT) return;
        if (!strain->dataReady()) {
            log_e("strain not ready, skipping loop at %d", t);
            return;
        }
        float rpm = mpu->rpm();
        if (rpm < 0) rpm = 0;
        float force = strain->measurement(true) * 9.80665;            // N
        float diameter = 2 * crankLength * PI / 1000.0;               // m
        double distance = diameter * rpm / 6000.0 * (t - previousT);  // m
        power = force * distance;
    }

   private:
};

#endif