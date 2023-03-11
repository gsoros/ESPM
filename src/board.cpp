#include "board.h"

void Board::setup() {
    setCpuFrequencyMhz(80);  // no wifi/bt below 80MHz

    esp_log_level_set("*", ESP_LOG_ERROR);
    Atoll::Log::setLevel(ESP_LOG_DEBUG);

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
    setupTask("api");
    setupTask("ota");
    setupTask("led");
    setupTask("wifi");
    setupTask("battery");
    setupTask("motion");
    setupTask("strain");
    setupTask("power");
    // setupTask("status");
    setupTask("temperature");
    setupTask("tc");

    bleServer.start();
    wifi.start();
}

void Board::setupTask(const char *taskName) {
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
    if (strcmp("api", taskName) == 0) {
        api.setup(&api, &arduinoPreferences, "API", API_SERVICE_UUID);
        return;
    }
    if (strcmp("ota", taskName) == 0) {
        ota.setup(hostName, 3232, true);
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
    if (strcmp("temperature", taskName) == 0) {
#ifdef FEATURE_TEMPERATURE
        temperature.setup(
#ifdef FEATURE_TEMPERATURE_COMPENSATION
            &tc
#endif  // FEATURE_TEMPERATURE_COMPENSATION
        );
#else
        log_d("no temperature sensor support");
#endif  // FEATURE_TEMPERATURE
        return;
    }
    if (strcmp("tc", taskName) == 0) {
#ifdef FEATURE_TEMPERATURE_COMPENSATION
        tc.setup(preferences);
#else
        log_d("no temperature compensation support");
#endif  // FEATURE_TEMPERATURE_COMPENSATION
        return;
    }
    log_e("unknown task: %s", taskName);
}

void Board::startTasks() {
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
    startTask("temperature");
    taskStart(BOARD_TASK_FREQ, 4096 + 1024);
}

void Board::startTask(const char *taskName) {
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
    if (strcmp("temperature", taskName) == 0) {
#ifdef FEATURE_TEMPERATURE
        temperature.begin();
#endif
        return;
    }
    log_e("unknown task: %s", taskName);
}

void Board::stopTask(const char *taskName) {
    if (strcmp("motion", taskName) == 0) {
        motion.taskStop();
        return;
    }
    log_e("unknown task: %s", taskName);
}

void Board::restartTask(const char *taskName) {
    log_i("restarting task %s", taskName);
    stopTask(taskName);
    startTask(taskName);
}

void Board::loop() {
    const ulong t = millis();
    const long tSleep = timeUntilDeepSleep(t);
    if (0 == tSleep) {
        log_i("Deep sleep now ...zzzZZZ");
        deepSleep();
    }
    if (tSleep <= _sleepCountdownAfter && _lastSleepCountdown + _sleepCountdownEvery <= t) {
        log_i("Deep sleep in %lis", tSleep / 1000UL);
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

bool Board::loadSettings() {
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

void Board::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putString("hostName", hostName);
    preferences->putULong("sleepDelay", sleepDelay);
    preferences->putInt("mdm", motionDetectionMethod);
    preferencesEnd();
}

long Board::timeUntilDeepSleep(ulong t) {
#ifdef DISABLE_SLEEP
    return -1;
#endif
    if (!sleepEnabled) return -1;
    if (0 == sleepDelay) return -1;
    if (otaMode) return -1;
    if (0 == t) t = millis();
    if (battery.isCharging()) {
        static ulong lastLog = 0;
        if (!lastLog || lastLog < t - 5 * 60 * 1000) {
            log_i("not sleeping while charging");
            lastLog = t;
        }
        return -1;
    }
    // if (0 == motion.lastMovement) return -1;
    if (t <= motion.lastMovement) return -1;
    const long tSleep = motion.lastMovement + sleepDelay - t;
    return tSleep > 0 ? tSleep : 0;  // return 0 if it's past our bedtime
}

int Board::deepSleep() {
#ifdef DISABLE_SLEEP
    return 1;
#endif
    if (!sleepEnabled) return 1;
    log_i("Preparing for deep sleep");
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
    char msg[32];
    snprintf(msg, sizeof(msg), "%d;%d=sleep", api.success()->code, api.command("system")->code);
    api.notifyTxChar(msg);
#ifdef FEATURE_SERIAL
    log_i("Entering deep sleep");
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

void Board::reboot() {
    api.notifyTxChar("Rebooting...");
    delay(500);
    bleServer.stop();
    delay(500);
    ESP.restart();
}

float Board::getStrain(bool clearBuffer) { return strain.value(clearBuffer); }

float Board::getLiveStrain() { return strain.liveValue(); }

float Board::getPower(bool clearBuffer) { return power.power(clearBuffer); }

void Board::setSleepDelay(const ulong delay) {
    if (delay < SLEEP_DELAY_MIN) {
        log_e("sleep delay too short");
        return;
    }
    sleepDelay = delay;
}

void Board::setMotionDetectionMethod(int method) {
    int prevMDM = motionDetectionMethod;
    motionDetectionMethod = method;
    if ((prevMDM == MDM_HALL || prevMDM == MDM_MPU) && (method != MDM_HALL && method != MDM_MPU)) {
        stopTask("motion");
    } else if ((prevMDM != MDM_HALL && prevMDM != MDM_MPU) && (method == MDM_HALL || method == MDM_MPU)) {
        setupTask("motion");
        startTask("motion");
    }
}
