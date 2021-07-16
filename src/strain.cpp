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
    ulong stabilizingTime = 1000 / 80;
    bool tare = true;
    device->start(stabilizingTime, tare);
    if (device->getTareTimeoutFlag()) {
        log_e("HX711 Tare Timeout");
    }
    loadCalibration();
}

void Strain::loop() {
    if (!device->update()) {
        return;
    }
    _measurement = device->getData();
    //_measurement = device->getDataWithoutSmoothing();
    _dataReady = true;
}

float Strain::measurement(bool unsetDataReadyFlag) {
    if (unsetDataReadyFlag) _dataReady = false;
    return _measurement;
}

bool Strain::dataReady() {
    return _dataReady;
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
        Serial.println("Error setting calibraton factor.");
        return -1;
    }
    device->getNewCalibration(knownMass);
    return 0;
}

void Strain::printCalibration() {
    Serial.printf("Strain calibration factor: %f\n", device->getCalFactor());
}

void Strain::loadCalibration() {
    if (!preferencesStartLoad()) return;
    if (!preferences->getBool("calibrated", false)) {
        preferencesEnd();
        log_e("Device has not yet been calibrated");
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
