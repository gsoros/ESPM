#include "api.h"
#include "board.h"

// Command format: commandCode|commandStr[=[arg]]; Reply format: commandCode:commandStr=[arg]
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

    // by default echo back the commandCode:commandStr=[arg] so client can
    // verify that this is a response to the correct command
    snprintf(reply, API_REPLY_MAXLENGTH, "%d:%s=", (int)command, commandCodeToStr(command));

    // these command processors can add their respective value to the reply
    if (Command::bootMode == command)
        return commandBootMode(argStr, reply);
    if (Command::hostName == command)
        return commandHostName(argStr, reply);
    if (Command::reboot == command)
        return commandReboot();
    if (Command::passkey == command)
        return commandPasskey(argStr, reply);
    if (Command::secureApi == command)
        return commandSecureApi(argStr, reply);
    Serial.printf("%s %s: %s\n", tag, resultStr(Result::unknownCommand), commandStr);
    return Result::unknownCommand;
}

// TODO If secureApi=true, to prevent lockout, maybe require passkey before setBootMode(live)?
API::Result API::commandBootMode(const char *modeStr, char *reply) {
    // set bootmode
    if (0 < strlen(modeStr)) {
        Board::BootMode modeCode = (Board::BootMode)atoi(modeStr);
        if (0 == strcmp(modeStr, board.bootModeStr(Board::BootMode::live)) ||
            modeCode == Board::BootMode::live) {
            modeCode = Board::BootMode::live;
        } else if (0 == strcmp(modeStr, board.bootModeStr(Board::BootMode::config)) ||
                   modeCode == Board::BootMode::config) {
            modeCode = Board::BootMode::config;
        } else {
            Serial.printf("%s %s: %s\n", tag, resultStr(Result::bootModeInvalid), modeStr);
            return Result::bootModeInvalid;
        }
        if (!board.setBootMode(modeCode))
            return Result::error;
    }
    //get bootmode
    char replyTmp[API_REPLY_MAXLENGTH];
    strncpy(replyTmp, reply, sizeof(replyTmp));
    snprintf(reply, API_REPLY_MAXLENGTH, "%s%d:%s",
             replyTmp, board.bootMode, board.bootModeStr(board.bootMode));
    return Result::success;
}

API::Result API::commandHostName(const char *hostNameStr, char *reply) {
    // set hostname
    if (0 < strlen(hostNameStr)) {
        int maxSize = sizeof(board.hostName);
        if (maxSize - 1 < strlen(hostNameStr)) return Result::hostNameInvalid;
        if (!isAlNumStr(hostNameStr)) return Result::hostNameInvalid;
        strncpy(board.hostName, hostNameStr, maxSize);
        board.saveSettings();
    }
    // get hostname
    strncat(reply, board.hostName, API_REPLY_MAXLENGTH - strlen(reply));
    return Result::success;
}

API::Result API::commandReboot() {
    board.reboot();
    return Result::success;
}

// TODO If bootmode=live, to prevent lockout, maybe require confirmation before setting passkey?
API::Result API::commandPasskey(const char *passkeyStr, char *reply) {
    char keyS[7] = "";
    // set passkey
    if (0 < strlen(passkeyStr)) {
        if (6 < strlen(passkeyStr)) return Result::passkeyInvalid;
        strncpy(keyS, passkeyStr, 6);
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
API::Result API::commandSecureApi(const char *secureApiStr, char *reply) {
    // set secureApi
    int newValue = -1;
    if (0 < strlen(secureApiStr)) {
        if (0 == strcmp("true", secureApiStr) || 0 == strcmp("1", secureApiStr)) {
            newValue = 1;
        } else if (0 == strcmp("false", secureApiStr) || 0 == strcmp("0", secureApiStr)) {
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

bool API::isAlNumStr(const char *str) {
    int len = strlen(str);
    int cnt = 0;
    while (*str != '\0' && cnt < len) {
        Serial.printf("%s isAlNum(%c): %d\n", tag, *str, isalnum(*str));
        if (!isalnum(*str)) return false;
        str++;
        cnt++;
    }
    return true;
}