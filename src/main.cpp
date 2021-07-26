#include <Arduino.h>

#include "board.h"

Board board;

void setup() {
#ifdef CORE_DEBUG_LEVEL
    esp_log_level_set("*", (esp_log_level_t)CORE_DEBUG_LEVEL);
#else
    esp_log_level_set("*", ESP_LOG_ERROR);
#endif
    board.setup();
    board.startTasks();
    board.resetBootMode();
}

void loop() {
    vTaskDelete(NULL);
}
