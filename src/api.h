#ifndef API_H
#define API_H

#define API_COMMAND_MAXLENGTH 32
#define API_ARG_MAXLENGTH 32

#define API_SUCCESS 0
#define API_SUCCESS_S "OK"
#define API_ERROR 1
#define API_ERROR_S "Generic error"
#define API_ERROR_UNKNOWN_COMMAND 2
#define API_ERROR_UNKNOWN_COMMAND_S "Unknown command"
#define API_ERROR_COMMAND_TOO_LONG 3
#define API_ERROR_COMMAND_TOO_LONG_S "Command too long"
#define API_ERROR_ARG_TOO_LONG 4
#define API_ERROR_ARG_TOO_LONG_S "Argument too long"
#define API_ERROR_UNKNOWN_BOOTMODE 5
#define API_ERROR_UNKNOWN_BOOTMODE_S "Unknown bootmode"

#define API_COMMAND_BOOTMODE 1
#define API_COMMAND_BOOTMODE_S "bootmode"
#define API_COMMAND_HOSTNAME 2
#define API_COMMAND_HOSTNAME_S "hostname"
#define API_COMMAND_REBOOT 3
#define API_COMMAND_REBOOT_S "reboot"

class API {
   public:
    const char *tag = "[API]";

    int handleCommand(const char *command);
    int setBootMode(const char *mode);
};

#endif