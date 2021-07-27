#include "api.h"
#include "board.h"

// Format: command || command=arg
api_result_t API::handleCommand(const char *commandWithArg) {
    Serial.printf("%s Handling command %s\n", tag, commandWithArg);
    char command[API_COMMAND_MAXLENGTH] = "";
    char arg[API_ARG_MAXLENGTH] = "";
    char *eqSign = strstr(commandWithArg, "=");
    if (eqSign) {
        int eqPos = eqSign - commandWithArg;
        if (API_COMMAND_MAXLENGTH < eqPos) {
            Serial.printf("%s %s: %s\n", tag, resultStr(AR_COMMAND_TOO_LONG), commandWithArg);
            return AR_COMMAND_TOO_LONG;
        }
        strncpy(command, commandWithArg, eqPos);
        int argSize = strlen(commandWithArg) - eqPos - 1;
        //Serial.printf("%s argSize=%d\n", tag, argSize);
        if (API_ARG_MAXLENGTH < argSize) {
            Serial.printf("%s %s: %s\n", tag, resultStr(AR_ARG_TOO_LONG), commandWithArg);
            return AR_ARG_TOO_LONG;
        }
        strncpy(arg, eqSign + 1, argSize);
    } else {
        if (API_COMMAND_MAXLENGTH < strlen(commandWithArg)) {
            Serial.printf("%s %s: %s\n", tag, resultStr(AR_COMMAND_TOO_LONG), commandWithArg);
            return AR_COMMAND_TOO_LONG;
        }
        strncpy(command, commandWithArg, sizeof command);
    }
    Serial.printf("%s command=%s arg=%s\n", tag, command, arg);
    api_command_t commandCode = (api_command_t)atoi(command);
    if (0 == strcmp(command, commandStr(AC_BOOTMODE)) || commandCode == AC_BOOTMODE) return setBootMode(arg);
    if (0 == strcmp(command, commandStr(AC_REBOOT)) || commandCode == AC_REBOOT) {
        board.reboot();
        return AR_SUCCESS;
    }
    Serial.printf("%s %s: %s\n", tag, resultStr(AR_UNKNOWN_COMMAND), command);
    return AR_UNKNOWN_COMMAND;
}

api_result_t API::setBootMode(const char *mode) {
    bootmode_t bootMode = (bootmode_t)atoi(mode);
    if (0 == strcmp(mode, board.bootModeStr(BOOTMODE_LIVE)) || bootMode == BOOTMODE_LIVE) {
        bootMode = BOOTMODE_LIVE;
    } else if (0 == strcmp(mode, board.bootModeStr(BOOTMODE_SETUP)) || bootMode == BOOTMODE_SETUP) {
        bootMode = BOOTMODE_SETUP;
    } else {
        Serial.printf("%s %s: %s\n", tag, resultStr(AR_BOOTMODE_INVALID), mode);
        return AR_BOOTMODE_INVALID;
    }
    if (board.setBootMode(bootMode))
        return AR_SUCCESS;
    return AR_ERROR;
}
