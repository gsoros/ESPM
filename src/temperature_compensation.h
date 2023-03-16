#if !defined(__temperature_compensation_h) && defined(FEATURE_TEMPERATURE_COMPENSATION)
#define __temperature_compensation_h

#include <Arduino.h>
#include "atoll_preferences.h"
#include "api.h"

#ifndef TC_TABLE_SIZE
#define TC_TABLE_SIZE 256  // temperature correction lookup table size
#endif

#ifndef TC_TABLE_KEY_OFFSET
#define TC_TABLE_KEY_OFFSET -15  // first element holds the correction value at -15˚C
#endif

#ifndef TC_TABLE_KEY_RESOLUTION
#define TC_TABLE_KEY_RESOLUTION 0.2F  // 50th element holds the correction value at -5˚C (OFFSET + 50 * RESOLUTION)
#endif

#ifndef TC_TABLE_VALUE_RESOLUTION
#define TC_TABLE_VALUE_RESOLUTION 0.1F  // a value of 10 means +1kg correction
#endif

#define TC_TABLE_VALUE_MIN INT8_MIN
#define TC_TABLE_VALUE_MAX INT8_MAX

class TemperatureCompensation : public Atoll::Preferences {
   public:
    TemperatureCompensation(
        uint16_t size = TC_TABLE_SIZE,
        int16_t keyOffset = TC_TABLE_KEY_OFFSET,
        float keyResolution = TC_TABLE_KEY_RESOLUTION,
        float valueResolution = TC_TABLE_VALUE_RESOLUTION);
    ~TemperatureCompensation();

    void setup(::Preferences *p);

    uint16_t getSize();
    bool setSize(uint16_t size);

    int16_t getKeyOffset();
    bool setKeyOffset(int16_t offset);

    float getKeyResolution();
    bool setKeyResolution(float resolution);

    float getValueResolution();
    bool setValueResolution(float resolution);

    int8_t getValue(uint16_t index);
    bool setValue(uint16_t index, int8_t value);
    bool unsetValue(uint16_t index);

    bool validIndex(uint16_t index);

    bool enabled = false;

   protected:
    int8_t *values = nullptr;
    uint16_t size = 0;
    int8_t keyOffset = 0;
    float keyResolution = 0.0f;
    float valueResolution = 0.0f;

   public:
    const int8_t valueMin = TC_TABLE_VALUE_MIN;
    const int8_t valueMax = TC_TABLE_VALUE_MAX;

    void loadSettings();
    void saveSettings();
    void printSettings();

    void addApiCommand();
    Api::Result *tcProcessor(Api::Message *msg);
};

#endif