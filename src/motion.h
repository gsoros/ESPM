#ifndef MOTION_H
#define MOTION_H

#include <Arduino.h>
#include <CircularBuffer.h>
#ifdef FEATURE_MPU
#include <MPU9250.h>
#include <Wire.h>
#ifndef MPU_RINGBUF_SIZE
#define MPU_RINGBUF_SIZE 16  // circular buffer size
#endif
#endif
#include <Preferences.h>

#include "atoll_preferences.h"
#include "atoll_task.h"

class Motion : public Atoll::Task, public Atoll::Preferences {
   public:
    const char *taskName() { return "Motion"; }
#ifdef FEATURE_MPU
    MPU9250 *mpu;
    ulong mpuLogMs = 0;
    bool mpuAccelGyroNeedsCalibration = false;
    bool mpuMagNeedsCalibration = false;
    void setup(const uint8_t sdaPin,
               const uint8_t sclPin,
               ::Preferences *p);
    void setup(const uint8_t sdaPin,
               const uint8_t sclPin,
               ::Preferences *p,
               const char *preferencesNS,
               uint8_t mpuAddress);
    void mpuEnableWomSleep();
    void mpuCalibrateAccelGyro();
    void mpuCalibrateMag();
    void mpuCalibrate();
    void printMpuAccelGyroCalibration();
    void printMpuMagCalibration();

#ifdef FEATURE_MPU_TEMPERATURE
    float getMpuTemperature();
#endif  // FEATURE_MPU_TEMPERATURE

#else
    void setup(::Preferences *p, const char *preferencesNS);

#endif  // FEATURE_MPU

    bool updateEnabled = false;
    ulong lastMovement = 0;
    uint16_t revolutions = 0;
    ulong lastCrankEventTime = 0;
    int lastHallValue = 0;
    int hallOffset = HALL_DEFAULT_OFFSET;
    int hallThreshold = HALL_DEFAULT_THRESHOLD;
    int hallThresLow = HALL_DEFAULT_THRES_LOW;

    void loop();

    int hall();

    void setHallOffset(int offset);
    void setHallThreshold(int threshold);
    void setHallThresLow(int threshold);
    void printSettings();

    void printMDCalibration();
    void loadSettings();
    void saveSettings();

   private:
    ulong _previousTime = 0;
#ifdef FEATURE_MPU
    float _previousAngle = 0.0;
    ulong _mpuLastLogMs = 0;
#endif
    bool _halfRevolution = false;

    float _prefGetValidFloat(const char *key, const float_t defaultValue);
    size_t _prefPutValidFloat(const char *key, const float_t value);
};

#endif