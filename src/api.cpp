#include "api.h"
#include "board.h"

// Command format: commandCode|commandStr[=[arg]]; Reply format: commandCode:commandStr=[value]
API::Result API::handleCommand(const char *commandWithArg, char *reply) {
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
    //Serial.printf("%s command=%d\n", tag, (int)command);

    // by default echo back the commandCode:commandStr= so client can
    // verify that this is a response to the correct command
    snprintf(reply, API_REPLY_MAXLENGTH, "%d:%s=", (int)command,
             command == API::Command::invalid
                 ? commandStr
                 : commandCodeToStr(command));

    if (command == API::Command::invalid) {
        Serial.printf("%s %s: %s\n", tag, resultStr(Result::unknownCommand), commandStr);
        return Result::unknownCommand;
    }

    // these command processors can add their respective [value] to the reply
    if (Command::wifi == command)
        return commandWifi(argStr, reply);
    if (Command::hostName == command)
        return commandHostName(argStr, reply);
    if (Command::reboot == command)
        return commandReboot(argStr, reply);
    if (Command::passkey == command)
        return commandPasskey(argStr, reply);
    if (Command::secureApi == command)
        return commandSecureApi(argStr, reply);
    if (Command::weightService == command)
        return commandWeightService(argStr, reply);
    if (Command::calibrateStrain == command)
        return commandCalibrateStrain(argStr, reply);
    if (Command::tare == command)
        return commandTare(argStr, reply);
    if (Command::wifiApEnabled == command)
        return commandWifiApEnabled(argStr, reply);
    if (Command::wifiApSSID == command)
        return commandWifiApSSID(argStr, reply);
    if (Command::wifiApPassword == command)
        return commandWifiApPassword(argStr, reply);
    if (Command::wifiStaEnabled == command)
        return commandWifiStaEnabled(argStr, reply);
    if (Command::wifiStaSSID == command)
        return commandWifiStaSSID(argStr, reply);
    if (Command::wifiStaPassword == command)
        return commandWifiStaPassword(argStr, reply);
    return Result::unknownCommand;
}

// TODO If secureApi=true, to prevent lockout, maybe require passkey before disabling wifi?
API::Result API::commandWifi(const char *str, char *reply) {
    // set wifi
    bool newValue = true;  // enable wifi by default
    if (0 < strlen(str)) {
        if (0 == strcmp("false", str) || 0 == strcmp("0", str)) {
            newValue = false;
        }
        board.wifi.setEnabled(newValue);
    }
    //get wifi
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.wifi.isEnabled(), board.wifi.isEnabled() ? "true" : "false");
    return Result::success;
}

API::Result API::commandHostName(const char *str, char *reply) {
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
    return Result::success;
}

API::Result API::commandReboot(const char *str, char *reply) {
    uint32_t delayT = 0;
    if (0 < strlen(str)) {
        delayT = (uint32_t)atoi(str);
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    snprintf(replyTmp, API_REPLY_MAXLENGTH, "%d:%s;%s%d",
             (int)Result::success,
             resultStr(Result::success),
             reply,
             delayT);
    board.ble.setApiValue(replyTmp);
    if (delayT > 0) {
        Serial.printf("%sRebooting in %ds\n", tag, delayT);
        delay(delayT);
    }
    board.ble.setApiValue("Rebooting...");
    Serial.printf("%sRebooting...\n", tag);
    Serial.flush();
    board.reboot();
    return Result::success;
}

// TODO If bootmode=live, to prevent lockout, maybe require confirmation before setting passkey?
API::Result API::commandPasskey(const char *str, char *reply) {
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
    //itoa(board.ble.passkey, keyS, 10);
    //strncat(reply, keyS, API_REPLY_MAXLENGTH - strlen(reply));
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d", replyTmp, (int)board.ble.passkey);
    return Result::success;
}

// TODO If bootmode=live, to prevent lockout, maybe require passkey before enabling secureAPI?
API::Result API::commandSecureApi(const char *str, char *reply) {
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
    //get secureApi
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, (int)board.ble.secureApi, board.ble.secureApi ? "true" : "false");
    return Result::success;
}

API::Result API::commandWeightService(const char *str, char *reply) {
    // set value
    if (0 < strlen(str)) {
        bool newValue = false;
        if (0 == strcmp("true", str) || 0 == strcmp("1", str))
            newValue = true;
        board.ble.setWmCharUpdateEnabled(newValue);
        if (!newValue) board.ble.setWmValue(0.0);
    }
    // get value
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s", replyTmp,
             (int)board.ble.wmCharUpdateEnabled,
             board.ble.wmCharUpdateEnabled ? "true" : "false");
    return Result::success;
}

API::Result API::commandCalibrateStrain(const char *str, char *reply) {
    Result result = Result::error;
    float knownMass;
    knownMass = (float)atof(str);
    if (1 < knownMass && knownMass < 1000) {
        if (0 == board.strain.calibrateTo(knownMass)) {
            board.strain.saveCalibration();
            result = Result::success;
        }
    }
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%f", replyTmp, knownMass);
    return result;
}

API::Result API::commandTare(const char *str, char *reply) {
    board.strain.tare();
    return Result::success;
}

API::Result API::commandWifiApEnabled(const char *str, char *reply) {
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
    return Result::success;
}

API::Result API::commandWifiApSSID(const char *str, char *reply) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        if (!isAlNumStr(str)) return Result::stringInvalid;
        strncpy(board.wifi.settings.apSSID, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, board.wifi.settings.apSSID, API_REPLY_MAXLENGTH - strlen(reply));
    return Result::success;
}

API::Result API::commandWifiApPassword(const char *str, char *reply) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        strncpy(board.wifi.settings.apPassword, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, "***", API_REPLY_MAXLENGTH - strlen(reply));
    return Result::success;
}

API::Result API::commandWifiStaEnabled(const char *str, char *reply) {
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
    return Result::success;
}

API::Result API::commandWifiStaSSID(const char *str, char *reply) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        if (!isAlNumStr(str)) return Result::stringInvalid;
        strncpy(board.wifi.settings.staSSID, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, board.wifi.settings.staSSID, API_REPLY_MAXLENGTH - strlen(reply));
    return Result::success;
}

API::Result API::commandWifiStaPassword(const char *str, char *reply) {
    if (0 < strlen(str)) {
        if (SETTINGS_STR_LENGTH - 1 < strlen(str)) return Result::argTooLong;
        strncpy(board.wifi.settings.staPassword, str, SETTINGS_STR_LENGTH);
        board.wifi.saveSettings();
        board.wifi.applySettings();
    }
    strncat(reply, "***", API_REPLY_MAXLENGTH - strlen(reply));
    return Result::success;
}

bool API::isAlNumStr(const char *str) {
    int len = strlen(str);
    int cnt = 0;
    while (*str != '\0' && cnt < len) {
        //Serial.printf("%s isAlNumStr(%c): %d\n", tag, *str, isalnum(*str));
        if (!isalnum(*str)) return false;
        str++;
        cnt++;
    }
    return true;
}