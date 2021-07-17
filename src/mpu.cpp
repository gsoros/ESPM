#include "board.h"
#include "mpu.h"

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
    Wire.begin(sdaPin, sclPin);
    vTaskDelay(100);
    device = new MPU9250();
    //device->verbose(true);
    MPU9250Setting s;
    s.skip_mag = true;
    s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
    //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_10HZ;
    //s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_3600HZ;
    //s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_10HZ;
    //s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_420HZ;
    //s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
    if (!device->setup(mpuAddress, s))
        Serial.println("[E] MPU Setup error");
    //device->selectFilter(QuatFilterSel::MAHONY);
    device->selectFilter(QuatFilterSel::NONE);
    //device->selectFilter(QuatFilterSel::MADGWICK);
    //device->setMagneticDeclination(5 + 19 / 60);  // 5° 19'
    loadCalibration();
    //printCalibration();
    updateEnabled = true;
    lastMovement = millis();
}

void MPU::loop() {
    if (accelGyroNeedsCalibration) {
        calibrateAccelGyro();
        accelGyroNeedsCalibration = false;
    }
    if (magNeedsCalibration) {
        calibrateMag();
        magNeedsCalibration = false;
    }
    if (!updateEnabled) {
        return;
    }
    if (!device->update()) {
        return;
    }
    const ulong t = millis();
    static ulong previousTime = 0;
    static float previousAngle = 0.0;
    float angle = device->getYaw() + 180.0;  // 0...360
    float newRpm = 0.0;
    if (0 < previousTime && !_rpmBuf.isEmpty()) {
        ushort dT = t - previousTime;  // ms
        if (0 < dT) {
            float dA = angle - previousAngle;  // deg
            // roll over zero deg, will fail when spinning faster than 180 deg/dT
            if (-360 < dA && dA <= -180) {
                dA += 360;
            }
            newRpm = dA / dT / 0.006;             // 1 deg/ms / .006 = 1 rpm
            if (newRpm < -200 || 200 < newRpm) {  // remove value > 200 rpm
                log_d("invalid rpm %f", newRpm);
                newRpm = 0.0;
            }
            if (-1 < newRpm && newRpm < 1) {  // remove noise from yaw drift at rest
                newRpm = 0.0;
            }
        }
    }
    _rpmBuf.push(newRpm);
    if (_rpmBuf.size()) {
        float avg = 0.0;
        for (decltype(_rpmBuf)::index_t i = 0; i < _rpmBuf.size(); i++) {
            avg += _rpmBuf[i] / _rpmBuf.size();
        }
        _rpm = avg;
        if (avg < -1 || 1 < avg) {
            lastMovement = t;
        }
        _dataReady = true;
    }
    if ((previousAngle < 180.0 && 180.0 <= angle) || (angle < 180.0 && 180.0 <= previousAngle)) {
        static bool secondCrankEvent = false;
        if (!secondCrankEvent) {
            if (0 < prevCrankEventTime) {
                ulong tmpDiff = t - prevCrankEventTime;
                if (300 < tmpDiff) {  // 300 ms = 200 RPM
                    lastCrankEventTimeDiff = tmpDiff;
                    revolutions++;
                    crankEventReady = true;
                    Serial.printf("[MPU] Crank event #%d dt: %ldms\n", revolutions, tmpDiff);
                } else {
                    Serial.printf("[MPU] Crank event skip, dt too small: %ldms\n", tmpDiff);
                }
            }
            prevCrankEventTime = t;
        }
        secondCrankEvent = !secondCrankEvent;
    }
    previousTime = t;
    previousAngle = angle;
}

float MPU::rpm(bool unsetDataReadyFlag) {
    if (unsetDataReadyFlag) _dataReady = false;
    //return 60.0;
    return _rpm;
}

bool MPU::dataReady() {
    return _dataReady;
}

MPU::Quaternion MPU::quaternion() {
    Quaternion q;
    q.x = device->getQuaternionX();
    q.y = device->getQuaternionY();
    q.z = device->getQuaternionZ();
    q.w = device->getQuaternionW();
    return q;
}

void MPU::enableWomSleep(void) {
    Serial.println("Enabling MPU W-O-M sleep");
    updateEnabled = false;
    delay(20);
    device->enableWomSleep();
}

void MPU::calibrateAccelGyro() {
    Serial.println("Accel and Gyro calibration, please leave the device still.");
    updateEnabled = false;
    device->calibrateAccelGyro();
    updateEnabled = true;
}

void MPU::calibrateMag() {
    Serial.println("Mag calibration, please wave device in a figure eight for 15 seconds.");
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
        log_e("MPU has not yet been calibrated");
        return;
    }
    device->setAccBias(
        prefGetValidFloat("abX", 0),
        prefGetValidFloat("abY", 0),
        prefGetValidFloat("abZ", 0));
    device->setGyroBias(
        prefGetValidFloat("gbX", 0),
        prefGetValidFloat("gbY", 0),
        prefGetValidFloat("gbZ", 0));
    device->setMagBias(
        prefGetValidFloat("mbX", 0),
        prefGetValidFloat("mbY", 0),
        prefGetValidFloat("mbZ", 0));
    device->setMagScale(
        prefGetValidFloat("msX", 1),
        prefGetValidFloat("msY", 1),
        prefGetValidFloat("msZ", 1));
    preferencesEnd();
    printCalibration();
}

void MPU::saveCalibration() {
    if (!preferencesStartSave()) return;
    prefPutValidFloat("abX", device->getAccBiasX());
    prefPutValidFloat("abY", device->getAccBiasY());
    prefPutValidFloat("abZ", device->getAccBiasZ());
    prefPutValidFloat("gbX", device->getGyroBiasX());
    prefPutValidFloat("gbY", device->getGyroBiasY());
    prefPutValidFloat("gbZ", device->getGyroBiasZ());
    prefPutValidFloat("mbX", device->getMagBiasX());
    prefPutValidFloat("mbY", device->getMagBiasY());
    prefPutValidFloat("mbZ", device->getMagBiasZ());
    prefPutValidFloat("msX", device->getMagScaleX());
    prefPutValidFloat("msY", device->getMagScaleY());
    prefPutValidFloat("msZ", device->getMagScaleZ());
    preferences->putBool("calibrated", true);
    preferencesEnd();
}

float MPU::prefGetValidFloat(const char *key, const float_t defaultValue) {
    float f = preferences->getFloat(key, defaultValue);
    log_i("loaded %f for (%s, %f) from %s", f, key, defaultValue, preferencesNS);
    if (isinf(f) || isnan(f)) {
        log_e("invalid, returning default %f for %s", defaultValue, key);
        f = defaultValue;
    }
    return f;
}

size_t MPU::prefPutValidFloat(const char *key, const float_t value) {
    size_t written = 0;
    if (isinf(value) || isnan(value)) {
        log_e("invalid, not saving %f for %s", value, key);
        return written;
    }
    written = preferences->putFloat(key, value);
    log_i("saved %f for %s in %s", value, key, preferencesNS);
    return written;
}
