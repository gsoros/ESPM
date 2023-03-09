#include "power.h"
#include "board.h"
#include "motion.h"
#include "strain.h"

void Power::setup(::Preferences *p) {
    _powerBuf.clear();
    preferencesSetup(p, "POWER");
    loadSettings();
}

void Power::loop() {
    if (_lastCrankEventTime < millis() - POWER_ZERO_DELAY_MS) {
        _powerBuf.push(0.0);
    }
}

void Power::onCrankEvent(const ulong msSinceLastEvent) {
    _lastCrankEventTime = millis();
    if (!board.strain.dataReady()) {
        // log_e("strain not ready, skipping loop at %d, SPS=%f", millis(), board.strain.device->getSPS());
        return;
    }
    /*
    double deltaT = msSinceLastEvent / 1000.0;  // t(s)
    float radius = crankLength(mm) / 1000.0;    // r(m)
    float distance = 2.0 * radius * PI;         // s(m)   = 2 * r(m) * π
    double velocity = distance / deltaT;        // v(m/s) = s(m) / t(s)
    float mass = board.strain.value(true);      // m(kg)
    float force = mass * 9.80665;               // F(N)   = m(kg) * G(m/s/s)
    float power = force * velocity;             // P(W)   = F(N) * v(m/s)
                                                // P      = m * G * v
                                                // P      = m * 9.80665 * s / t
                                                // P      = m * 9.80665 * 2 * r * π / t
                                                // power  = board.strain.value(true) * 9.80665 * 2 * crankLength / 1000.0 * π / (msSinceLastEvent / 1000.0)
                                                // power  = board.strain.value(true) / msSinceLastEvent * crankLength * 9.80665 * 2  / 1000.0 * π  * 1000.0
                                                // power  = board.strain.value(true) / msSinceLastEvent * crankLength * 9.80665 * 2 * π
                                                // power  = board.strain.value(true) / msSinceLastEvent * crankLength * 61.616999192652692
    */
    float power = board.strain.value(true) / msSinceLastEvent * crankLength * 61.616999192652692;
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
    log_i("Settings: crank length: %.2fmm, strain is %sreversed, MPU is %sreversed, value is %sdoubled",
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
