#ifndef MOTION_H
#define MOTION_H

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

class Motion : public Task, public HasPreferences {
   public:
    MPU9250 *device;
    bool updateEnabled = false;
    bool accelGyroNeedsCalibration = false;
    bool magNeedsCalibration = false;
    ulong lastMovement = 0;
    uint16_t revolutions = 0;
    ulong lastCrankEventTime = 0;
    int lastHallValue = 0;
    int hallOffset = HALL_DEFAULT_OFFSET;
    int hallThreshold = HALL_DEFAULT_THRESHOLD;
    int hallThresLow = HALL_DEFAULT_THRES_LOW;

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

    void enableWomSleep();
    void calibrateAccelGyro();
    void calibrateMag();
    void calibrate();
    void setHallOffset(int offset);
    void setHallThreshold(int threshold);
    void setHallThresLow(int threshold);
    void printCalibration();
    void printAccelGyroCalibration();
    void printMagCalibration();
    void printMDCalibration();
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