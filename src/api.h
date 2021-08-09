#ifndef API_H
#define API_H

#include <Arduino.h>

#define API_COMMAND_MAXLENGTH 32
#define API_ARG_MAXLENGTH 32

class API {
   public:
    enum Result {
        success,
        error,
        unknownCommand,
        commandTooLong,
        argTooLong,
        bootModeInvalid,
        hostNameInvalid,
        passkeyInvalid,
        secureApiInvalid
    };

    enum Command {
        invalid,
        bootMode,
        hostName,
        reboot,
        passkey,
        secureApi
    };

    const char *tag = "[API]";

    Result handleCommand(const char *commandWithArg);
    Result commandBootMode(const char *modeStr);
    Result commandHostName(const char *hostNameStr);
    Result commandReboot();
    Result commandPasskey(const char *passkeyStr);
    Result commandSecureApi(const char *secureApiStr);

    const char *resultStr(Result r) {
        switch (r) {
            case success:
                return "OK";
            case error:
                return "Generic error";
            case unknownCommand:
                return "Unknown command";
            case commandTooLong:
                return "Command too long";
            case argTooLong:
                return "Argument too long";
            case bootModeInvalid:
                return "Invalid bootmode";
            case hostNameInvalid:
                return "Invalid hostname";
            case passkeyInvalid:
                return "Invalid passkey";
            case secureApiInvalid:
                return "Invalid secureApi argument";
        }
        return "Unknown error";
    }

    const char *commandStr(Command c) {
        switch (c) {
            case invalid:
                return "invalid";
            case bootMode:
                return "bootmode";
            case hostName:
                return "hostname";
            case reboot:
                return "reboot";
            case passkey:
                return "passkey";
            case secureApi:
                return "secureApi";
        }
        return "unknown";
    }

    Command parseCommandStr(const char *str) {
        if (0 == strcmp(str, commandStr(Command::bootMode))) return Command::bootMode;
        if (0 == strcmp(str, commandStr(Command::hostName))) return Command::hostName;
        if (0 == strcmp(str, commandStr(Command::reboot))) return Command::reboot;
        if (0 == strcmp(str, commandStr(Command::passkey))) return Command::passkey;
        if (0 == strcmp(str, commandStr(Command::secureApi))) return Command::secureApi;
        return Command::invalid;
    }

    bool isAlNumStr(const char *str);
};

#endif