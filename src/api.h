#ifndef API_H
#define API_H

#define API_COMMAND_MAXLENGTH 32
#define API_ARG_MAXLENGTH 32

enum api_result_t {
    AR_SUCCESS,
    AR_ERROR,
    AR_UNKNOWN_COMMAND,
    AR_COMMAND_TOO_LONG,
    AR_ARG_TOO_LONG,
    AR_BOOTMODE_INVALID
};

enum api_command_t {
    AC_INVALID,
    AC_BOOTMODE,
    AC_HOSTNAME,
    AC_REBOOT
};

class API {
   public:
    const char *tag = "[API]";

    api_result_t handleCommand(const char *command);
    api_result_t setBootMode(const char *mode);

    const char *resultStr(api_result_t result) {
        switch (result) {
            case AR_SUCCESS:
                return "OK";
            case AR_ERROR:
                return "Generic error";
            case AR_UNKNOWN_COMMAND:
                return "Unknown command";
            case AR_COMMAND_TOO_LONG:
                return "Command too long";
            case AR_ARG_TOO_LONG:
                return "Argument too long";
            case AR_BOOTMODE_INVALID:
                return "Invalid bootmode";
        }
        return "Unknown error";
    }

    const char *commandStr(api_command_t command) {
        switch (command) {
            case AC_INVALID:
                return "invalid";
            case AC_BOOTMODE:
                return "bootmode";
            case AC_HOSTNAME:
                return "hostname";
            case AC_REBOOT:
                return "reboot";
        }
        return "unknown";
    }
};

#endif