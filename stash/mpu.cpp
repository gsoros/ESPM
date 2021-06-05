#include <Arduino.h>
#include <Wire.h>
#include "MPU9250.h"

#define SDA_PIN 23
#define SCL_PIN 19


MPU9250 mpu;

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(2000);

    mpu.verbose(true);
    mpu.setup(0x68);
    mpu.selectFilter(QuatFilterSel::MAHONY);
    mpu.calibrateAccelGyro();
    mpu.setMagneticDeclination(5.19);

    //mpu.setMagBias(163.269455, -842.188354, 294.161469);
    //mpu.setMagScale(1.003484, 1.058824, 0.944262);

    mpu.setMagBias(129.550766, -762.064697, 213.780151);
    mpu.setMagScale(1.044610, 0.996454, 0.962329);
    
    /*
    mpu.calibrateMag();
    Serial.printf("mpu.setMagBias(%f, %f, %f);\nmpu.setMagScale(%f, %f, %f);\n", 
        mpu.getMagBiasX(), mpu.getMagBiasY(), mpu.getMagBiasZ(), 
        mpu.getMagScaleX(), mpu.getMagScaleY(), mpu.getMagScaleZ());
    while(1);
    */
    
}

void loop() {
    if (mpu.update()) {
        Serial.printf("%f %f %f\n", mpu.getPitch(), mpu.getRoll(), mpu.getYaw());
    }
}