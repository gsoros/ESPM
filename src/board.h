#ifndef BOARD_H___
#define BOARD_H___

#include <Arduino.h>
#include <Preferences.h>

#include "definitions.h"

#ifdef FEATURE_SERIAL
#include "atoll_split_stream.h"
#include "atoll_wifi_serial.h"
#else
#include "atoll_null_serial.h"
#endif

#include "atoll_preferences.h"
#include "atoll_task.h"
#include "ble_server.h"
#include "api.h"
#include "atoll_battery.h"
#include "wifi.h"
#include "atoll_ota.h"

#ifdef FEATURE_TEMPERATURE
#include "atoll_temperature_sensor.h"
#endif

#include "motion.h"
#include "strain.h"
#include "power.h"
// #include "status.h"
#include "led.h"
#include "atoll_log.h"

// The one in charge
class Board : public Atoll::Task,
              public Atoll::Preferences {
   public:
    const char *taskName() { return "Board"; }
    ::Preferences arduinoPreferences = ::Preferences();
#ifdef FEATURE_SERIAL
    HardwareSerial hwSerial = HardwareSerial(0);
    Atoll::WifiSerial wifiSerial;
#endif
    Wifi wifi;
    BleServer bleServer;
    Api api;
    Atoll::Battery battery;
    Motion motion;
    Strain strain;
    Power power;
    Atoll::Ota ota;
    // Status status;
    Led led;

#ifdef FEATURE_TEMPERATURE
    typedef Atoll::TemperatureSensor Temp;
    Temp *crankTemperature = nullptr;
    void onTempChange(Temp *sensor);
#endif

    bool otaMode = false;
    bool sleepEnabled = true;
    ulong sleepDelay = SLEEP_DELAY_DEFAULT;
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    uint8_t motionDetectionMethod = MOTION_DETECTION_METHOD;

    void setup();
    void setupTask(const char *taskName);
    void startTasks();
    void startTask(const char *taskName);
    void stopTask(const char *taskName);
    void restartTask(const char *taskName);
    void loop();
    bool loadSettings();
    void saveSettings();
    // Returns time in ms until entering deep sleep, or -1 in case of no such plans.
    long timeUntilDeepSleep(ulong t = 0);
    int deepSleep();
    void reboot();
    float getStrain(bool clearBuffer = false);
    float getLiveStrain();
    float getPower(bool clearBuffer = false);
    void setSleepDelay(const ulong delay);
    void setMotionDetectionMethod(int method);

   private:
    const ulong _sleepCountdownAfter = SLEEP_COUNTDOWN_AFTER;
    const ulong _sleepCountdownEvery = SLEEP_COUNTDOWN_EVERY;
    ulong _lastSleepCountdown = 0;
};

extern Board board;

#endif