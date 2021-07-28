#include "api.h"
#include "board.h"

// Format: command || command=arg
API::Result API::handleCommand(const char *commandWithArg) {
    Serial.printf("%s Handling command %s\n", tag, commandWithArg);
    char command[API_COMMAND_MAXLENGTH] = "";
    char arg[API_ARG_MAXLENGTH] = "";
    char *eqSign = strstr(commandWithArg, "=");
    if (eqSign) {
        int eqPos = eqSign - commandWithArg;
        if (API_COMMAND_MAXLENGTH < eqPos) {
            Serial.printf("%s %s: %s\n", tag, resultStr(Result::commandTooLong), commandWithArg);
            return Result::commandTooLong;
        }
        strncpy(command, commandWithArg, eqPos);
        int argSize = strlen(commandWithArg) - eqPos - 1;
        //Serial.printf("%s argSize=%d\n", tag, argSize);
        if (API_ARG_MAXLENGTH < argSize) {
            Serial.printf("%s %s: %s\n", tag, resultStr(Result::argTooLong), commandWithArg);
            return Result::argTooLong;
        }
        strncpy(arg, eqSign + 1, argSize);
    } else {
        if (API_COMMAND_MAXLENGTH < strlen(commandWithArg)) {
            Serial.printf("%s %s: %s\n", tag, resultStr(Result::commandTooLong), commandWithArg);
            return Result::commandTooLong;
        }
        strncpy(command, commandWithArg, sizeof command);
    }
    Serial.printf("%s command=%s arg=%s\n", tag, command, arg);
    Command commandCode = (Command)atoi(command);
    if (0 == strcmp(command, commandStr(Command::bootMode)) || commandCode == Command::bootMode)
        return commandBootMode(arg);
    if (0 == strcmp(command, commandStr(Command::reboot)) || commandCode == Command::reboot)
        return commandReboot();
    Serial.printf("%s %s: %s\n", tag, resultStr(Result::unknownCommand), command);
    return Result::unknownCommand;
}

API::Result API::commandBootMode(const char *modeStr) {
    Board::BootMode modeCode = (Board::BootMode)atoi(modeStr);
    if (0 == strcmp(modeStr, board.bootModeStr(Board::BootMode::live)) || modeCode == Board::BootMode::live) {
        modeCode = Board::BootMode::live;
    } else if (0 == strcmp(modeStr, board.bootModeStr(Board::BootMode::config)) || modeCode == Board::BootMode::config) {
        modeCode = Board::BootMode::config;
    } else {
        Serial.printf("%s %s: %s\n", tag, resultStr(Result::bootModeInvalid), modeStr);
        return Result::bootModeInvalid;
    }
    if (board.setBootMode(modeCode))
        return Result::success;
    return Result::error;
}

API::Result API::commandReboot() {
    board.reboot();
    return Result::success;
}
