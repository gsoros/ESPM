#ifndef TASK_H
#define TASK_H

#include <Arduino.h>
#include "splitstream.h"

class Task {
   public:
    TaskHandle_t taskHandle = NULL;
    char taskName[32];
    uint16_t taskFreq = 10;     // desired task frequency in Hz
    uint32_t taskStack = 4096;  // task stack size in bytes
    uint8_t taskPriority = 1;
    ulong taskLastLoop = 0;
    ulong taskLastLoopDelay = 0;

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
        taskSetDelayFromFreq();
        //xTaskCreate(taskLoop, taskName, stack, this, priority, &taskHandle);
        BaseType_t err = xTaskCreatePinnedToCore(taskLoop, taskName, stack, this, priority, &taskHandle, 1);
        if (pdPASS != err)
            log_e("Failed to start task %s, error %d", taskName, err);
    }

    bool taskRunning() {
        return NULL != taskHandle;
    }

    void taskStop() {
        if (NULL != taskHandle)
            vTaskDelete(taskHandle);
        taskHandle = NULL;
    }

    void taskSetFreq(const uint16_t freq) {
        taskFreq = freq;
        taskSetDelayFromFreq();
    }

    void taskUpdateStats(const ulong t) {
        taskLastLoopDelay = t - taskLastLoop;
        taskLastLoop = t;
    }

    int taskGetLowestStackLevel() {
        return NULL == taskHandle ? -1 : (int)uxTaskGetStackHighWaterMark(taskHandle);
    }

    virtual void loop(const ulong t){};
    Task() {}   // undefined reference to 'vtable...
    ~Task() {}  // undefined reference to 'vtable...

   private:
    TickType_t xLastWakeTime;
    TickType_t taskDelay;

    static void taskLoop(void *p) {
        Task *thisPtr = (Task *)p;
        thisPtr->xLastWakeTime = xTaskGetTickCount();
        for (;;) {
            const ulong t = millis();
            if (thisPtr->xLastWakeTime + thisPtr->taskDelay <= t) {
                //log_w("%s (%dHz) missed a beat", thisPtr->taskName, thisPtr->taskFreq);
                thisPtr->xLastWakeTime = t;
            }
            vTaskDelayUntil(&(thisPtr->xLastWakeTime), thisPtr->taskDelay);
            thisPtr->loop(t);
            thisPtr->taskUpdateStats(t);
        }
    }

    void taskSetDelayFromFreq() {
        taskDelay = 1000 / taskFreq / portTICK_PERIOD_MS;
    }
};

#endif
