#include "api.h"
#include "board.h"

// Format: command || command=arg
int API::handleCommand(const char *commandWithArg) {
    Serial.printf("%s Handling command %s\n", tag, commandWithArg);
    char command[API_COMMAND_MAXLENGTH] = "";
    char arg[API_ARG_MAXLENGTH] = "";
    char *eqSign = strstr(commandWithArg, "=");
    if (eqSign) {
        int eqPos = eqSign - commandWithArg;
        if (API_COMMAND_MAXLENGTH < eqPos) {
            Serial.printf("%s %s: %s\n", tag, API_ERROR_COMMAND_TOO_LONG_S, commandWithArg);
            return API_ERROR_COMMAND_TOO_LONG;
        }
        strncpy(command, commandWithArg, eqPos);
        int argSize = strlen(commandWithArg) - eqPos - 1;
        //Serial.printf("%s argSize=%d\n", tag, argSize);
        if (API_ARG_MAXLENGTH < argSize) {
            Serial.printf("%s %s: %s\n", tag, API_ERROR_ARG_TOO_LONG_S, commandWithArg);
            return API_ERROR_ARG_TOO_LONG;
        }
        strncpy(arg, eqSign + 1, argSize);
    } else {
        if (API_COMMAND_MAXLENGTH < strlen(commandWithArg)) {
            Serial.printf("%s %s: %s\n", tag, API_ERROR_COMMAND_TOO_LONG_S, commandWithArg);
            return API_ERROR_COMMAND_TOO_LONG;
        }
        strncpy(command, commandWithArg, sizeof command);
    }
    Serial.printf("%s command=%s arg=%s\n", tag, command, arg);
    if (0 == strcmp(command, API_COMMAND_BOOTMODE_S) || atoi(command) == API_COMMAND_BOOTMODE)
        return setBootMode(arg);
    if (0 == strcmp(command, API_COMMAND_REBOOT_S) || atoi(command) == API_COMMAND_REBOOT) {
        ESP.restart();
        return API_SUCCESS;
    }
    Serial.printf("%s %s: %s\n", tag, API_ERROR_UNKNOWN_COMMAND_S, command);
    return API_ERROR_UNKNOWN_COMMAND;
}

int API::setBootMode(const char *mode) {
    int bootMode = BOOTMODE_INVALID;
    if (0 == strcmp(mode, BOOTMODE_LIVE_S) || atoi(mode) == BOOTMODE_LIVE) {
        bootMode = BOOTMODE_LIVE;
    } else if (0 == strcmp(mode, BOOTMODE_OTA_S) || atoi(mode) == BOOTMODE_OTA) {
        bootMode = BOOTMODE_OTA;
    } else if (0 == strcmp(mode, BOOTMODE_DEBUG_S) || atoi(mode) == BOOTMODE_DEBUG) {
        bootMode = BOOTMODE_DEBUG;
    }
    if (BOOTMODE_INVALID == bootMode) {
        Serial.printf("%s %s: %s\n", tag, API_ERROR_UNKNOWN_BOOTMODE_S, mode);
        return API_ERROR_UNKNOWN_BOOTMODE;
    }
    if (board.setBootMode(bootMode))
        return API_SUCCESS;
    return API_ERROR;
}