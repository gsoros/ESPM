#include "api.h"
#include "board.h"

void Api::setup(
    Api *instance,
    ::Preferences *p,
    const char *preferencesNS,
    BleServer *bleServer,
    const char *serviceUuid) {
    Atoll::Api::setup(instance, p, preferencesNS, bleServer, serviceUuid);

    addCommand(ApiCommand("system", Api::systemProcessor));
}

void Api::beforeBleServiceStart(Atoll::BleServer *server, BLEService *service) {
    // api char for reading hall effect sensor measurements
    BleServer *s = (BleServer *)server;
    s->hallChar = service->createCharacteristic(
        BLEUUID(HALL_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY);
    s->hallChar->setCallbacks(server);
    uint8_t bytes[2];
    bytes[0] = 0 & 0xff;
    bytes[1] = (0 >> 8) & 0xff;
    s->hallChar->setValue((uint8_t *)bytes, 2);  // set initial value
    BLEDescriptor *hallDesc = s->hallChar->createDescriptor(
        BLEUUID(HALL_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char str[] = "Hall Effect Sensor reading";
    hallDesc->setValue((uint8_t *)str, strlen(str));
}

ApiResult *Api::systemProcessor(ApiMessage *msg) {
    if (msg->argStartsWith("hostname")) {
        char buf[sizeof(board.hostName)] = "";
        msg->argGetParam("hostname:", buf, sizeof(buf));
        if (0 < strlen(buf)) {
            // set hostname
            if (strlen(buf) < 2) return argInvalid();
            if (!isAlNumStr(buf)) return argInvalid();
            strncpy(board.hostName, buf, sizeof(board.hostName));
            board.saveSettings();
        }
        // get hostname
        strncpy(msg->reply, board.hostName, msgReplyLength);
        return success();
    } else if (msg->argIs("ota") || msg->argIs("OTA")) {
        log_i("entering ota mode");
        board.wifi.setEnabled(true, false);
        board.ota.taskStart(OTA_TASK_FREQ);
        msg->replyAppend("ota");
        return success();
    }
    msg->replyAppend("|", true);
    msg->replyAppend("hostname[:str]|ota");
    return Atoll::Api::systemProcessor(msg);
}

/*
API::Result API::commandWeightService(const char *str, char *reply, char *value) {
    // set value
    if (0 < strlen(str)) {
        uint8_t newValue = (uint8_t)atoi(str);
        if (0 <= newValue && newValue < WM_MAX)
            board.ble.setWmCharMode(newValue);
        if (WM_OFF == newValue) board.ble.setWmValue(0.0);
    }
    // get value
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp,
             (int)board.ble.wmCharMode);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.ble.wmCharMode);
    return Result::success;
}

API::Result API::commandCalibrateStrain(const char *str, char *reply) {
    Result result = Result::error;
    float knownMass;
    knownMass = (float)atof(str);
    if (1 < knownMass && knownMass < 1000) {
        if (0 == board.strain.calibrateTo(knownMass)) {
            board.strain.saveSettings();
            result = Result::success;
        }
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%f", replyTmp, knownMass);
    return result;
}

API::Result API::commandTare(const char *str, char *reply) {
    if (0 < strlen(str)) {
        board.strain.tare();
        return Result::success;
    }
    return Result::argInvalid;
}

API::Result API::commandWifiApEnabled(const char *str, char *reply, char *value) {
    bool newValue = true;  // enable by default
    if (0 < strlen(str)) {
        if (0 == strcmp("false", str) || 0 == strcmp("0", str)) {
            newValue = false;
        }
        board.wifi.settings.apEnabled = newValue;
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.wifi.settings.apEnabled,
             board.wifi.settings.apEnabled ? "true" : "false");
    strncpy(value, board.wifi.settings.apEnabled ? "1" : "0", API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandWifiApSSID(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        if (!isAlNumStr(str)) return Result::stringInvalid;
        strncpy(board.wifi.settings.apSSID, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, board.wifi.settings.apSSID, API_REPLY_MAXLENGTH - strlen(reply));
    strncpy(value, board.wifi.settings.apSSID, API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandWifiApPassword(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        strncpy(board.wifi.settings.apPassword, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, board.wifi.settings.apPassword, API_REPLY_MAXLENGTH - strlen(reply));
    strncpy(value, board.wifi.settings.apPassword, API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandWifiStaEnabled(const char *str, char *reply, char *value) {
    bool newValue = true;  // enable by default
    if (0 < strlen(str)) {
        if (0 == strcmp("false", str) || 0 == strcmp("0", str)) {
            newValue = false;
        }
        board.wifi.settings.staEnabled = newValue;
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.wifi.settings.staEnabled,
             board.wifi.settings.staEnabled ? "true" : "false");
    strncpy(value, board.wifi.settings.staEnabled ? "1" : "0", API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandWifiStaSSID(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        if (!isAlNumStr(str)) return Result::stringInvalid;
        strncpy(board.wifi.settings.staSSID, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, board.wifi.settings.staSSID, API_REPLY_MAXLENGTH - strlen(reply));
    strncpy(value, board.wifi.settings.staSSID, API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandWifiStaPassword(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        strncpy(board.wifi.settings.staPassword, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, board.wifi.settings.staPassword, API_REPLY_MAXLENGTH - strlen(reply));
    strncpy(value, board.wifi.settings.staPassword, API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandCrankLength(const char *str, char *reply, char *value) {
    Result result = Result::success;
    if (1 < strlen(str)) {
        result = Result::error;
        float crankLength = (float)atof(str);
        if (10 < crankLength && crankLength < 2000) {
            board.power.crankLength = crankLength;
            board.power.saveSettings();
            result = Result::success;
        }
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%f", replyTmp, board.power.crankLength);
    snprintf(value, API_VALUE_MAXLENGTH, "%f", board.power.crankLength);
    return result;
}

API::Result API::commandReverseStrain(const char *str, char *reply, char *value) {
    bool newValue = false;  // disable by default
    if (0 < strlen(str)) {
        if (0 == strcmp("true", str) || 0 == strcmp("1", str)) {
            newValue = true;
        }
        board.power.reverseStrain = newValue;
        board.power.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.power.reverseStrain,
             board.power.reverseStrain ? "true" : "false");
    strncpy(value, board.power.reverseStrain ? "1" : "0", API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandDoublePower(const char *str, char *reply, char *value) {
    bool newValue = false;  // disable by default
    if (0 < strlen(str)) {
        if (0 == strcmp("true", str) || 0 == strcmp("1", str)) {
            newValue = true;
        }
        board.power.reportDouble = newValue;
        board.power.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.power.reportDouble,
             board.power.reportDouble ? "true" : "false");
    strncpy(value, board.power.reportDouble ? "1" : "0", API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandSleepDelay(const char *str, char *reply, char *value) {
    Result result = Result::success;
    if (1 < strlen(str)) {
        result = Result::error;
        ulong sleepDelay = (ulong)atoi(str);
        if (SLEEP_DELAY_MIN < sleepDelay) {
            board.sleepDelay = sleepDelay;
            board.saveSettings();
            result = Result::success;
        }
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%ld", replyTmp, board.sleepDelay);
    snprintf(value, API_VALUE_MAXLENGTH, "%ld", board.sleepDelay);
    return result;
}

API::Result API::commandHallChar(const char *str, char *reply, char *value) {
    bool newValue = false;  // disable by default
    if (0 < strlen(str)) {
        if (0 == strcmp("true", str) || 0 == strcmp("1", str)) {
            newValue = true;
        }
        board.ble.hallCharUpdateEnabled = newValue;
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.ble.hallCharUpdateEnabled,
             board.ble.hallCharUpdateEnabled ? "true" : "false");
    strncpy(value, board.ble.hallCharUpdateEnabled ? "1" : "0", API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandHallOffset(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        board.motion.hallOffset = atoi(str);
        board.motion.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, board.motion.hallOffset);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.motion.hallOffset);
    return Result::success;
}

API::Result API::commandHallThreshold(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        board.motion.setHallThreshold(atoi(str));
        board.motion.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, board.motion.hallThreshold);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.motion.hallThreshold);
    return Result::success;
}

API::Result API::commandHallThresLow(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        board.motion.setHallThresLow(atoi(str));
        board.motion.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, board.motion.hallThresLow);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.motion.hallThresLow);
    return Result::success;
}

API::Result API::commandStrainThreshold(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        board.strain.setMdmStrainThreshold(atoi(str));
        board.strain.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, board.strain.mdmStrainThreshold);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.strain.mdmStrainThreshold);
    return Result::success;
}

API::Result API::commandStrainThresLow(const char *str, char *reply, char *value) {
    if (0 < strlen(str)) {
        board.strain.setMdmStrainThresLow(atoi(str));
        board.strain.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, board.strain.mdmStrainThresLow);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.strain.mdmStrainThresLow);
    return Result::success;
}

API::Result API::commandMotionDetectionMethod(const char *str, char *reply, char *value) {
    Result result = Result::error;
    Serial.printf("[API] commandMotionDetectionMethod(\"%s\")\n", str);
    if (0 < strlen(str)) {
        int tmpI = atoi(str);
        if (0 <= tmpI && tmpI < MDM_MAX) {
            board.setMotionDetectionMethod(tmpI);
            board.saveSettings();
            result = Result::success;
        } else
            result = Result::argInvalid;
    } else
        result = Result::success;
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, board.motionDetectionMethod);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.motionDetectionMethod);
    return result;
}

API::Result API::commandSleep(const char *str, char *reply) {
    Serial.println("[API] commandSleep()");
    Result result = Result::argInvalid;
    if (0 == strcmp("true", str) || 0 == strcmp("1", str)) {
        result = board.deepSleep() == 0 ? Result::success : Result::error;
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s", replyTmp);
    return result;
}

API::Result API::commandNegativeTorqueMethod(const char *str, char *reply, char *value) {
    Result result = Result::error;
    Serial.printf("[API] commandNegativeTorqueMethod(\"%s\")\n", str);
    if (0 < strlen(str)) {
        int tmpI = atoi(str);
        if (0 <= tmpI && tmpI < NTM_MAX) {
            board.strain.negativeTorqueMethod = tmpI;
            board.strain.saveSettings();
            result = Result::success;
        } else
            result = Result::argInvalid;
    } else
        result = Result::success;
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, board.strain.negativeTorqueMethod);
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.strain.negativeTorqueMethod);
    return result;
}

API::Result API::commandAutoTare(const char *str, char *reply, char *value) {
    Serial.printf("[API] commandAutoTare(\"%s\")\n", str);
    if (0 < strlen(str)) {
        int tmpI = atoi(str);
        if (0 == strcmp("true", str) || 1 == tmpI)
            board.strain.setAutoTare(true);
        else if (0 == strcmp("false", str) || 0 == tmpI)
            board.strain.setAutoTare(false);
        board.strain.saveSettings();
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s", replyTmp, (int)board.strain.getAutoTare(), board.strain.getAutoTare() ? "true" : "false");
    snprintf(value, API_VALUE_MAXLENGTH, "%s", board.strain.getAutoTare() ? "1" : "0");
    return Result::success;
}

API::Result API::commandAutoTareDelayMs(const char *str, char *reply, char *value) {
    Serial.printf("[API] commandAutoTareDelayMs(\"%s\")\n", str);
    if (0 < strlen(str)) {
        int tmpI = atoi(str);
        if (10 < tmpI && tmpI < 10000) {
            board.strain.setAutoTareDelayMs(tmpI);
            board.strain.saveSettings();
        }
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%lu", replyTmp, board.strain.getAutoTareDelayMs());
    snprintf(value, API_VALUE_MAXLENGTH, "%lu", board.strain.getAutoTareDelayMs());
    return Result::success;
}

API::Result API::commandAutoTareRangeG(const char *str, char *reply, char *value) {
    Serial.printf("[API] commandAutoTareRangeG(\"%s\")\n", str);
    if (0 < strlen(str)) {
        int tmpI = atoi(str);
        if (10 < tmpI && tmpI < 10000) {
            board.strain.setAutoTareRangeG(tmpI);
            board.strain.saveSettings();
        }
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, (int)board.strain.getAutoTareRangeG());
    snprintf(value, API_VALUE_MAXLENGTH, "%d", board.strain.getAutoTareRangeG());
    return Result::success;
}

API::Result API::commandConfig(const char *str, char *reply) {
    Serial.printf("[API] commandConfig(\"%s\")\n", str);
    char config[API_REPLY_MAXLENGTH] = "";
    int numSkipped = 5;
    int skippedCommands[numSkipped] = {
        Command::reboot,           // 3
        Command::calibrateStrain,  // 7
        Command::tare,             // 8
        Command::sleep,            // 26
        Command::config            // 31
    };
    char command[API_COMMAND_MAXLENGTH] = "";
    char replyTmp[API_REPLY_MAXLENGTH] = "";
    char value[API_VALUE_MAXLENGTH] = "";
    for (uint8_t commandI = 1; commandI < Command::COMMAND_MAX; commandI++) {
        bool skip = false;
        for (int i = 0; i < numSkipped; i++) {
            if (skippedCommands[i] == commandI) {
                skip = true;
                break;
            }
        }
        if (skip) continue;
        strncpy(command, commandCodeToStr((Command)commandI), sizeof(command));
        strcpy(replyTmp, "");
        strcpy(value, "");
        // Serial.printf("[API] commandConfig calling command: %s\n", command);
        // Serial.flush();
        //  delay(1000);
        handleCommand(command, replyTmp, value);
        // TODO replace ";" and "=" characters in reply?
        char configTmp[sizeof(config)];
        strncpy(configTmp, config, sizeof(configTmp));
        // Serial.printf("Adding to config: %d=%s\n", commandI, value);
        // Serial.flush();
        //  delay(1000);
        // Serial.printf("config: %s\nsizeof(config): %d\n", config, sizeof(config));
        // Serial.printf("configTmp: %s\nsizeof(configTmp): %d\n", configTmp, sizeof(configTmp));
        // Serial.printf("commandI: %d\nvalue: %s\n", commandI, value);
        // Serial.flush();
        //  delay(1000);
        // int written =
        snprintf(config, sizeof(config), "%s%d=%s;", configTmp, commandI, value);
        // Serial.printf("Adding to config done, written %d bytes.\n", written);
        // Serial.flush();
        //  delay(1000);
    }
    // strcpy(replyTmp, "");
    // strncpy(replyTmp, reply, sizeof(replyTmp));
    // snprintf(reply, API_REPLY_MAXLENGTH, "%s%s", replyTmp, config);
    strncat(reply, config, API_REPLY_MAXLENGTH - strlen(reply) - 1);
    // Serial.printf("[API] commandConfig reply: %s\n", reply);
    // Serial.flush();
    // delay(1000);
    return Result::success;
}
*/
