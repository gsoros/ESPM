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
#include "motion.h"
#include "strain.h"
#include "power.h"
#include "ble.h"
#include "api.h"
#include "battery.h"
#include "wificonnection.h"
#include "ota.h"
#include "status.h"
#include "led.h"

// The one in charge
class Board : public HasPreferences,
              public Task {
   public:
    Preferences boardPreferences = Preferences();
#ifdef FEATURE_SERIAL
    HardwareSerial hwSerial = HardwareSerial(0);
    WifiSerial wifiSerial;
#endif
    WifiConnection wifi;
    BLE ble;
    API api;
    Battery battery;
    Motion motion;
    Strain strain;
    Power power;
    OTA ota;
    Status status;
    Led led;

    bool sleepEnabled = true;
    ulong sleepDelay = SLEEP_DELAY_DEFAULT;
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    int motionDetectionMethod = MOTION_DETECTION_METHOD;

    void setup() {
        setCpuFrequencyMhz(80);  // no wifi/bt below 80MHz
#ifdef FEATURE_SERIAL
        hwSerial.begin(115200);
#endif
        preferencesSetup(&boardPreferences, "BOARD");
        loadSettings();
        setupTask("led");
        setupTask("wifi");
#ifdef FEATURE_SERIAL
        //wifiSerial.setup(); // Wifi will setup and start WifiSerial
        setupTask("serial");
#endif
        setupTask("ble");
        setupTask("battery");
        setupTask("motion");
        setupTask("strain");
        setupTask("power");
        //ota.setup(hostName); // Wifi will setup and start OTA
        setupTask("status");
    }

    void setupTask(const char *taskName) {
        if (strcmp("led", taskName) == 0) {
            led.setup();
            return;
        }
        if (strcmp("wifi", taskName) == 0) {
            wifi.setup(preferences);
            return;
        }
        if (strcmp("serial", taskName) == 0) {
            Serial.setup(&hwSerial, &wifiSerial, true, true);
            while (!hwSerial) vTaskDelay(10);
            return;
        }
        if (strcmp("ble", taskName) == 0) {
            ble.setup(hostName, preferences);
            return;
        }
        if (strcmp("battery", taskName) == 0) {
            battery.setup(preferences);
            return;
        }
        if (strcmp("strain", taskName) == 0) {
            strain.setup(STRAIN_DOUT_PIN, STRAIN_SCK_PIN, preferences);
            return;
        }
        if (strcmp("power", taskName) == 0) {
            power.setup(preferences);
            return;
        }
        if (strcmp("status", taskName) == 0) {
            status.setup();
            return;
        }
        log_e("unknown task: %s", taskName);
    }

    void startTasks() {
#ifdef FEATURE_SERIAL
        //startTask("wifiSerial");
#endif
        // wifi.taskStart("Wifi Task", 1); // Wifi task is empty
        startTask("ble");
        startTask("battery");
        if (motionDetectionMethod == MDM_HALL || motionDetectionMethod == MDM_MPU)
            startTask("motion");
        startTask("strain");
        startTask("power");
        //startTask("ota");
        startTask("status");
        startTask("led");
        taskStart("Board Task", 1);
    }

    void startTask(const char *taskName) {
        if (strcmp("wifiSerial", taskName) == 0) {
            if (WiFi.getMode() == wifi_mode_t::WIFI_MODE_NULL) {
                Serial.printf("[Board] Wifi disabled, not starting WifiSerial task\n");
                return;
            }
            wifiSerial.taskStart("WifiSerial Task", 10);
            return;
        }
        if (strcmp("ble", taskName) == 0) {
            ble.taskStart("BLE Task", 10);
            return;
        }
        if (strcmp("battery", taskName) == 0) {
            battery.taskStart("Battery Task", 1);
            return;
        }
        if (strcmp("motion", taskName) == 0) {
            motion.taskStart("Motion Task", 125);
            return;
        }
        if (strcmp("strain", taskName) == 0) {
            strain.taskStart("Strain Task", 90);
            return;
        }
        if (strcmp("power", taskName) == 0) {
            power.taskStart("Power Task", 10);
            return;
        }
        if (strcmp("ota", taskName) == 0) {
            if (WiFi.getMode() == WIFI_MODE_NULL) {
                Serial.printf("[Board] Wifi disabled, not starting OTA task\n");
                return;
            }
            ota.taskStart("OTA Task", 10, 8192);
            return;
        }
        if (strcmp("status", taskName) == 0) {
            status.taskStart("Status Task", 10);
            return;
        }
        if (strcmp("led", taskName) == 0) {
            led.taskStart("Led Task", 10);
            return;
        }
        log_e("unknown task: %s", taskName);
    }

    void stopTask(const char *taskName) {
        if (strcmp("ota", taskName) == 0) {
            ota.off();
            return;
        }
        if (strcmp("wifiSerial", taskName) == 0) {
            wifiSerial.off();
            return;
        }
        if (strcmp("motion", taskName) == 0) {
            motion.taskStop();
            return;
        }
        log_e("unknown task: %s", taskName);
    }

    void restartTask(const char *taskName) {
        Serial.printf("[Board] Restarting task %s\n", taskName);
        stopTask(taskName);
        startTask(taskName);
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
        char tmpHostName[SETTINGS_STR_LENGTH];
        strncpy(tmpHostName, preferences->getString("hostName", hostName).c_str(), 32);
        if (1 < strlen(tmpHostName)) {
            strncpy(hostName, tmpHostName, 32);
        }
        sleepDelay = preferences->getULong("sleepDelay", sleepDelay);
        motionDetectionMethod = preferences->getInt("mdm", motionDetectionMethod);
        preferencesEnd();
        return true;
    }

    void saveSettings() {
        if (!preferencesStartSave()) return;
        preferences->putString("hostName", hostName);
        preferences->putULong("sleepDelay", sleepDelay);
        preferences->putInt("mdm", motionDetectionMethod);
        preferencesEnd();
    }

    // Returns time in ms until entering deep sleep, or -1 in case of no such plans.
    long timeUntilDeepSleep(ulong t = 0) {
#ifdef DISABLE_SLEEP
        return -1;
#endif
        if (!sleepEnabled) return -1;
        if (0 == sleepDelay) return -1;
        if (0 == t) t = millis();
        // if (0 == motion.lastMovement) return -1;
        if (t <= motion.lastMovement) return -1;
        const long tSleep = motion.lastMovement + sleepDelay - t;
        return tSleep > 0 ? tSleep : 0;  // return 0 if it's past our bedtime
    }

    int deepSleep() {
#ifdef DISABLE_SLEEP
        return 1;
#endif
        if (!sleepEnabled) return 1;
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
        motion.enableWomSleep();
        //pinMode(MPU_WOM_INT_PIN, INPUT_PULLDOWN);
        ble.setApiValue("Sleep...");
#ifdef FEATURE_SERIAL
        Serial.println("[Board] Entering deep sleep");
        Serial.flush();
        delay(500);
        wifiSerial.disconnect();
#endif
        ble.stop();
        delay(500);
        esp_sleep_enable_ext0_wakeup(MPU_WOM_INT_PIN, HIGH);
        esp_deep_sleep_start();
        return 0;
    }

    void reboot() {
        ble.setApiValue("Rebooting...");
        delay(500);
        ble.stop();
        delay(500);
        ESP.restart();
    }

    float getStrain(bool clearBuffer = false) { return strain.value(clearBuffer); }
    float getLiveStrain() { return strain.liveValue(); }
    float getPower(bool clearBuffer = false) { return power.power(clearBuffer); }
    void setSleepDelay(const ulong delay) {
        if (delay < SLEEP_DELAY_MIN) {
            Serial.printf("[Board] sleep delay too short");
            return;
        }
        sleepDelay = delay;
    }

    void setMotionDetectionMethod(int method) {
        int prevMDM = motionDetectionMethod;
        motionDetectionMethod = method;
        if ((prevMDM == MDM_HALL || prevMDM == MDM_MPU) && (method != MDM_HALL && method != MDM_MPU)) {
            stopTask("motion");
        } else if ((prevMDM != MDM_HALL && prevMDM != MDM_MPU) && (method == MDM_HALL || method == MDM_MPU)) {
            setupTask("motion");
            startTask("motion");
        }
    }

   private:
    const ulong _sleepCountdownAfter = SLEEP_COUNTDOWN_AFTER;
    const ulong _sleepCountdownEvery = SLEEP_COUNTDOWN_EVERY;
    ulong _lastSleepCountdown = 0;
};

extern Board board;

#endif