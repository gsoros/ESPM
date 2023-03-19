#if defined(FEATURE_MPU_TEMPERATURE) && defined(FEATURE_MPU) && defined(FEATURE_TEMPERATURE)

#include "board.h"
#include "mpu_temperature.h"

MpuTemperature::MpuTemperature(
    const char *label,
    float updateFrequency,
    Callback onValueChange)
    : Atoll::TemperatureSensor(
          label,
          updateFrequency),
      onValueChange(onValueChange){};

bool MpuTemperature::update() {
    // round down to one decimal
    updateValue((int)(board.motion.getMpuTemperature() * 10) / 10.0f);
    return true;
}

void MpuTemperature::callOnValueChange() {
    if (onValueChange) onValueChange(this);
}

#endif