#ifndef API_H
#define API_H

#include <Arduino.h>

#define API_COMMAND_MAXLENGTH 32
#define API_ARG_MAXLENGTH 32
#define API_REPLY_MAXLENGTH 64

class API {
   public:
    enum Command {
        invalid,
        wifi,                  // enable wifi
        hostName,              // set host name
        reboot,                // reboot
        passkey,               // secure api passkey
        secureApi,             // enable secure api
        weightService,         // enable weight scale service
        calibrateStrain,       // calibrate strain measurement to known weight
        tare,                  // tare weight measurement
        wifiApEnabled,         // enable wifi ap
        wifiApSSID,            // set wifi ap ssid
        wifiApPassword,        // set wifi ap password
        wifiStaEnabled,        // enable wifi sta
        wifiStaSSID,           // set wifi sta ssid
        wifiStaPassword,       // set wifi sta password
        crankLength,           // set crank length
        reverseStrain,         // reverse strain measurement
        doublePower,           // double power readings
        sleepDelay,            // sleep delay
        hallChar,              // enable hall characteristic updates
        hallOffset,            // hall sensor reading offset
        hallThreshold,         // hall sensor high threshold
        hallThresLow,          // hall sensor low threshold
        strainThreshold,       // strain motion detection high threshold
        strainThresLow,        // strain motion detection low threshold
        motionDetectionMethod  // select motion detection method
        // sleep
    };

    enum Result {
        success,
        error,
        unknownCommand,
        commandTooLong,
        argTooLong,
        stringInvalid,
        passkeyInvalid,
        secureApiInvalid,
        calibrationFailed,
        tareFailed,
        argInvalid
    };

    const char *tag = "[API]";

    Result handleCommand(const char *commandWithArg, char *reply);
    Result commandWifi(const char *str, char *reply);
    Result commandHostName(const char *str, char *reply);
    Result commandReboot(const char *str, char *reply);
    Result commandPasskey(const char *str, char *reply);
    Result commandSecureApi(const char *str, char *reply);
    Result commandWeightService(const char *str, char *reply);
    Result commandCalibrateStrain(const char *str, char *reply);
    Result commandTare(const char *str, char *reply);
    Result commandWifiApEnabled(const char *str, char *reply);
    Result commandWifiApSSID(const char *str, char *reply);
    Result commandWifiApPassword(const char *str, char *reply);
    Result commandWifiStaEnabled(const char *str, char *reply);
    Result commandWifiStaSSID(const char *str, char *reply);
    Result commandWifiStaPassword(const char *str, char *reply);
    Result commandCrankLength(const char *str, char *reply);
    Result commandReverseStrain(const char *str, char *reply);
    Result commandDoublePower(const char *str, char *reply);
    Result commandSleepDelay(const char *str, char *reply);
    Result commandHallChar(const char *str, char *reply);
    Result commandHallOffset(const char *str, char *reply);
    Result commandHallThreshold(const char *str, char *reply);
    Result commandHallThresLow(const char *str, char *reply);
    Result commandStrainThreshold(const char *str, char *reply);
    Result commandStrainThresLow(const char *str, char *reply);
    Result commandMotionDetectionMethod(const char *str, char *reply);

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
            case stringInvalid:
                return "Invalid string";
            case passkeyInvalid:
                return "Invalid passkey";
            case secureApiInvalid:
                return "Invalid secureApi argument";
            case calibrationFailed:
                return "Calibration failed";
            case tareFailed:
                return "Tare failed";
            case argInvalid:
                return "Invalid argument";
        }
        return "Unknown error";
    }

    const char *commandCodeToStr(Command c) {
        switch (c) {
            case invalid:
                return "invalid";
            case wifi:
                return "wifi";
            case hostName:
                return "hostName";
            case reboot:
                return "reboot";
            case passkey:
                return "passkey";
            case secureApi:
                return "secureApi";
            case weightService:
                return "weightService";
            case calibrateStrain:
                return "calibrateStrain";
            case tare:
                return "tare";
            case wifiApEnabled:
                return "wifiApEnabled";
            case wifiApSSID:
                return "wifiApSSID";
            case wifiApPassword:
                return "wifiApPassword";
            case wifiStaEnabled:
                return "wifiStaEnabled";
            case wifiStaSSID:
                return "wifiStaSSID";
            case wifiStaPassword:
                return "wifiStaPassword";
            case crankLength:
                return "crankLength";
            case reverseStrain:
                return "reverseStrain";
            case doublePower:
                return "doublePower";
            case sleepDelay:
                return "sleepDelay";
            case hallChar:
                return "hallChar";
            case hallOffset:
                return "hallOffset";
            case hallThreshold:
                return "hallThreshold";
            case hallThresLow:
                return "hallThresLow";
            case strainThreshold:
                return "strainThreshold";
            case strainThresLow:
                return "strainThresLow";
            case motionDetectionMethod:
                return "motionDetectionMethod";
        }
        return "unknown";
    }

    Command parseCommandStr(const char *str) {
        if (atoi(str) == Command::wifi ||
            0 == strcmp(str, commandCodeToStr(Command::wifi))) return Command::wifi;
        if (atoi(str) == Command::hostName ||
            0 == strcmp(str, commandCodeToStr(Command::hostName))) return Command::hostName;
        if (atoi(str) == Command::reboot ||
            0 == strcmp(str, commandCodeToStr(Command::reboot))) return Command::reboot;
        if (atoi(str) == Command::passkey ||
            0 == strcmp(str, commandCodeToStr(Command::passkey))) return Command::passkey;
        if (atoi(str) == Command::secureApi ||
            0 == strcmp(str, commandCodeToStr(Command::secureApi))) return Command::secureApi;
        if (atoi(str) == Command::weightService ||
            0 == strcmp(str, commandCodeToStr(Command::weightService))) return Command::weightService;
        if (atoi(str) == Command::calibrateStrain ||
            0 == strcmp(str, commandCodeToStr(Command::calibrateStrain))) return Command::calibrateStrain;
        if (atoi(str) == Command::tare ||
            0 == strcmp(str, commandCodeToStr(Command::tare))) return Command::tare;
        if (atoi(str) == Command::wifiApEnabled ||
            0 == strcmp(str, commandCodeToStr(Command::wifiApEnabled))) return Command::wifiApEnabled;
        if (atoi(str) == Command::wifiApSSID ||
            0 == strcmp(str, commandCodeToStr(Command::wifiApSSID))) return Command::wifiApSSID;
        if (atoi(str) == Command::wifiApPassword ||
            0 == strcmp(str, commandCodeToStr(Command::wifiApPassword))) return Command::wifiApPassword;
        if (atoi(str) == Command::wifiStaEnabled ||
            0 == strcmp(str, commandCodeToStr(Command::wifiStaEnabled))) return Command::wifiStaEnabled;
        if (atoi(str) == Command::wifiStaSSID ||
            0 == strcmp(str, commandCodeToStr(Command::wifiStaSSID))) return Command::wifiStaSSID;
        if (atoi(str) == Command::wifiStaPassword ||
            0 == strcmp(str, commandCodeToStr(Command::wifiStaPassword))) return Command::wifiStaPassword;
        if (atoi(str) == Command::crankLength ||
            0 == strcmp(str, commandCodeToStr(Command::crankLength))) return Command::crankLength;
        if (atoi(str) == Command::reverseStrain ||
            0 == strcmp(str, commandCodeToStr(Command::reverseStrain))) return Command::reverseStrain;
        if (atoi(str) == Command::doublePower ||
            0 == strcmp(str, commandCodeToStr(Command::doublePower))) return Command::doublePower;
        if (atoi(str) == Command::sleepDelay ||
            0 == strcmp(str, commandCodeToStr(Command::sleepDelay))) return Command::sleepDelay;
        if (atoi(str) == Command::hallChar ||
            0 == strcmp(str, commandCodeToStr(Command::hallChar))) return Command::hallChar;
        if (atoi(str) == Command::hallOffset ||
            0 == strcmp(str, commandCodeToStr(Command::hallOffset))) return Command::hallOffset;
        if (atoi(str) == Command::hallThreshold ||
            0 == strcmp(str, commandCodeToStr(Command::hallThreshold))) return Command::hallThreshold;
        if (atoi(str) == Command::hallThresLow ||
            0 == strcmp(str, commandCodeToStr(Command::hallThresLow))) return Command::hallThresLow;
        if (atoi(str) == Command::strainThreshold ||
            0 == strcmp(str, commandCodeToStr(Command::strainThreshold))) return Command::strainThreshold;
        if (atoi(str) == Command::strainThresLow ||
            0 == strcmp(str, commandCodeToStr(Command::strainThresLow))) return Command::strainThresLow;
        if (atoi(str) == Command::motionDetectionMethod ||
            0 == strcmp(str, commandCodeToStr(Command::motionDetectionMethod))) return Command::motionDetectionMethod;
        return Command::invalid;
    }

    bool isAlNumStr(const char *str);
};

#endif
