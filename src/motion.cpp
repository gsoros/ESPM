#include "board.h"
#include "motion.h"

#include "driver/adc.h"

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
    if (board.motionDetectionMethod == MDM_MPU) {
        Wire.begin(sdaPin, sclPin);
        vTaskDelay(100);
        device = new MPU9250();
        // device->verbose(true);
        MPU9250Setting s;
        s.skip_mag = true;  // compass not needed
        s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
        // s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_10HZ;
        // s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_3600HZ;
        // s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_10HZ;
        // s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_420HZ;
        // s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
        if (!device->setup(mpuAddress, s))
            log_e("setup error");
        // device->selectFilter(QuatFilterSel::MAHONY);
        device->selectFilter(QuatFilterSel::NONE);
        // device->selectFilter(QuatFilterSel::MADGWICK);
        // device->setMagneticDeclination(5 + 19 / 60);  // 5° 19'
    } else if (board.motionDetectionMethod == MDM_HALL) {
        adc1_config_width(ADC_WIDTH_BIT_12);
    }
    loadSettings();
    // printSettings();
    updateEnabled = true;
    lastMovement = millis();
}

void Motion::loop() {
    const ulong t = millis();

    if (board.motionDetectionMethod == MDM_MPU) {
        if (accelGyroNeedsCalibration) {
            calibrateAccelGyro();
            accelGyroNeedsCalibration = false;
        }
        if (magNeedsCalibration) {
            calibrateMag();
            magNeedsCalibration = false;
        }
        if (!updateEnabled)
            return;
        if (!device->update())
            return;

        float angle = device->getYaw() + 180.0;  // -180...180 -> 0...360

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
                        // Serial.printf("[MOTION] Crank event skip, dt too small: %ldms\n", dt);
                    }
                }
                lastCrankEventTime = t;
            }
            _halfRevolution = !_halfRevolution;
        }
        _previousTime = t;
        _previousAngle = angle;
    } else if (board.motionDetectionMethod == MDM_HALL) {
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
                    // Serial.printf("[MOTION] Crank event skip, dt too small: %ldms\n", dt);
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
    Serial.print("[MOTION] hall() ");
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

// Enable wake-on-motion and go to sleep
void Motion::enableWomSleep(void) {
    // Todo enable waking on hall sensor (https://esp32.com/viewtopic.php?t=4608)
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("Enabling W-O-M sleep");
    updateEnabled = false;
    delay(20);
    device->enableWomSleep();
}

void Motion::calibrateAccelGyro() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("Accel and Gyro calibration, please leave the device still.");
    updateEnabled = false;
    device->calibrateAccelGyro();
    updateEnabled = true;
}

void Motion::calibrateMag() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("Mag calibration, please wave device in a figure eight for 15 seconds.");
    updateEnabled = false;
    device->calibrateMag();
    updateEnabled = true;
}

void Motion::calibrate() {
    device->calibrateAccelGyro();
    device->calibrateMag();
    printSettings();
    saveSettings();
}

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
    printAccelGyroCalibration();
    printMagCalibration();
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

void Motion::printAccelGyroCalibration() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("%16s ---------X--------------Y--------------Z------\n", preferencesNS);
    log_i("Accel bias [g]:    %14f %14f %14f",
          device->getAccBiasX() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
          device->getAccBiasY() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY,
          device->getAccBiasZ() * 1000.f / (float)MPU9250::CALIB_ACCEL_SENSITIVITY);
    log_i("Gyro bias [deg/s]: %14f %14f %14f",
          device->getGyroBiasX() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
          device->getGyroBiasY() / (float)MPU9250::CALIB_GYRO_SENSITIVITY,
          device->getGyroBiasZ() / (float)MPU9250::CALIB_GYRO_SENSITIVITY);
    log_i("---------------------------------------------------------------");
}

void Motion::printMagCalibration() {
    if (board.motionDetectionMethod != MDM_MPU) return;
    log_i("%16s ---------X--------------Y--------------Z------", preferencesNS);
    log_i("Mag bias [mG]:     %14f %14f %14f",
          device->getMagBiasX(),
          device->getMagBiasY(),
          device->getMagBiasZ());
    log_i("Mag scale:         %14f %14f %14f",
          device->getMagScaleX(),
          device->getMagScaleY(),
          device->getMagScaleZ());
    log_i("---------------------------------------------------------------");
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
        if (!preferences->getBool("calibrated", false)) {
            preferencesEnd();
            log_e("[MOTION] MOTION has not yet been calibrated");
            return;
        }
        device->setAccBias(
            _prefGetValidFloat("abX", 0),
            _prefGetValidFloat("abY", 0),
            _prefGetValidFloat("abZ", 0));
        device->setGyroBias(
            _prefGetValidFloat("gbX", 0),
            _prefGetValidFloat("gbY", 0),
            _prefGetValidFloat("gbZ", 0));
        device->setMagBias(
            _prefGetValidFloat("mbX", 0),
            _prefGetValidFloat("mbY", 0),
            _prefGetValidFloat("mbZ", 0));
        device->setMagScale(
            _prefGetValidFloat("msX", 1),
            _prefGetValidFloat("msY", 1),
            _prefGetValidFloat("msZ", 1));
    }
    hallOffset = preferences->getInt("hallO", hallOffset);
    hallThreshold = preferences->getInt("hallT", hallThreshold);
    hallThresLow = preferences->getInt("hallTL", hallThresLow);
    preferencesEnd();
    printSettings();
}

void Motion::saveSettings() {
    if (!preferencesStartSave()) return;
    if (board.motionDetectionMethod == MDM_MPU) {
        _prefPutValidFloat("abX", device->getAccBiasX());
        _prefPutValidFloat("abY", device->getAccBiasY());
        _prefPutValidFloat("abZ", device->getAccBiasZ());
        _prefPutValidFloat("gbX", device->getGyroBiasX());
        _prefPutValidFloat("gbY", device->getGyroBiasY());
        _prefPutValidFloat("gbZ", device->getGyroBiasZ());
        _prefPutValidFloat("mbX", device->getMagBiasX());
        _prefPutValidFloat("mbY", device->getMagBiasY());
        _prefPutValidFloat("mbZ", device->getMagBiasZ());
        _prefPutValidFloat("msX", device->getMagScaleX());
        _prefPutValidFloat("msY", device->getMagScaleY());
        _prefPutValidFloat("msZ", device->getMagScaleZ());
        preferences->putBool("calibrated", true);
    }
    preferences->putInt("hallO", (int32_t)hallOffset);
    preferences->putInt("hallT", (int32_t)hallThreshold);
    preferences->putInt("hallTL", (int32_t)hallThresLow);
    preferencesEnd();
}

float Motion::_prefGetValidFloat(const char *key, const float_t defaultValue) {
    float f = preferences->getFloat(key, defaultValue);
    log_i("[MOTION] loaded %f for (%s, %f) from %s", f, key, defaultValue, preferencesNS);
    if (isinf(f) || isnan(f)) {
        log_e("[MOTION] invalid, returning default %f for %s", defaultValue, key);
        f = defaultValue;
    }
    return f;
}

size_t Motion::_prefPutValidFloat(const char *key, const float_t value) {
    size_t written = 0;
    if (isinf(value) || isnan(value)) {
        log_e("[MOTION] invalid, not saving %f for %s", value, key);
        return written;
    }
    written = preferences->putFloat(key, value);
    log_i("[MOTION] saved %f for %s in %s", value, key, preferencesNS);
    return written;
}
