#include "api.h"
#include "board.h"

// Format: command || command=arg
API::Result API::handleCommand(const char *commandWithArg) {
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
    if (Command::bootMode == command)
        return commandBootMode(argStr);
    if (Command::reboot == command)
        return commandReboot();
    if (Command::passkey == command)
        return commandPasskey(argStr);
    if (Command::secureApi == command)
        return commandSecureApi(argStr);
    Serial.printf("%s %s: %s\n", tag, resultStr(Result::unknownCommand), commandStr);
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

API::Result API::commandPasskey(const char *passkeyStr) {
    if (6 < strlen(passkeyStr)) return Result::passkeyInvalid;
    char keyS[7] = "";
    strncpy(keyS, passkeyStr, 6);
    uint32_t keyI = (uint32_t)atoi(keyS);
    if (999999 < keyI) return Result::passkeyInvalid;
    Serial.printf("%s Setting new passkey: %d\n", tag, keyI);
    board.ble.setApiValue("Setting new passkey");
    board.ble.setPasskey(keyI);
    return Result::success;
}

API::Result API::commandSecureApi(const char *secureApiStr) {
    if (0 == strcmp("true", secureApiStr) || 0 == strcmp("1", secureApiStr)) {
        if (board.ble.secureApi) {
            board.ble.setApiValue("Already using secureAPI");
            return Result::success;
        }
        Serial.printf("%s Enabling secureAPI\n", tag);
        board.ble.setApiValue("Enabling secureAPI");
        board.ble.setSecureApi(true);
        return Result::success;
    }
    if (0 == strcmp("false", secureApiStr) || 0 == strcmp("0", secureApiStr)) {
        if (!board.ble.secureApi) {
            board.ble.setApiValue("SecureAPI already disabled");
            return Result::success;
        }
        Serial.printf("%s Disabling secureAPI\n", tag);
        board.ble.setApiValue("Disabling secureAPI");
        board.ble.setSecureApi(false);
        return Result::success;
    }
    return Result::secureApiInvalid;
}
