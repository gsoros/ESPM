#include "board.h"
#include "strain.h"

void Strain::setup(const gpio_num_t doutPin,
                   const gpio_num_t sckPin,
                   Preferences *p,
                   const char *preferencesNS) {
    this->doutPin = doutPin;
    this->sckPin = sckPin;
    preferencesSetup(p, preferencesNS);
    rtc_gpio_hold_dis(sckPin);  // required to put HX711 into sleep mode
    device = new HX711_ADC(doutPin, sckPin);
    device->begin();
    Serial.print("[STRAIN] Starting HX711, waiting for tare...");
    ulong stabilizingTime = 1000 / 80;  // 80 sps
    device->start(stabilizingTime, false);
    device->tare();
    Serial.println(" done.");
    if (device->getTareTimeoutFlag()) {
        Serial.println("[Strain] HX711 tare timeout");
    }
    loadCalibration();
}

void Strain::loop() {
    if (1 != device->update())  // 1: data ready; 2: tare complete
        return;
    _measurementBuf.push(device->getData());
    if (board.motionDetectionMethod == MDM_STRAIN) {
        if (!_halfRevolution) {
            if (_measurementBuf.last() <= mdmStrainThresLow) {
                _halfRevolution = true;
            }
        } else if (mdmStrainThreshold <= _measurementBuf.last()) {
            _halfRevolution = false;
            ulong t = millis();
            board.motion.lastMovement = t;
            if (0 < board.motion.lastCrankEventTime) {
                ulong tDiff = t - board.motion.lastCrankEventTime;
                if (300 < tDiff) {  // 300 ms = 200 RPM
                    board.motion.revolutions++;
                    Serial.printf("[STRAIN] Crank event #%d dt: %ldms\n", board.motion.revolutions, tDiff);
                    board.power.onCrankEvent(tDiff);
                    board.ble.onCrankEvent(t, board.motion.revolutions);
                    board.motion.lastCrankEventTime = t;
                } else {
                    //Serial.printf("[MOTION] Crank event skip, dt too small: %ldms\n", tDiff);
                }
            } else {
                board.motion.lastCrankEventTime = t;
            }
        }
    }
}

// returns the average of the measurement values in the buffer, optionally emtying the buffer
float Strain::value(bool clearBuffer) {
    float avg = 0.0;
    if (!dataReady()) return avg;
    for (decltype(_measurementBuf)::index_t i = 0; i < _measurementBuf.size(); i++) {
        avg += _measurementBuf[i] / _measurementBuf.size();
    }
    if (clearBuffer) _measurementBuf.clear();
    return avg;
}

float Strain::liveValue() {
    return dataReady() ? _measurementBuf.last() : 0.0;
}

bool Strain::dataReady() {
    return 0 < _measurementBuf.size();
}

void Strain::sleep() {
    device->powerDown();
    rtc_gpio_hold_en(sckPin);
}

void Strain::setMdmStrainThreshold(int threshold) {
    mdmStrainThreshold = threshold;
}

void Strain::setMdmStrainThresLow(int threshold) {
    mdmStrainThresLow = threshold;
}

// calibrate to a known mass in kg
int Strain::calibrateTo(float knownMass) {
    float calFactor = device->getCalFactor();
    if (isnan(calFactor) ||
        isinf(calFactor) ||
        (-0.000000001 < calFactor && calFactor < 0.000000001)) {
        device->setCalFactor(1.0);
        delay(10);
    }
    if (isnan(knownMass) ||
        isinf(knownMass) ||
        (-0.000000001 < knownMass && knownMass < 0.000000001)) {
        Serial.println("[Strain] Error setting calibraton factor.");
        return -1;
    }
    device->getNewCalibration(knownMass);
    return 0;
}

void Strain::printCalibration() {
    Serial.printf("[Strain] Calibration factor: %f\n", device->getCalFactor());
}

void Strain::loadCalibration() {
    if (!preferencesStartLoad()) return;
    mdmStrainThreshold = preferences->getInt("mdmSThres", mdmStrainThreshold);
    mdmStrainThresLow = preferences->getInt("mdmSThresL", mdmStrainThresLow);
    if (!preferences->getBool("calibrated", false)) {
        preferencesEnd();
        log_e("[Strain] Device has not yet been calibrated");
        return;
    }
    device->setCalFactor(preferences->getFloat("calibration", 0.0));
    preferencesEnd();
}

void Strain::saveCalibration() {
    if (!preferencesStartSave()) return;
    preferences->putInt("mdmSThres", mdmStrainThreshold);
    preferences->putInt("mdmSThresL", mdmStrainThresLow);
    preferences->putBool("calibrated", true);
    preferences->putFloat("calibration", device->getCalFactor());
    preferencesEnd();
}

void Strain::tare() {
    device->tare();
}
