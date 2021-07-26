#ifndef API_H
#define API_H

#define API_SUCCESS 1
#define API_FAILURE 0

#define BOOTMODE_NORMAL 0
#define BOOTMODE_OTA 1

class API {
   public:
    int handleCommand(const char *command);
    int setBootMode(int mode);
};

#endif