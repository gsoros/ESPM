#include "api.h"
#include "board.h"

// Command format: commandCode|commandStr[=[arg]];
// Reply format: commandCode:commandStr=[value]
API::Result API::handleCommand(const char *commandWithArg, char *reply, char *value) {
    Serial.printf("%s Handling command %s\n", tag, commandWithArg);
    char commandStr[API_COMMAND_MAXLENGTH] = "";
    char argStr[API_ARG_MAXLENGTH] = "";
    int commandWithArgLength = strlen(commandWithArg);
    char *eqSign = strstr(commandWithArg, "=");
    int commandEnd = eqSign ? eqSign - commandWithArg : commandWithArgLength;

    if (API_COMMAND_MAXLENGTH < commandEnd) {
        Serial.printf("%s %s: %s\n", tag, resultStr(Result::commandTooLong), commandWithArg);
        return Result::commandTooLong;
    }
    strncpy(commandStr, commandWithArg, commandEnd);

    if (eqSign) {
        int argLength = commandWithArgLength - commandEnd - 1;
        Serial.printf("%s argSize=%d\n", tag, argLength);
        if (API_ARG_MAXLENGTH < argLength) {
            Serial.printf("%s %s: %s\n", tag, resultStr(Result::argTooLong), commandWithArg);
            return Result::argTooLong;
        }
        strncpy(argStr, eqSign + 1, argLength);
    }

    Serial.printf("%s commandStr=%s argStr=%s\n", tag, commandStr, argStr);

    Command command = parseCommandStr(commandStr);
    // Serial.printf("%s command=%d\n", tag, (int)command);

    // by default echo back the commandCode:commandStr= so clients can
    // verify that this is a response to the correct command
    snprintf(reply, API_REPLY_MAXLENGTH, "%d:%s=", (int)command,
             command == API::Command::invalid
                 ? commandStr
                 : commandCodeToStr(command));

    if (command == API::Command::invalid) {
        Serial.printf("%s %s: %s\n", tag, resultStr(Result::unknownCommand), commandStr);
        return Result::unknownCommand;
    }

    // these command processors can add their respective [value] to the [reply]
    if (Command::wifi == command)
        return commandWifi(argStr, reply, value);
    if (Command::hostName == command)
        return commandHostName(argStr, reply, value);
    if (Command::reboot == command)
        return commandReboot(argStr, reply);
    if (Command::passkey == command)
        return commandPasskey(argStr, reply, value);
    if (Command::secureApi == command)
        return commandSecureApi(argStr, reply, value);
    if (Command::weightService == command)
        return commandWeightService(argStr, reply, value);
    if (Command::calibrateStrain == command)
        return commandCalibrateStrain(argStr, reply);
    if (Command::tare == command)
        return commandTare(argStr, reply);
    if (Command::wifiApEnabled == command)
        return commandWifiApEnabled(argStr, reply, value);
    if (Command::wifiApSSID == command)
        return commandWifiApSSID(argStr, reply, value);
    if (Command::wifiApPassword == command)
        return commandWifiApPassword(argStr, reply, value);
    if (Command::wifiStaEnabled == command)
        return commandWifiStaEnabled(argStr, reply, value);
    if (Command::wifiStaSSID == command)
        return commandWifiStaSSID(argStr, reply, value);
    if (Command::wifiStaPassword == command)
        return commandWifiStaPassword(argStr, reply, value);
    if (Command::crankLength == command)
        return commandCrankLength(argStr, reply, value);
    if (Command::reverseStrain == command)
        return commandReverseStrain(argStr, reply, value);
    if (Command::doublePower == command)
        return commandDoublePower(argStr, reply, value);
    if (Command::sleepDelay == command)
        return commandSleepDelay(argStr, reply, value);
    if (Command::hallChar == command)
        return commandHallChar(argStr, reply, value);
    if (Command::hallOffset == command)
        return commandHallOffset(argStr, reply, value);
    if (Command::hallThreshold == command)
        return commandHallThreshold(argStr, reply, value);
    if (Command::hallThresLow == command)
        return commandHallThresLow(argStr, reply, value);
    if (Command::strainThreshold == command)
        return commandStrainThreshold(argStr, reply, value);
    if (Command::strainThresLow == command)
        return commandStrainThresLow(argStr, reply, value);
    if (Command::motionDetectionMethod == command)
        return commandMotionDetectionMethod(argStr, reply, value);
    if (Command::sleep == command)
        return commandSleep(argStr, reply);
    if (Command::negativeTorqueMethod == command)
        return commandNegativeTorqueMethod(argStr, reply, value);
    if (Command::autoTare == command)
        return commandAutoTare(argStr, reply, value);
    if (Command::autoTareDelayMs == command)
        return commandAutoTareDelayMs(argStr, reply, value);
    if (Command::autoTareRangeG == command)
        return commandAutoTareRangeG(argStr, reply, value);
    if (Command::config == command)
        return commandConfig(argStr, reply);
    return Result::unknownCommand;
}

// TODO If secureApi=true, to prevent lockout, maybe require passkey before disabling wifi?
API::Result API::commandWifi(const char *str, char *reply, char *value) {
    // set wifi
    bool newValue = true;  // enable wifi by default
    if (0 < strlen(str)) {
        if (0 == strcmp("false", str) || 0 == strcmp("0", str)) {
            newValue = false;
        }
        board.wifi.setEnabled(newValue);
    }
    // get wifi
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.wifi.isEnabled(), board.wifi.isEnabled() ? "true" : "false");
    strncpy(value, board.wifi.isEnabled() ? "1" : "0", API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandHostName(const char *str, char *reply, char *value) {
    // set hostname
    if (0 < strlen(str)) {
        int maxSize = sizeof(board.hostName);
        if (maxSize - 1 < strlen(str)) return Result::stringInvalid;
        if (!isAlNumStr(str)) return Result::stringInvalid;
        strncpy(board.hostName, str, maxSize);
        board.saveSettings();
    }
    // get hostname
    strncat(reply, board.hostName, API_REPLY_MAXLENGTH - strlen(reply));
    strncpy(value, board.hostName, API_VALUE_MAXLENGTH);
    return Result::success;
}

API::Result API::commandReboot(const char *str, char *reply) {
    Result result = Result::argInvalid;
    uint32_t delayT = 0;
    if (0 < strlen(str)) {
        delayT = (uint32_t)atoi(str);
        result = Result::success;
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    snprintf(replyTmp, API_REPLY_MAXLENGTH, "%d:%s;%s%d",
             (int)result,
             resultStr(result),
             reply,
             delayT);
    if (Result::success == result) {
        board.ble.setApiValue(replyTmp);
        if (delayT > 0) {
            Serial.printf("%sRebooting in %ds\n", tag, delayT);
            delay(delayT);
        }
        board.ble.setApiValue("Rebooting...");
        Serial.printf("%sRebooting...\n", tag);
        Serial.flush();
        board.reboot();
    }
    // Serial.printf("[API] commandReboot repyTmp=%s reply=%s\n", replyTmp, reply);
    return result;
}

// TODO To prevent lockout, maybe require confirmation before setting passkey?
API::Result API::commandPasskey(const char *str, char *reply, char *value) {
    char keyS[7] = "";
    // set passkey
    if (0 < strlen(str)) {
        if (6 < strlen(str)) return Result::passkeyInvalid;
        strncpy(keyS, str, 6);
        uint32_t keyI = (uint32_t)atoi(keyS);
        if (999999 < keyI) return Result::passkeyInvalid;
        board.ble.setPasskey(keyI);
    }
    // get passkey
    itoa(board.ble.passkey, keyS, 10);
    strncpy(value, keyS, API_VALUE_MAXLENGTH);
    // strncat(reply, keyS, API_REPLY_MAXLENGTH - strlen(reply));
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, (int)board.ble.passkey);
    return Result::success;
}

// TODO To prevent lockout, maybe require passkey before enabling secureAPI?
API::Result API::commandSecureApi(const char *str, char *reply, char *value) {
    // set secureApi
    int newValue = -1;
    if (0 < strlen(str)) {
        if (0 == strcmp("true", str) || 0 == strcmp("1", str)) {
            newValue = 1;
        } else if (0 == strcmp("false", str) || 0 == strcmp("0", str)) {
            newValue = 0;
        }
        if (newValue == -1) {
            return Result::secureApiInvalid;
        }
        board.ble.setSecureApi((bool)newValue);
    }
    // get secureApi
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.ble.secureApi, board.ble.secureApi ? "true" : "false");
    strncpy(value, board.ble.secureApi ? "1" : "0", API_VALUE_MAXLENGTH);
    return Result::success;
}

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

bool API::isAlNumStr(const char *str) {
    int len = strlen(str);
    int cnt = 0;
    while (*str != '\0' && cnt < len) {
        // Serial.printf("%s isAlNumStr(%c): %d\n", tag, *str, isalnum(*str));
        if (!isalnum(*str)) return false;
        str++;
        cnt++;
    }
    return true;
}