#ifndef BOARD_H___
#define BOARD_H___

#include <Arduino.h>
#include <Preferences.h>

#include "definitions.h"

#ifdef FEATURE_SERIAL
#include "splitstream.h"
#include "wifiserial.h"
#else
#include "nullserial.h"
#endif
#include "haspreferences.h"
#include "task.h"
#include "mpu.h"
#include "strain.h"
#include "power.h"
#include "ble.h"
#include "battery.h"
#include "wificonnection.h"
#include "ota.h"
#include "status.h"
#include "led.h"
#ifdef FEATURE_WEBSERVER
#include "webserver.h"
#endif

// The one in charge
class Board : public HasPreferences, public Task {
   public:
    Preferences boardPreferences = Preferences();
#ifdef FEATURE_SERIAL
    HardwareSerial hwSerial = HardwareSerial(0);
    WifiSerial wifiSerial;
#endif
    WifiConnection wifi;
    BLE ble;
    Battery battery;
    MPU mpu;
    Strain strain;
    Power power;
    OTA ota;
    Status status;
    Led led;
#ifdef FEATURE_WEBSERVER
    WebServer webserver;
#endif
    bool sleepEnabled = true;
    char hostName[32] = HOSTNAME;

    void setup() {
        setCpuFrequencyMhz(80);  // no wifi/bt below 80MHz
#ifdef FEATURE_SERIAL
        hwSerial.begin(115200);
#endif
        preferencesSetup(&boardPreferences, "BOARD");
        loadSettings();
        led.setup();
        wifi.setup(preferences);
#ifdef FEATURE_SERIAL
        wifiSerial.setup();
        Serial.setup(&hwSerial, &wifiSerial, true, true);
        while (!hwSerial) vTaskDelay(10);
#endif
        ble.setup(hostName, preferences);
        battery.setup(preferences);
        mpu.setup(MPU_SDA_PIN, MPU_SCL_PIN, preferences);
        strain.setup(STRAIN_DOUT_PIN, STRAIN_SCK_PIN, preferences);
        power.setup(preferences);
        ota.setup(hostName);
        status.setup();
#ifdef FEATURE_WEBSERVER
        webserver.setup();
#endif
    }

    void startTasks() {
#ifdef FEATURE_SERIAL
        wifiSerial.taskStart("WifiSerial Task", 10);
#endif
        wifi.taskStart("Wifi Task", 1);
        ble.taskStart("BLE Task", 10);
        battery.taskStart("Battery Task", 1);
        mpu.taskStart("MPU Task", 125);
        strain.taskStart("Strain Task", 90);
        power.taskStart("Power Task", 10);
        ota.taskStart("OTA Task", 10, 8192);
        status.taskStart("Status Task", 10);
        led.taskStart("Led Task", 10);
        taskStart("Board Task", 1);
#ifdef FEATURE_WEBSERVER
        webserver.taskStart("Webserver Task", 20, 8192);
#endif
    }

    void loop() {
        const ulong t = millis();
        const long tSleep = timeUntilDeepSleep(t);
        if (0 == tSleep) {
            Serial.printf("[Board] Deep sleep now ...zzzZZZ\n");
            deepSleep();
        }
        if (tSleep <= _sleepCountdownAfter && _lastSleepCountdown + _sleepCountdownEvery <= t) {
            Serial.printf("[Board] Deep sleep in %lis\n", tSleep / 1000UL);
            _lastSleepCountdown = t;
        }
    }

    bool loadSettings() {
        if (!preferencesStartLoad()) return false;
        char tmpHostName[32];
        strncpy(tmpHostName, preferences->getString("hostName", hostName).c_str(), 32);
        if (1 < strlen(tmpHostName)) {
            strncpy(hostName, tmpHostName, 32);
        }
        preferencesEnd();
        return true;
    }

    void saveSettings() {
        if (!preferencesStartSave()) return;
        preferences->putString("hostName", hostName);
        preferencesEnd();
    }

    // Returns time in ms until entering deep sleep, or -1 in case of no such plans.
    long timeUntilDeepSleep(ulong t = 0) {
#ifdef DISABLE_SLEEP
        return -1;
#endif
        if (!sleepEnabled) return -1;
        if (0 == t) t = millis();
        if (0 == mpu.lastMovement || t <= mpu.lastMovement) return -1;
        const long tSleep = mpu.lastMovement + _sleepDelay - t;
        return tSleep > 0 ? tSleep : 0;  // return 0 if it's past our bedtime
    }

    void deepSleep() {
#ifdef DISABLE_SLEEP
        return;
#endif
        if (!sleepEnabled) return;
        Serial.println("[Board] Preparing for deep sleep");
        /*
        rtc_gpio_init(MPU_WOM_INT_PIN);
        rtc_gpio_set_direction(MPU_WOM_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_hold_dis(MPU_WOM_INT_PIN);
        rtc_gpio_set_level(MPU_WOM_INT_PIN, LOW);
        rtc_gpio_pulldown_en(MPU_WOM_INT_PIN);
        rtc_gpio_hold_en(MPU_WOM_INT_PIN);
        */
        strain.sleep();
        mpu.enableWomSleep();
        //pinMode(MPU_WOM_INT_PIN, INPUT_PULLDOWN);
#ifdef FEATURE_SERIAL
        Serial.println("[Board] Entering deep sleep");
        Serial.flush();
        delay(1000);
        wifiSerial.disconnect();
#endif
        delay(1000);
        esp_sleep_enable_ext0_wakeup(MPU_WOM_INT_PIN, HIGH);
        esp_deep_sleep_start();
    }

    float getRpm(bool unsetDataReadyFlag = false) { return mpu.rpm(unsetDataReadyFlag); }
    float getStrain(bool clearBuffer = false) { return strain.value(clearBuffer); }
    float getPower(bool clearBuffer = false) { return power.power(clearBuffer); }
    void setSleepDelay(const ulong delay) { _sleepDelay = delay; }

   private:
    ulong _sleepDelay = 5 * 60 * 1000;             // 5m
    const ulong _sleepCountdownAfter = 30 * 1000;  // 30s
    const ulong _sleepCountdownEvery = 2000;       // 2s
    ulong _lastSleepCountdown = 0;
};

extern Board board;

#endif