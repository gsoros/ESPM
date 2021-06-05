#ifndef MPU_H
#define MPU_H

#include <Wire.h>
#include <MPU9250.h>

#define SDA_PIN 23
#define SCL_PIN 19

class MPU {
    public:
    MPU9250 mpu;
    uint32_t crankPosition = 0;
    uint32_t power = 0;

    void setup() {
        Wire.begin(SDA_PIN, SCL_PIN);
        delay(100);
        this->mpu.verbose(true);
        this->mpu.setup(0x68);
        this->mpu.selectFilter(QuatFilterSel::MAHONY);
        this->mpu.calibrateAccelGyro();
        this->mpu.setMagneticDeclination(5.19);
        this->mpu.setMagBias(129.550766, -762.064697, 213.780151);
        this->mpu.setMagScale(1.044610, 0.996454, 0.962329);
    }

    void loop() {
        if (this->mpu.update()) {
            //Serial.printf("%f %f %f\n", 
            //    this->mpu.getPitch(), this->mpu.getRoll(), this->mpu.getYaw());
            this->crankPosition = 180 + (int)this->mpu.getYaw();
            this->power = this->crankPosition; // TODO
        }
    }
};

#endif