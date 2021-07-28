#ifndef API_H
#define API_H

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
        bootModeInvalid
    };

    enum Command {
        invalid,
        bootMode,
        hostName,
        reboot
    };

    const char *tag = "[API]";

    Result handleCommand(const char *commandWithArg);
    Result commandBootMode(const char *modeStr);
    Result commandReboot();

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
        }
        return "unknown";
    }
};

#endif