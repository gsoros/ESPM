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
    uint16_t revolutions = 0;
    ulong lastCrankEventTime = 0;
    int lastHallValue = 0;

    void setup(const uint8_t sdaPin,
               const uint8_t sclPin,
               Preferences *p);
    void setup(const uint8_t sdaPin,
               const uint8_t sclPin,
               Preferences *p,
               const char *preferencesNS,
               uint8_t mpuAddress);

    void loop();

    int hall();

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
    ulong _previousTime = 0;
    float _previousAngle = 0.0;
    bool _halfRevolution = false;

    float _prefGetValidFloat(const char *key, const float_t defaultValue);
    size_t _prefPutValidFloat(const char *key, const float_t value);
};

#endif