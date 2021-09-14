#include "board.h"
#include "mpu.h"

#include "driver/adc.h"

void MPU::setup(const uint8_t sdaPin,
                const uint8_t sclPin,
                Preferences *p) {
    setup(sdaPin, sclPin, p, "MPU", 0x68);
}
void MPU::setup(const uint8_t sdaPin,
                const uint8_t sclPin,
                Preferences *p,
                const char *preferencesNS,
                uint8_t mpuAddress) {
    preferencesSetup(p, preferencesNS);
    if (MOVEMENT_DETECTION_METHOD == MD_MPU) {
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
            Serial.println("[MPU] Setup error");
        //device->selectFilter(QuatFilterSel::MAHONY);
        device->selectFilter(QuatFilterSel::NONE);
        //device->selectFilter(QuatFilterSel::MADGWICK);
        //device->setMagneticDeclination(5 + 19 / 60);  // 5° 19'
        loadCalibration();
        //printCalibration();
    } else {  // MOVEMENT_DETECTION_METHOD == MD_HALL
        adc1_config_width(ADC_WIDTH_BIT_12);
    }
    updateEnabled = true;
    lastMovement = millis();
}

void MPU::loop() {
    const ulong t = millis();

    if (MOVEMENT_DETECTION_METHOD == MD_MPU) {
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
                        Serial.printf("[MPU] Crank event #%d dt: %ldms\n", revolutions, tDiff);
                        board.power.onCrankEvent(tDiff);
                        board.ble.onCrankEvent(t, revolutions);
                    } else {
                        //Serial.printf("[MPU] Crank event skip, dt too small: %ldms\n", tDiff);
                    }
                }
                lastCrankEventTime = t;
            }
            _halfRevolution = !_halfRevolution;
        }
        _previousTime = t;
        _previousAngle = angle;
    } else {  // MOVEMENT_DETECTION_METHOD == MD_HALL
        //_hallBuf.push(hall_sensor_read() + HALL_DEFAULT_OFFSET);
        if (HALL_DEFAULT_THRESHOLD < abs(hall())) {
            lastMovement = t;
            if (0 < lastCrankEventTime) {
                ulong tDiff = t - lastCrankEventTime;
                if (300 < tDiff) {  // 300 ms = 200 RPM
                    revolutions++;
                    Serial.printf("[MPU] Crank event #%d dt: %ldms\n", revolutions, tDiff);
                    board.power.onCrankEvent(tDiff);
                    board.ble.onCrankEvent(t, revolutions);
                } else {
                    //Serial.printf("[MPU] Crank event skip, dt too small: %ldms\n", tDiff);
                }
            }
            lastCrankEventTime = t;
        }
    }
}

int MPU::hall() {
    float avg = 0.0;
    for (int i = 0; i < HALL_DEFAULT_SAMPLES; i++) {
        avg += hall_sensor_read() / HALL_DEFAULT_SAMPLES;
    }
    lastHallValue = (int)avg + HALL_DEFAULT_OFFSET;
    return lastHallValue;
}

// Enable wake-on-motion and go to sleep
void MPU::enableWomSleep(void) {
    Serial.println("[MPU] Enabling W-O-M sleep");
    updateEnabled = false;
    delay(20);
    device->enableWomSleep();
}

void MPU::calibrateAccelGyro() {
    Serial.println("[MPU] Accel and Gyro calibration, please leave the device still.");
    updateEnabled = false;
    device->calibrateAccelGyro();
    updateEnabled = true;
}

void MPU::calibrateMag() {
    Serial.println("[MPU] Mag calibration, please wave device in a figure eight for 15 seconds.");
    updateEnabled = false;
    device->calibrateMag();
    updateEnabled = true;
}

void MPU::calibrate() {
    device->calibrateAccelGyro();
    device->calibrateMag();
    printCalibration();
    saveCalibration();
}

void MPU::printCalibration() {
    printAccelGyroCalibration();
    printMagCalibration();
}

void MPU::printAccelGyroCalibration() {
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

void MPU::printMagCalibration() {
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

void MPU::loadCalibration() {
    if (!preferencesStartLoad()) return;
    if (!preferences->getBool("calibrated", false)) {
        preferencesEnd();
        log_e("[MPU] MPU has not yet been calibrated");
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
    preferencesEnd();
    printCalibration();
}

void MPU::saveCalibration() {
    if (!preferencesStartSave()) return;
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
    preferencesEnd();
}

float MPU::_prefGetValidFloat(const char *key, const float_t defaultValue) {
    float f = preferences->getFloat(key, defaultValue);
    log_i("[MPU] loaded %f for (%s, %f) from %s", f, key, defaultValue, preferencesNS);
    if (isinf(f) || isnan(f)) {
        log_e("[MPU] invalid, returning default %f for %s", defaultValue, key);
        f = defaultValue;
    }
    return f;
}

size_t MPU::_prefPutValidFloat(const char *key, const float_t value) {
    size_t written = 0;
    if (isinf(value) || isnan(value)) {
        log_e("[MPU] invalid, not saving %f for %s", value, key);
        return written;
    }
    written = preferences->putFloat(key, value);
    log_i("[MPU] saved %f for %s in %s", value, key, preferencesNS);
    return written;
}
