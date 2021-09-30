#include "board.h"
#include "motion.h"

#include "driver/adc.h"

void MOTION::setup(const uint8_t sdaPin,
                   const uint8_t sclPin,
                   Preferences *p) {
    setup(sdaPin, sclPin, p, "MOTION", 0x68);
}
void MOTION::setup(const uint8_t sdaPin,
                   const uint8_t sclPin,
                   Preferences *p,
                   const char *preferencesNS,
                   uint8_t mpuAddress) {
    preferencesSetup(p, preferencesNS);
    if (detectionMethod == MDM_MPU) {
        Wire.begin(sdaPin, sclPin);
        vTaskDelay(100);
        device = new MPU9250();
        //device->verbose(true);
        MPU9250Setting s;
        s.skip_mag = true;  // compass not needed
        s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
        //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_10HZ;
        //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_3600HZ;
        //s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_10HZ;
        //s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_420HZ;
        //s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
        if (!device->setup(mpuAddress, s))
            Serial.println("[MOTION] Setup error");
        //device->selectFilter(QuatFilterSel::MAHONY);
        device->selectFilter(QuatFilterSel::NONE);
        //device->selectFilter(QuatFilterSel::MADGWICK);
        //device->setMagneticDeclination(5 + 19 / 60);  // 5° 19'
    } else {  // detectionMethod == MDM_HALL
        adc1_config_width(ADC_WIDTH_BIT_12);
    }
    loadCalibration();
    //printCalibration();
    updateEnabled = true;
    lastMovement = millis();
}

void MOTION::loop() {
    const ulong t = millis();

    if (detectionMethod == MDM_MPU) {
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
                    ulong tDiff = t - lastCrankEventTime;
                    if (300 < tDiff) {  // 300 ms = 200 RPM
                        revolutions++;
                        Serial.printf("[MOTION] Crank event #%d dt: %ldms\n", revolutions, tDiff);
                        board.power.onCrankEvent(tDiff);
                        board.ble.onCrankEvent(t, revolutions);
                    } else {
                        //Serial.printf("[MOTION] Crank event skip, dt too small: %ldms\n", tDiff);
                    }
                }
                lastCrankEventTime = t;
            }
            _halfRevolution = !_halfRevolution;
        }
        _previousTime = t;
        _previousAngle = angle;
    } else {  // detectionMethod == MDM_HALL
        if (!_halfRevolution) {
            if (abs(hall()) < hallThresLow) {
                _halfRevolution = true;
            }
        } else if (hallThreshold < abs(hall())) {
            _halfRevolution = false;
            lastMovement = t;
            if (0 < lastCrankEventTime) {
                ulong tDiff = t - lastCrankEventTime;
                if (300 < tDiff) {  // 300 ms = 200 RPM
                    revolutions++;
                    Serial.printf("[MOTION] Crank event #%d dt: %ldms\n", revolutions, tDiff);
                    board.power.onCrankEvent(tDiff);
                    board.ble.onCrankEvent(t, revolutions);
                    lastCrankEventTime = t;
                } else {
                    //Serial.printf("[MOTION] Crank event skip, dt too small: %ldms\n", tDiff);
                }
            } else {
                lastCrankEventTime = t;
            }
        }
    }
}

int MOTION::hall() {
    float avg = 0.0;
    for (int i = 0; i < HALL_DEFAULT_SAMPLES; i++) {
        avg += hall_sensor_read() / HALL_DEFAULT_SAMPLES;
        delayMicroseconds(3);
    }
    lastHallValue = (int)avg + hallOffset;
    return lastHallValue;
}

// Enable wake-on-motion and go to sleep
void MOTION::enableWomSleep(void) {
    // Todo enable waking on hall sensor (https://esp32.com/viewtopic.php?t=4608)
    if (detectionMethod != MDM_MPU) return;
    Serial.println("[MOTION] Enabling W-O-M sleep");
    updateEnabled = false;
    delay(20);
    device->enableWomSleep();
}

void MOTION::calibrateAccelGyro() {
    if (detectionMethod != MDM_MPU) return;
    Serial.println("[MOTION] Accel and Gyro calibration, please leave the device still.");
    updateEnabled = false;
    device->calibrateAccelGyro();
    updateEnabled = true;
}

void MOTION::calibrateMag() {
    if (detectionMethod != MDM_MPU) return;
    Serial.println("[MOTION] Mag calibration, please wave device in a figure eight for 15 seconds.");
    updateEnabled = false;
    device->calibrateMag();
    updateEnabled = true;
}

void MOTION::calibrate() {
    device->calibrateAccelGyro();
    device->calibrateMag();
    printCalibration();
    saveCalibration();
}

void MOTION::setMovementDetectionMethod(int method) {
    detectionMethod = method;
}

void MOTION::setHallOffset(int offset) {
    hallOffset = offset;
}

void MOTION::setHallThreshold(int threshold) {
    hallThreshold = threshold;
}

void MOTION::setHallThresLow(int threshold) {
    hallThresLow = threshold;
}

void MOTION::printCalibration() {
    printAccelGyroCalibration();
    printMagCalibration();
    printHallCalibration();
    Serial.printf("Movement detection method: %s\n",
                  (detectionMethod == MDM_MPU) ? "MPU" : "Hall effect sensor");
}

void MOTION::printAccelGyroCalibration() {
    if (detectionMethod != MDM_MPU) return;
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
}

void MOTION::printMagCalibration() {
    if (detectionMethod != MDM_MPU) return;
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
}

void MOTION::printHallCalibration() {
    Serial.printf("Hall offset: %d\nHall low threshold: %d\nHall high threshold: %d\n",
                  hallOffset,
                  hallThresLow,
                  hallThreshold);
}

void MOTION::loadCalibration() {
    if (!preferencesStartLoad()) return;
    detectionMethod = preferences->getInt("method", detectionMethod);
    detectionMethod = detectionMethod == MDM_MPU ? MDM_MPU : MDM_HALL;
    if (detectionMethod == MDM_MPU) {
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
    printCalibration();
}

void MOTION::saveCalibration() {
    if (!preferencesStartSave()) return;
    if (detectionMethod == MDM_MPU) {
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
    preferences->putInt("method", (int32_t)detectionMethod);
    preferencesEnd();
}

float MOTION::_prefGetValidFloat(const char *key, const float_t defaultValue) {
    float f = preferences->getFloat(key, defaultValue);
    log_i("[MOTION] loaded %f for (%s, %f) from %s", f, key, defaultValue, preferencesNS);
    if (isinf(f) || isnan(f)) {
        log_e("[MOTION] invalid, returning default %f for %s", defaultValue, key);
        f = defaultValue;
    }
    return f;
}

size_t MOTION::_prefPutValidFloat(const char *key, const float_t value) {
    size_t written = 0;
    if (isinf(value) || isnan(value)) {
        log_e("[MOTION] invalid, not saving %f for %s", value, key);
        return written;
    }
    written = preferences->putFloat(key, value);
    log_i("[MOTION] saved %f for %s in %s", value, key, preferencesNS);
    return written;
}
