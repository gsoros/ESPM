#include "api.h"
#include "board.h"

int API::handleCommand(const char *commandWithArgs) {
    Serial.printf("[API] Handling command %s\n", commandWithArgs);
    return API_FAILURE;
    // TODO
    int i = 0;
    while (commandWithArgs[++i]) {
        if (commandWithArgs[i] == '=')
            break;
    }
    char *command = "";
    strncpy(command, commandWithArgs, i);
    char response[32];
    snprintf(response, sizeof(response), "Response to \"%s\"", command);
    Serial.printf("[API] Sending response: %s\n", response);
    board.ble.setApiResponse(response);
    return API_FAILURE;
}

int API::setBootMode(int mode) {
    return API_FAILURE;
}