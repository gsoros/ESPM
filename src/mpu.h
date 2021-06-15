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
        //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_92HZ;
        //s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
        mpu.setup(0x68, s);
        //mpu.selectFilter(QuatFilterSel::MAHONY);
        //mpu.calibrateAccelGyro();
        //mpu.setMagneticDeclination(5+19/60); // 5° 19'
        //mpu.setMagBias(129.550766, -762.064697, 213.780151);
        //mpu.setMagScale(1.044610, 0.996454, 0.962329);
    }

    void loop() {
        if (mpu.update()) {
            yaw = mpu.getYaw();
            //yaw = mpu.getEulerZ();
            //if (180 < yaw) yaw = 180;
            //if (-180 > yaw) yaw = -180;
            lastMeasurementTime = millis();
            resetIdleCycles();
        } else {
            increaseIdleCycles();
        }
    }
};

#endif