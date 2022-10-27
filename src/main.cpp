#include <Arduino.h>

#include "board.h"

Board board;

void setup() {
    board.setup();
    board.startTasks();
}

void loop() {
    vTaskDelete(NULL);
}
