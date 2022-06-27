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

#include "motion.h"
#include "strain.h"
#include "power.h"
//#include "status.h"
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

    bool sleepEnabled = true;
    ulong sleepDelay = SLEEP_DELAY_DEFAULT;
    char hostName[SETTINGS_STR_LENGTH] = HOSTNAME;
    uint8_t motionDetectionMethod = MOTION_DETECTION_METHOD;

    void setup() {
        setCpuFrequencyMhz(80);  // no wifi/bt below 80MHz
#ifdef FEATURE_SERIAL
        hwSerial.begin(115200);
        wifiSerial.setup(hostName, 0, 0, WIFISERIAL_TASK_FREQ, 2048 + 1024);
        Serial.setup(&hwSerial, &wifiSerial);
        while (!hwSerial) vTaskDelay(10);
#endif
        preferencesSetup(&arduinoPreferences, "BOARD");
        loadSettings();
        log_i("\n\n\n%s %s %s\n\n\n", hostName, __DATE__, __TIME__);
        setupTask("bleServer");
        api.setup(&api, &arduinoPreferences, "API", &bleServer, API_SERVICE_UUID);
        ota.setup(hostName, 3232, true);
        setupTask("led");
        setupTask("wifi");
        setupTask("battery");
        setupTask("motion");
        setupTask("strain");
        setupTask("power");
        // setupTask("status");

        bleServer.start();
        wifi.start();
    }

    void setupTask(const char *taskName) {
        if (strcmp("led", taskName) == 0) {
            led.setup();
            return;
        }
        if (strcmp("wifi", taskName) == 0) {
            wifi.setup(hostName, preferences, "Wifi", &wifi, &api, &ota);
            return;
        }
        if (strcmp("bleServer", taskName) == 0) {
            bleServer.setup(hostName, preferences);
            return;
        }
        if (strcmp("battery", taskName) == 0) {
            battery.setup(preferences, BATTERY_PIN, &battery, &api, &bleServer);
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
        if (strcmp("motion", taskName) == 0) {
            if (motionDetectionMethod == MDM_HALL || motionDetectionMethod == MDM_MPU)
                motion.setup(MPU_SDA_PIN, MPU_SCL_PIN, preferences);
            return;
        }
        // if (strcmp("status", taskName) == 0) {
        //     status.setup();
        //     return;
        // }
        log_e("unknown task: %s", taskName);
    }

    void startTasks() {
#ifdef FEATURE_SERIAL
        // startTask("wifiSerial");
#endif
        // wifi.taskStart("Wifi Task", 1); // Wifi task is empty
        startTask("bleServer");
        startTask("battery");
        if (motionDetectionMethod == MDM_HALL || motionDetectionMethod == MDM_MPU)
            startTask("motion");
        startTask("strain");
        startTask("power");
        // startTask("ota");
        // startTask("status");
        startTask("led");
        taskStart(BOARD_TASK_FREQ, 4096 + 1024);
    }

    void startTask(const char *taskName) {
        if (strcmp("bleServer", taskName) == 0) {
            bleServer.taskStart(BLE_SERVER_TASK_FREQ, bleServer.taskStack);
            return;
        }
        if (strcmp("battery", taskName) == 0) {
            battery.taskStart(BATTERY_TASK_FREQ);
            return;
        }
        if (strcmp("motion", taskName) == 0) {
            motion.taskStart(MOTION_TASK_FREQ);
            return;
        }
        if (strcmp("strain", taskName) == 0) {
            strain.taskStart(STRAIN_TASK_FREQ);
            return;
        }
        if (strcmp("power", taskName) == 0) {
            power.taskStart(POWER_TASK_FREQ);
            return;
        }
        // if (strcmp("status", taskName) == 0) {
        //     status.taskStart(STATUS_TASK_FREQ);
        //     return;
        // }
        if (strcmp("led", taskName) == 0) {
            led.taskStart(LED_TASK_FREQ);
            return;
        }
        log_e("unknown task: %s", taskName);
    }

    void stopTask(const char *taskName) {
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
#ifdef FEATURE_SERIAL
        while (Serial.available()) {
            int i = Serial.read();
            if (0 <= i && i < UINT8_MAX) {
                Serial.write((uint8_t)i);           // echo serial input
                api.write((const uint8_t *)&i, 1);  // feed serial input to api
            }
        }
#endif
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
        // pinMode(MPU_WOM_INT_PIN, INPUT_PULLDOWN);
        api.notifyTxChar("Sleep...");
#ifdef FEATURE_SERIAL
        Serial.println("[Board] Entering deep sleep");
        Serial.flush();
        delay(500);
        wifiSerial.disconnect();
#endif
        bleServer.stop();
        delay(500);
        esp_sleep_enable_ext0_wakeup(MPU_WOM_INT_PIN, HIGH);
        esp_deep_sleep_start();
        return 0;
    }

    void reboot() {
        api.notifyTxChar("Rebooting...");
        delay(500);
        bleServer.stop();
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