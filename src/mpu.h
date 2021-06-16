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
    const char *preferencesNS;
    unsigned long lastMeasurementTime = 0;
    float yaw;

    void setup(Preferences *p, const char *preferencesNS = "MPU")
    {
        preferences = p;
        this->preferencesNS = preferencesNS;
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
#endif
            if (device->update()) {
                yaw = device->getYaw();
                //yaw = device->getAccY();
                //yaw = device->getEulerZ();
                //if (180 < yaw) yaw = 180;
                //if (-180 > yaw) yaw = -180;
                lastMeasurementTime = millis();
                resetIdleCycles();
#ifdef MPU_USE_INTERRUPT
                dataReady = false;
#endif
                return;
            }
#ifdef MPU_USE_INTERRUPT
        }
#endif
        increaseIdleCycles();
    }

    void calibrate()
    {
        Serial.println("Accel and Gyro calibration will start in 2 seconds...");
        Serial.println("Please leave the device still.");
        delay(2000);
        device->calibrateAccelGyro();
        Serial.println("Mag calibration will start in 2 seconds...");
        Serial.println("Please wave device in a figure eight for 15 seconds.");
        delay(2000);
        device->calibrateMag();
        printCalibration();
        saveCalibration();
    }

    void printCalibration()
    {
        Serial.printf("%16s ---------X--------------Y--------------Z------\n", preferencesNS);
        Serial.printf("Accel bias [g]:    %14f %14f %14f\n",
                      device->getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
                      device->getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
                      device->getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
        Serial.printf("Gyro bias [deg/s]: %14f %14f %14f\n",
                      device->getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
                      device->getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
                      device->getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
        Serial.printf("Mag bias [mG]:     %14f %14f %14f\n",
                      device->getMagBiasX(),
                      device->getMagBiasY(),
                      device->getMagBiasZ());
        Serial.printf("Mag scale:         %14f %14f %14f\n",
                      device->getMagScaleX(),
                      device->getMagScaleY(),
                      device->getMagScaleZ());
        Serial.printf("---------------------------------------------------------------\n");
    }

    void loadCalibration()
    {
        if (!preferences->begin(preferencesNS, true)) {      // try ro mode
            if (!preferences->begin(preferencesNS, false)) { // open in rw mode to create ns
                log_e("Preferences begin failed for '%s'.", preferencesNS);
                while (1)
                    ;
            }
        }
        if (!preferences->getBool("calibrated", false)) {
            Serial.println("MPU has not yet been calibrated, press any key to begin calibration.");
            while (!Serial.available()) {
                delay(10);
            }
            preferences->end();
            calibrate();
            return;
        }
        device->setAccBias(
            preferences->getFloat("abX", 0),
            preferences->getFloat("abY", 0),
            preferences->getFloat("abZ", 0));
        device->setGyroBias(
            preferences->getFloat("gbX", 0),
            preferences->getFloat("gbY", 0),
            preferences->getFloat("gbZ", 0));
        device->setMagBias(
            preferences->getFloat("mbX", 0),
            preferences->getFloat("mbY", 0),
            preferences->getFloat("mbZ", 0));
        device->setMagScale(
            preferences->getFloat("msX", 1),
            preferences->getFloat("msY", 1),
            preferences->getFloat("msZ", 1));
        preferences->end();
        printCalibration();
    }

    void saveCalibration()
    {
        if (!preferences->begin(preferencesNS, false)) {
            log_e("Preferences begin failed for '%s'.", preferencesNS);
        }
        preferences->putFloat("abX", device->getAccBiasX());
        preferences->putFloat("abY", device->getAccBiasY());
        preferences->putFloat("abZ", device->getAccBiasZ());
        preferences->putFloat("gbX", device->getGyroBiasX());
        preferences->putFloat("gbY", device->getGyroBiasY());
        preferences->putFloat("gbZ", device->getGyroBiasZ());
        preferences->putFloat("mbX", device->getMagBiasX());
        preferences->putFloat("mbY", device->getMagBiasY());
        preferences->putFloat("mbZ", device->getMagBiasZ());
        preferences->putFloat("msX", device->getMagScaleX());
        preferences->putFloat("msY", device->getMagScaleY());
        preferences->putFloat("msZ", device->getMagScaleZ());
        preferences->putBool("calibrated", true);
    }
};

#endif