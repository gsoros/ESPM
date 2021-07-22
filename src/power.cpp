#include "power.h"
#include "board.h"
#include "mpu.h"
#include "strain.h"

void Power::setup(Preferences *p) {
    preferencesSetup(p, "POWER");
    loadSettings();
}

void Power::loop() {
    if (_lastCrankEventTime < millis() - 1000) {
        _powerBuf.push(0.0);
    }
}

void Power::onCrankEvent(const ulong msSinceLastEvent) {
    _lastCrankEventTime = millis();
    if (!board.strain.dataReady()) {
        //log_e("strain not ready, skipping loop at %d, SPS=%f", millis(), board.strain.device->getSPS());
        return;
    }
    double deltaT = msSinceLastEvent / 1000.0;  // t(s)
    float radius = crankLength / 1000.0;        // r(m)
    float distance = 2.0 * radius * PI;         // s(m)   = 2 * r(m) * π
    double velocity = distance / deltaT;        // v(m/s) = s(m) / t(s)
    float mass = board.strain.value(true);      // m(kg)
    float force = mass * 9.80665;               // F(N)   = m(kg) * G(m/s/s)
    float power = force * velocity;             // P(W)   = F(N) * v(m/s)
    if (reportDouble) power *= 2;
    if (power < 0.0)
        power = 0.0;
    else if (10000.0 < power)
        power = 10000.0;
    _powerBuf.push(power);
}

// Returns the average of the buffered power values, optionally emptying the buffer.
// Measurements are added to the buffer once per crank revolution.
float Power::power(bool clearBuffer) {
    float power = 0;
    if (_powerBuf.isEmpty()) return power;
    for (decltype(_powerBuf)::index_t i = 0; i < _powerBuf.size(); i++) {
        power += _powerBuf[i] / _powerBuf.size();
    }
    if (clearBuffer) _powerBuf.clear();
    return power;
}

void Power::loadSettings() {
    if (!preferencesStartLoad()) return;
    crankLength = preferences->getFloat("crankLength", 172.5);
    reverseMPU = preferences->getBool("reverseMPU", false);
    reverseStrain = preferences->getBool("reverseStrain", false);
    reportDouble = preferences->getBool("reportDouble", true);
    preferencesEnd();
}

void Power::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putFloat("crankLength", crankLength);
    preferences->putBool("reverseMPU", reverseMPU);
    preferences->putBool("reverseStrain", reverseStrain);
    preferences->putBool("reportDouble", reportDouble);
    preferencesEnd();
}

void Power::printSettings() {
    Serial.printf(
        "[Power] Settings\nCrank length: %.2fmm\nStrain is %sreversed\nMPU is %sreversed\nValue is %sdoubled\n",
        crankLength, reverseStrain ? "" : "not ",
        reverseMPU ? "" : "not ",
        reportDouble ? "" : "not ");
}

float Power::filterNegative(float value, bool reverse) {
    if (reverse) value *= -1;
    if (value < 0.0)
        return 0.0;
    else
        return value;
}
