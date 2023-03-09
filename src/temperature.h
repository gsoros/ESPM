#if !defined(__temperature_h) && defined(FEATURE_TEMPERATURE)
#define __temperature_h

#include "definitions.h"
#include "atoll_temperature_sensor.h"
#include "atoll_preferences.h"

#ifndef TC_TABLE_SIZE
#define TC_TABLE_SIZE 50  // temperature correction lookup table size
#endif

#ifndef TC_TABLE_OFFSET
#define TC_TABLE_OFFSET -15  // first element corresponds to the correction value at -15˚C
#endif

#ifndef TC_TABLE_MAGNITUDE
#define TC_TABLE_MAGNITUDE 0.1F  // a value of 10 means +1kg correction
#endif

#define TC_VALUE_EMPTY INT8_MIN

class Temperature : public Atoll::Preferences {
   public:
    typedef Atoll::TemperatureSensor Sensor;
    typedef int8_t CorrectionTable[TC_TABLE_SIZE];

    Sensor *crankSensor = nullptr;

    Temperature();
    ~Temperature();

    void setup();
    void begin();

    void onSensorValueChange(Sensor *sensor);

    size_t tcTableSize();

    int8_t tcTableGetValue(uint16_t index);
    bool tcTableSetValue(uint16_t index, int8_t value);
    bool tcTableUnsetValue(uint16_t index);
    float tcGetCorrection();

   protected:
    bool tcTableValidIndex(uint16_t index);
    void tcSetCorrection(float temperature);

    float correction = 0.0f;  // kg
    CorrectionTable tcTable;
};

#endif