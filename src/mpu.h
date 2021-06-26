#ifndef MPU_H
#define MPU_H

#include "idle.h"
#include <Arduino.h>
#include <MPU9250.h>
#include <Preferences.h>
#include <Wire.h>

#define MPU_ADDR 0x68
// #define MPU_USE_INTERRUPT
// #define MPU_INT_PIN 15

#ifdef MPU_USE_INTERRUPT
volatile bool mpuDataReady;
void IRAM_ATTR mpuDataReadyISR()
{
    mpuDataReady = true;
}
#endif

class MPU : public Idle
{
public:
    MPU9250 *device;
    bool updateEnabled = false;
    bool accelGyroNeedsCalibration = false;
    bool magNeedsCalibration = false;
    Preferences *preferences;
    const char *preferencesNS;
    ulong lastMeasurementTime = 0;
    float measurement = 0.0;
    float qX = 0.0;
    float qY = 0.0;
    float qZ = 0.0;
    float qW = 0.0;

    void setup(uint8_t sdaPin,
               uint8_t sclPin,
               Preferences *p,
               const char *preferencesNS = "MPU")
    {
        preferences = p;
        this->preferencesNS = preferencesNS;
        Wire.begin(sdaPin, sclPin);
        delay(100);
        device = new MPU9250();
        //device->verbose(true);
        MPU9250Setting s;
        s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
        //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_10HZ;
        //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_3600HZ;
        //s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_10HZ;
        //s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_420HZ;
        //s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
        device->setup(MPU_ADDR, s);
        //device->selectFilter(QuatFilterSel::MAHONY);
        device->selectFilter(QuatFilterSel::NONE);
        //device->selectFilter(QuatFilterSel::MADGWICK);
        //device->calibrateAccelGyro();
        device->setMagneticDeclination(5 + 19 / 60); // 5° 19'
        //device->setMagBias(129.550766, -762.064697, 213.780151);
        //device->setMagScale(1.044610, 0.996454, 0.962329);
        loadCalibration();
#ifdef MPU_USE_INTERRUPT
        attachInterrupt(digitalPinToInterrupt(MPU_INT_PIN), mpuDataReadyISR, FALLING);
#endif
        updateEnabled = true;
    }

    void loop(const ulong t)
    {
        if (accelGyroNeedsCalibration)
        {
            calibrateAccelGyro();
            accelGyroNeedsCalibration = false;
        }
        if (magNeedsCalibration)
        {
            calibrateMag();
            magNeedsCalibration = false;
        }
        if (!updateEnabled)
            return;
#ifdef MPU_USE_INTERRUPT
        if (mpuDataReady)
        {
#endif
            if (device->update())
            {
                measurement = device->getYaw();
                //measurement = device->getAccY();
                //measurement = device->getEulerZ();
                qX = device->getQuaternionX();
                qY = device->getQuaternionY();
                qZ = device->getQuaternionZ();
                qW = device->getQuaternionW();
                //Serial.printf("%f %f %f %f\n", qX, qY, qZ, qW);
                lastMeasurementTime = millis();
                resetIdleCycles();
#ifdef MPU_USE_INTERRUPT
                mpuDataReady = false;
#endif
                return;
            }
#ifdef MPU_USE_INTERRUPT
        }
#endif
        increaseIdleCycles();
    }

    void calibrateAccelGyro()
    {
        log_i("Accel and Gyro calibration, please leave the device still.");
        //delay(2000);
        updateEnabled = false;
        device->calibrateAccelGyro();
        updateEnabled = true;
    }

    void calibrateMag()
    {
        log_i("Mag calibration, please wave device in a figure eight for 15 seconds.");
        //delay(2000);
        updateEnabled = false;
        device->calibrateMag();
        updateEnabled = true;
    }

    void calibrate()
    {
        device->calibrateAccelGyro();
        device->calibrateMag();
        printCalibration();
        saveCalibration();
    }

    void printCalibration()
    {
        printAccelGyroCalibration();
        printMagCalibration();
    }

    void printAccelGyroCalibration()
    {
#ifdef FEATURE_SERIALIO

        Serial.printf("%16s ---------X--------------Y--------------Z------\n", preferencesNS);
        Serial.printf("Accel bias [g]:    %14f %14f %14f\n",
                      device->getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
                      device->getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
                      device->getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
        Serial.printf("Gyro bias [deg/s]: %14f %14f %14f\n",
                      device->getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
                      device->getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
                      device->getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
        Serial.printf("---------------------------------------------------------------\n");
#endif
    }

    void printMagCalibration()
    {
#ifdef FEATURE_SERIALIO

        Serial.printf("%16s ---------X--------------Y--------------Z------\n", preferencesNS);
        Serial.printf("Mag bias [mG]:     %14f %14f %14f\n",
                      device->getMagBiasX(),
                      device->getMagBiasY(),
                      device->getMagBiasZ());
        Serial.printf("Mag scale:         %14f %14f %14f\n",
                      device->getMagScaleX(),
                      device->getMagScaleY(),
                      device->getMagScaleZ());
        Serial.printf("---------------------------------------------------------------\n");
#endif
    }

    void loadCalibration()
    {
        if (!preferences->begin(preferencesNS, true)) // try ro mode
        {
            if (!preferences->begin(preferencesNS, false)) // open in rw mode to create ns
            {
                log_e("Preferences begin failed for '%s'\n", preferencesNS);
                return;
            }
        }
        if (!preferences->getBool("calibrated", false))
        {
            preferences->end();
            log_e("MPU has not yet been calibrated");
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
        if (!preferences->begin(preferencesNS, false))
        {
            log_e("Preferences begin failed for '%s'.", preferencesNS);
            return;
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
        preferences->end();
    }
};

#endif