#ifndef MPU_H
#define MPU_H

#include <Arduino.h>
#include <CircularBuffer.h>
#include <MPU9250.h>
#include <Preferences.h>
#include <Wire.h>

#include "haspreferences.h"
#include "task.h"

#ifndef MPU_RINGBUF_SIZE
#define MPU_RINGBUF_SIZE 16  // circular buffer size
#endif

class MPU : public Task, public HasPreferences {
   public:
    MPU9250 *device;
    bool updateEnabled = false;
    bool accelGyroNeedsCalibration = false;
    bool magNeedsCalibration = false;
    ulong lastMovement = 0;

    struct Quaternion {
        float x, y, z, w;
    };

    void setup(const uint8_t sdaPin,
               const uint8_t sclPin,
               Preferences *p);
    void setup(const uint8_t sdaPin,
               const uint8_t sclPin,
               Preferences *p,
               const char *preferencesNS,
               uint8_t mpuAddress);

    void loop();

    float rpm(bool unsetDataReadyFlag = false);
    bool dataReady();
    Quaternion quaternion();
    void enableWomSleep(void);
    void calibrateAccelGyro();
    void calibrateMag();
    void calibrate();
    void printCalibration();
    void printAccelGyroCalibration();
    void printMagCalibration();
    void loadCalibration();
    void saveCalibration();

   private:
    CircularBuffer<float, MPU_RINGBUF_SIZE> _rpmBuf;
    float _rpm = 0.0;  // average rpm of the ring buffer
    bool _dataReady = false;

    float prefGetValidFloat(const char *key, const float_t defaultValue);
    size_t prefPutValidFloat(const char *key, const float_t value);
};

#endif