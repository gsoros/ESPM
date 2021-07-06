#include <Arduino.h>

#include "board.h"

Board board;

void setup() {
    esp_log_level_set("*", ESP_LOG_DEBUG);
    board.setup();
    board.startTasks();
}

void loop() {
    vTaskDelete(NULL);
}
