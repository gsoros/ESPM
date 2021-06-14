#ifndef MPU_H
#define MPU_H

#include <Wire.h>
#include <MPU9250.h>
#include "idle.h"

#define SDA_PIN 23
#define SCL_PIN 19

class MPU : public Idle {
    public:
    MPU9250 mpu;
    unsigned long lastMeasurementTime = 0;
    float yaw;

    void setup() {
        Wire.begin(SDA_PIN, SCL_PIN);
        delay(100);
        mpu.verbose(true);
        MPU9250Setting s;
        s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
        mpu.setup(0x68, s);
        mpu.selectFilter(QuatFilterSel::MAHONY);
        mpu.calibrateAccelGyro();
        mpu.setMagneticDeclination(5.19);
        mpu.setMagBias(129.550766, -762.064697, 213.780151);
        mpu.setMagScale(1.044610, 0.996454, 0.962329);
    }

    void loop() {
        if (mpu.update()) {
            yaw = mpu.getYaw();
            lastMeasurementTime = millis();
            resetIdleCycles();
        } else {
            increaseIdleCycles();
        }
    }
};

#endif