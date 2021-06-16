#ifndef MPU_H
#define MPU_H

#include "idle.h"
#include <Arduino.h>
#include <MPU9250.h>
#include <Preferences.h>
#include <Wire.h>

#define MPU_SDA_PIN 23
#define MPU_SCL_PIN 19
#define MPU_ADDR 0x68
// #define MPU_USE_INTERRUPT
// #define MPU_INT_PIN 15

class MPU : public Idle
{
public:
    MPU9250 *device;
    const uint8_t sdaPin = MPU_SDA_PIN;
    const uint8_t sclPin = MPU_SCL_PIN;
#ifdef MPU_USE_INTERRUPT
    const uint8_t intPin = MPU_INT_PIN;
    volatile bool dataReady;
#endif
    Preferences *preferences;
    unsigned long lastMeasurementTime = 0;
    float yaw;

    void setup(Preferences *p)
    {
        preferences = p;
        Wire.begin(sdaPin, sclPin);
        delay(100);
        device = new MPU9250();
        //device->verbose(true);
        MPU9250Setting s;
        s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
        s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_10HZ;
        //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_3600HZ;
        s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_10HZ;
        //s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_420HZ;
        //s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
        device->setup(MPU_ADDR, s);
        //device->selectFilter(QuatFilterSel::MAHONY);
        device->selectFilter(QuatFilterSel::NONE);
        //device->selectFilter(QuatFilterSel::MADGWICK);
        device->calibrateAccelGyro();
        device->setMagneticDeclination(5 + 19 / 60); // 5° 19'
        device->setMagBias(129.550766, -762.064697, 213.780151);
        device->setMagScale(1.044610, 0.996454, 0.962329);
        loadCalibration();
    }

    void loop()
    {
#ifdef MPU_USE_INTERRUPT
        if (dataReady) {
#else
        if (device->update()) {
#endif
            yaw = device->getYaw();
            //yaw = device->getAccY();
            //yaw = device->getEulerZ();
            //if (180 < yaw) yaw = 180;
            //if (-180 > yaw) yaw = -180;
            lastMeasurementTime = millis();
#ifdef MPU_USE_INTERRUPT
            dataReady = false;
#endif
            resetIdleCycles();
        } else {
            increaseIdleCycles();
        }
    }

    void calibrate()
    {
        Serial.println("Accel and Gyro calibration will start in 5sec.");
        Serial.println("Please leave the device still.");
        device->verbose(true);
        delay(5000);
        device->calibrateAccelGyro();
        Serial.println("Mag calibration will start in 5sec.");
        Serial.println("Please wave device in a figure eight until done.");
        delay(5000);
        device->calibrateMag();
        printCalibration();
        device->verbose(false);
        //saveCalibration();
        //loadCalibration();
    }

    void printCalibration()
    {
        Serial.println("Accel bias [g]: ");
        Serial.print(device->getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
        Serial.print(", ");
        Serial.print(device->getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
        Serial.print(", ");
        Serial.print(device->getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
        Serial.println();
        Serial.println("Gyro bias [deg/s]: ");
        Serial.print(device->getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
        Serial.print(", ");
        Serial.print(device->getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
        Serial.print(", ");
        Serial.print(device->getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
        Serial.println();
        Serial.println("Mag bias [mG]: ");
        Serial.print(device->getMagBiasX());
        Serial.print(", ");
        Serial.print(device->getMagBiasY());
        Serial.print(", ");
        Serial.print(device->getMagBiasZ());
        Serial.println();
        Serial.println("Mag scale []: ");
        Serial.print(device->getMagScaleX());
        Serial.print(", ");
        Serial.print(device->getMagScaleY());
        Serial.print(", ");
        Serial.print(device->getMagScaleZ());
    }

    void loadCalibration()
    {
    }
};

#endif