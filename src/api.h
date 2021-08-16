#ifndef API_H
#define API_H

#include <Arduino.h>

#define API_COMMAND_MAXLENGTH 32
#define API_ARG_MAXLENGTH 32
#define API_REPLY_MAXLENGTH 64

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
        secureApiInvalid,
        calibrationFailed,
        tareFailed
    };

    enum Command {
        invalid,
        bootMode,
        hostName,
        reboot,
        passkey,
        secureApi,
        apiStrain,
        calibrateStrain,
        tare
    };

    const char *tag = "[API]";

    Result handleCommand(const char *commandWithArg, char *reply);
    Result commandBootMode(const char *modeStr, char *reply);
    Result commandHostName(const char *hostNameStr, char *reply);
    Result commandReboot();
    Result commandPasskey(const char *passkeyStr, char *reply);
    Result commandSecureApi(const char *secureApiStr, char *reply);
    Result commandApiStrain(const char *enabledStr, char *reply);
    Result commandCalibrateStrain(const char *knownMass, char *reply);
    Result commandTare(const char *str, char *reply);

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
            case calibrationFailed:
                return "Calibration failed";
            case tareFailed:
                return "Tare failed";
        }
        return "Unknown error";
    }

    const char *commandCodeToStr(Command c) {
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
            case apiStrain:
                return "apiStrain";
            case calibrateStrain:
                return "calibrateStrain";
            case tare:
                return "tare";
        }
        return "unknown";
    }

    Command parseCommandStr(const char *str) {
        if (atoi(str) == Command::bootMode ||
            0 == strcmp(str, commandCodeToStr(Command::bootMode))) return Command::bootMode;
        if (atoi(str) == Command::hostName ||
            0 == strcmp(str, commandCodeToStr(Command::hostName))) return Command::hostName;
        if (atoi(str) == Command::reboot ||
            0 == strcmp(str, commandCodeToStr(Command::reboot))) return Command::reboot;
        if (atoi(str) == Command::passkey ||
            0 == strcmp(str, commandCodeToStr(Command::passkey))) return Command::passkey;
        if (atoi(str) == Command::secureApi ||
            0 == strcmp(str, commandCodeToStr(Command::secureApi))) return Command::secureApi;
        if (atoi(str) == Command::apiStrain ||
            0 == strcmp(str, commandCodeToStr(Command::apiStrain))) return Command::apiStrain;
        if (atoi(str) == Command::calibrateStrain ||
            0 == strcmp(str, commandCodeToStr(Command::calibrateStrain))) return Command::calibrateStrain;
        if (atoi(str) == Command::tare ||
            0 == strcmp(str, commandCodeToStr(Command::tare))) return Command::tare;
        return Command::invalid;
    }

    bool isAlNumStr(const char *str);
};

#endif