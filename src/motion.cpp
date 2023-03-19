#include "board.h"
#include "motion.h"

#include "driver/adc.h"

#ifdef FEATURE_MPU
void Motion::setup(const uint8_t sdaPin,
                   const uint8_t sclPin,
                   ::Preferences *p) {
    setup(sdaPin, sclPin, p, "MOTION", 0x68);
}
void Motion::setup(const uint8_t sdaPin,
                   const uint8_t sclPin,
                   ::Preferences *p,
                   const char *preferencesNS,
                   uint8_t mpuAddress) {
    preferencesSetup(p, preferencesNS);
    if (board.motionDetectionMethod == MDM_MPU ||
#ifdef FEATURE_MPU_TEMPERATURE
        true
#else
        false
#endif
    ) {
        Wire.begin(sdaPin, sclPin);
        vTaskDelay(100);
        mpu = new MPU9250();
        // device->verbose(true);
        MPU9250Setting s;
        s.skip_mag = true;  // compass not needed
        s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
        // s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_10HZ;
        // s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_3600HZ;
        // s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_10HZ;
        // s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_420HZ;
        // s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
        if (!mpu->setup(mpuAddress, s))
            log_e("setup error");
        // device->selectFilter(QuatFilterSel::MAHONY);
        mpu->selectFilter(QuatFilterSel::NONE);
        // device->selectFilter(QuatFilterSel::MADGWICK);
        // device->setMagneticDeclination(5 + 19 / 60);  // 5° 19'
    }
#else   // !FEATURE_MPU
void Motion::setup(::Preferences *pp, const char *preferencesNS) {
    preferencesSetup(p, preferencesNS);
    if (board.motionDetectionMethod == MDM_MPU) log_e("MDM is MPU but FEATURE_MPU is missing");
#endif  // FEATURE_MPU

    if (board.motionDetectionMethod == MDM_HALL) {
        adc1_config_width(ADC_WIDTH_BIT_12);
    }
    loadSettings();
    // printSettings();
    updateEnabled = true;
    lastMovement = millis();
}

void Motion::loop() {
    const ulong t = millis();

#ifdef FEATURE_MPU
    if (board.motionDetectionMethod == MDM_MPU) {
        if (mpuAccelGyroNeedsCalibration) {
            mpuCalibrateAccelGyro();
            mpuAccelGyroNeedsCalibration = false;
        }
        if (mpuMagNeedsCalibration) {
            mpuCalibrateMag();
            mpuMagNeedsCalibration = false;
        }
        if (!updateEnabled)
            return;
        if (!mpu->update())
            return;

#ifdef FEATURE_SERIAL
        if (0 < mpuLogMs && _mpuLastLogMs + mpuLogMs <= t) {
            Serial.printf("[MPU] %.2f %.2f %.2f %.2f\n",
                          mpu->getPitch(),
                          mpu->getRoll(),
                          mpu->getYaw(),
                          mpu->getTemperature());
            _mpuLastLogMs = t;
        }
#endif  // FEATURE_SERIAL

        float angle = mpu->getYaw() + 180.0;  // -180...180 -> 0...360

        if ((_previousAngle < 180.0 && 180.0 <= angle) || (angle < 180.0 && 180.0 <= _previousAngle)) {
            lastMovement = t;
            if (!_halfRevolution) {
                if (0 < lastCrankEventTime) {
                    ulong dt = t - lastCrankEventTime;
                    if (CRANK_EVENT_MIN_MS < dt) {
                        revolutions++;
                        log_i("crank event #%d dt: %ldms", revolutions, dt);
                        board.power.onCrankEvent(dt);
                        board.bleServer.onCrankEvent(t, revolutions);
                    } else {
                        // Serial.printf("Crank event skip, dt too small: %ldms\n", dt);
                    }
                }
                lastCrankEventTime = t;
            }
            _halfRevolution = !_halfRevolution;
        }
        _previousTime = t;
        _previousAngle = angle;
    } else

#ifdef FEATURE_MPU_TEMPERATURE
        mpu->update();
#endif  // FEATURE_MPU_TEMPERATURE

#endif  // FEATURE_MPU

    if (board.motionDetectionMethod == MDM_HALL) {
        if (!_halfRevolution) {
            if (abs(hall()) < hallThresLow) {
                _halfRevolution = true;
            }
        } else if (hallThreshold < abs(hall())) {
            _halfRevolution = false;
            lastMovement = t;
            if (0 < lastCrankEventTime) {
                ulong dt = t - lastCrankEventTime;
                if (CRANK_EVENT_MIN_MS < dt) {
                    revolutions++;
                    log_i("crank event #%d dt: %ldms", revolutions, dt);
                    board.power.onCrankEvent(dt);
                    board.bleServer.onCrankEvent(t, revolutions);
                    lastCrankEventTime = t;
                } else {
                    // Serial.printf("Crank event skip, dt too small: %ldms\n", dt);
                }
            } else {
                lastCrankEventTime = t;
            }
        }
    }
}

int Motion::hall() {
    float avg = 0.0;
    for (int i = 0; i < HALL_DEFAULT_SAMPLES; i++) {
        avg += hall_sensor_read() / HALL_DEFAULT_SAMPLES;
        delayMicroseconds(3);
    }
    /*
    int value;
    Serial.print("hall() ");
    for (int i = 0; i < HALL_DEFAULT_SAMPLES; i++) {
        value = hall_sensor_read();
        avg += value / HALL_DEFAULT_SAMPLES;
        Serial.printf("%d ", value + hallOffset);
    }
    Serial.println();
    */
    lastHallValue = (int)avg + hallOffset;
    return lastHallValue;
}

#ifdef FEATURE_MPU
// Enable wake-on-motion and go to sleep
void Motion::mpuEnableWomSleep(void) {
    // Todo enable waking on hall sensor (https://esp32.com/viewtopic.php?t=4608)
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("Enabling W-O-M sleep");
    updateEnabled = false;
    delay(20);
    mpu->enableWomSleep();
}

void Motion::mpuCalibrateAccelGyro() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("Accel and Gyro calibration, please leave the device still.");
    updateEnabled = false;
    mpu->calibrateAccelGyro();
    updateEnabled = true;
}

void Motion::mpuCalibrateMag() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("Mag calibration, please wave device in a figure eight for 15 seconds.");
    updateEnabled = false;
    mpu->calibrateMag();
    updateEnabled = true;
}

void Motion::mpuCalibrate() {
    mpu->calibrateAccelGyro();
    mpu->calibrateMag();
    printSettings();
    saveSettings();
}

void Motion::printMpuAccelGyroCalibration() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("%16s ---------X--------------Y--------------Z------\n", preferencesNS);
    log_i("Accel bias [g]:    %14f %14f %14f",
          mpu->getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
          mpu->getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
          mpu->getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
    log_i("Gyro bias [deg/s]: %14f %14f %14f",
          mpu->getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
          mpu->getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
          mpu->getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
    log_i("---------------------------------------------------------------");
}

void Motion::printMpuMagCalibration() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("%16s ---------X--------------Y--------------Z------", preferencesNS);
    log_i("Mag bias [mG]:     %14f %14f %14f",
          mpu->getMagBiasX(),
          mpu->getMagBiasY(),
          mpu->getMagBiasZ());
    log_i("Mag scale:         %14f %14f %14f",
          mpu->getMagScaleX(),
          mpu->getMagScaleY(),
          mpu->getMagScaleZ());
    log_i("---------------------------------------------------------------");
}
#endif

void Motion::setHallOffset(int offset) {
    hallOffset = offset;
}

void Motion::setHallThreshold(int threshold) {
    hallThreshold = threshold;
}

void Motion::setHallThresLow(int threshold) {
    hallThresLow = threshold;
}

void Motion::printSettings() {
#ifdef FEATURE_MPU
    printMpuAccelGyroCalibration();
    printMpuMagCalibration();
#endif
    printMDCalibration();
    log_i("Movement detection method:");
    if (board.motionDetectionMethod == MDM_STRAIN)
        log_i("Strain");
    else if (board.motionDetectionMethod == MDM_MPU)
        log_i("MPU");
    else if (board.motionDetectionMethod == MDM_HALL)
        log_i("Hall sensor");
    else
        log_i("invalid");
}

void Motion::printMDCalibration() {
    log_i("Hall offset: %d\nHall low threshold: %d\nHall high threshold: %d",
          hallOffset,
          hallThresLow,
          hallThreshold);
    log_i("Strain MD low threshold: %d\nStrain MD high threshold: %d",
          board.strain.mdmStrainThresLow,
          board.strain.mdmStrainThreshold);
}

void Motion::loadSettings() {
    if (!preferencesStartLoad()) return;

    if (board.motionDetectionMethod == MDM_MPU) {
#ifdef FEATURE_MPU
        if (!preferences->getBool("mpuCal", false)) {
            preferencesEnd();
            log_e("mpu has not yet been calibrated");
            return;
        }
        mpu->setAccBias(
            _prefGetValidFloat("mpuabX", 0),
            _prefGetValidFloat("mpuabY", 0),
            _prefGetValidFloat("mpuabZ", 0));
        mpu->setGyroBias(
            _prefGetValidFloat("mpugbX", 0),
            _prefGetValidFloat("mpugbY", 0),
            _prefGetValidFloat("mpugbZ", 0));
        mpu->setMagBias(
            _prefGetValidFloat("mpumbX", 0),
            _prefGetValidFloat("mpumbY", 0),
            _prefGetValidFloat("mpumbZ", 0));
        mpu->setMagScale(
            _prefGetValidFloat("mpumsX", 1),
            _prefGetValidFloat("mpumsY", 1),
            _prefGetValidFloat("mpumsZ", 1));
#else
        log_e("MDM is MPU but FEATURE_MPU is missing");
#endif
    }
    hallOffset = preferences->getInt("hallO", hallOffset);
    hallThreshold = preferences->getInt("hallT", hallThreshold);
    hallThresLow = preferences->getInt("hallTL", hallThresLow);
    preferencesEnd();
    printSettings();
}

void Motion::saveSettings() {
    if (!preferencesStartSave()) return;
#ifdef FEATURE_MPU
    if (board.motionDetectionMethod == MDM_MPU) {
        _prefPutValidFloat("mpuabX", mpu->getAccBiasX());
        _prefPutValidFloat("mpuabY", mpu->getAccBiasY());
        _prefPutValidFloat("mpuabZ", mpu->getAccBiasZ());
        _prefPutValidFloat("mpugbX", mpu->getGyroBiasX());
        _prefPutValidFloat("mpugbY", mpu->getGyroBiasY());
        _prefPutValidFloat("mpugbZ", mpu->getGyroBiasZ());
        _prefPutValidFloat("mpumbX", mpu->getMagBiasX());
        _prefPutValidFloat("mpumbY", mpu->getMagBiasY());
        _prefPutValidFloat("mpumbZ", mpu->getMagBiasZ());
        _prefPutValidFloat("mpumsX", mpu->getMagScaleX());
        _prefPutValidFloat("mpumsY", mpu->getMagScaleY());
        _prefPutValidFloat("mpumsZ", mpu->getMagScaleZ());
        preferences->putBool("mpuCal", true);
    }
#endif
    preferences->putInt("hallO", (int32_t)hallOffset);
    preferences->putInt("hallT", (int32_t)hallThreshold);
    preferences->putInt("hallTL", (int32_t)hallThresLow);
    preferencesEnd();
}

float Motion::_prefGetValidFloat(const char *key, const float_t defaultValue) {
    float f = preferences->getFloat(key, defaultValue);
    log_i("loaded %f for (%s, %f) from %s", f, key, defaultValue, preferencesNS);
    if (isinf(f) || isnan(f)) {
        log_e("invalid, returning default %f for %s", defaultValue, key);
        f = defaultValue;
    }
    return f;
}

size_t Motion::_prefPutValidFloat(const char *key, const float_t value) {
    size_t written = 0;
    if (isinf(value) || isnan(value)) {
        log_e("invalid, not saving %f for %s", value, key);
        return written;
    }
    written = preferences->putFloat(key, value);
    log_i("saved %f for %s in %s", value, key, preferencesNS);
    return written;
}

#if defined(FEATURE_MPU) && defined(FEATURE_MPU_TEMPERATURE)
float Motion::getMpuTemperature() {
    return mpu->getTemperature();
}
#endif