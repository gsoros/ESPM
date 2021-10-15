#ifndef TASK_H
#define TASK_H

#include "xtensa/core-macros.h"
#include <Arduino.h>
#include "splitstream.h"
#include "definitions.h"

class Task {
   public:
    TaskHandle_t taskHandle = NULL;
    char taskName[SETTINGS_STR_LENGTH];
    uint16_t taskFreq = 10;     // desired task frequency in Hz
    uint32_t taskStack = 4096;  // task stack size in bytes
    uint8_t taskPriority = 1;

    void taskStart() {
        taskStart(taskName, taskFreq, taskStack, taskPriority);
    }
    void taskStart(uint16_t freq) {
        taskStart(taskName, freq, taskStack, taskPriority);
    }
    void taskStart(const char *name) {
        taskStart(name, taskFreq, taskStack, taskPriority);
    }
    void taskStart(const char *name, uint16_t freq) {
        taskStart(name, freq, taskStack, taskPriority);
    }
    void taskStart(const char *name, uint16_t freq, uint32_t stack) {
        taskStart(name, freq, stack, taskPriority);
    }
    void taskStart(const char *name, uint16_t freq, uint32_t stack, uint8_t priority) {
        if (taskRunning()) taskStop();
        strncpy(taskName, name, 32);
        taskFreq = freq;
        _taskSetDelayFromFreq();
        Serial.printf("[Task] Starting %s at %dHz (delay: %dms)\n", name, freq, _xTaskDelay);
        //xTaskCreate(_taskLoop, taskName, stack, this, priority, &taskHandle);
        BaseType_t err = xTaskCreatePinnedToCore(_taskLoop, taskName, stack, this, priority, &taskHandle, 1);
        if (pdPASS != err)
            log_e("Failed to start task %s, error %d", taskName, err);
    }

    bool taskRunning() {
        return NULL != taskHandle;
    }

    void taskStop() {
        if (NULL != taskHandle) {
            Serial.printf("[Task] Stopping %s\n", taskName);
            vTaskDelete(taskHandle);
        }
        taskHandle = NULL;
    }

    void taskSetFreq(const uint16_t freq) {
        taskFreq = freq;
        _taskSetDelayFromFreq();
    }

    int taskGetLowestStackLevel() {
        return NULL == taskHandle ? -1 : (int)uxTaskGetStackHighWaterMark(taskHandle);
    }

    virtual void loop(){};
    Task() {}   // undefined reference to 'vtable...
    ~Task() {}  // undefined reference to 'vtable...

   private:
    TickType_t _xLastWakeTime;
    TickType_t _xTaskDelay;  // TODO unit is ticks but actually =ms??

    static void _taskLoop(void *p) {
        Task *thisPtr = (Task *)p;
        thisPtr->_xLastWakeTime = xTaskGetTickCount();
        for (;;) {
            vTaskDelayUntil(&(thisPtr->_xLastWakeTime), thisPtr->_xTaskDelay);
            thisPtr->loop();
        }
    }

    void _taskSetDelayFromFreq() {
        //_xTaskDelay = 1000 / taskFreq / portTICK_PERIOD_MS;
        _xTaskDelay = pdMS_TO_TICKS(1000 / taskFreq);
    }
};

#endif
