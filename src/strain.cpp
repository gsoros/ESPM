#include "board.h"
#include "strain.h"

void Strain::setup(const gpio_num_t doutPin,
                   const gpio_num_t sckPin,
                   Preferences *p,
                   const char *preferencesNS) {
    this->doutPin = doutPin;
    this->sckPin = sckPin;
    preferencesSetup(p, preferencesNS);
    rtc_gpio_hold_dis(sckPin);
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
    if (1 != device->update()) {  // 1: data ready; 2: tare complete
        return;
    }
    _measurementBuf.push(device->getData());
}

// returns the average of the measurement values in the buffer, optionally emtying the buffer
float Strain::value(bool clearBuffer) {
    float avg = 0.0;
    for (decltype(_measurementBuf)::index_t i = 0; i < _measurementBuf.size(); i++) {
        avg += _measurementBuf[i] / _measurementBuf.size();
    }
    if (clearBuffer) _measurementBuf.clear();
    return avg;
}

float Strain::liveValue() {
    return _measurementBuf.last();
}

bool Strain::dataReady() {
    return _measurementBuf.size();
}

void Strain::sleep() {
    device->powerDown();
    rtc_gpio_hold_en(sckPin);
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
    if (!preferences->getBool("calibrated", false)) {
        preferencesEnd();
        log_e("[Strain] Device has not yet been calibrated");
        return;
    }
    device->setCalFactor(preferences->getFloat("calibration", 0));
    preferencesEnd();
}

void Strain::saveCalibration() {
    if (!preferencesStartSave()) return;
    preferences->putBool("calibrated", true);
    preferences->putFloat("calibration", device->getCalFactor());
    preferencesEnd();
}

void Strain::tare() {
    device->tare();
}
