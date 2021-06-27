#ifndef TASK_H
#define TASK_H

#include <Arduino.h>

class Task {
   public:
    TaskHandle_t taskHandle = NULL;
    char taskName[32] = "Unnamed Task";
    uint16_t taskFreq = 10;     // desired task frequency in Hz
    uint32_t taskStack = 8192;  // task stack size in bytes
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
        taskDelay = 1000 / freq / portTICK_PERIOD_MS;
        xTaskCreate(taskLoop, taskName, stack, this, priority, &taskHandle);
    }

    bool taskRunning() {
        return NULL != taskHandle;
    }

    void taskStop() {
        if (NULL != taskHandle)
            vTaskDelete(taskHandle);
        taskHandle = NULL;
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
    uint16_t taskDelay;

    static void taskLoop(void *p) {
        Task *thisPtr = (Task *)p;
        thisPtr->xLastWakeTime = xTaskGetTickCount();
        for (;;) {
            vTaskDelayUntil(&thisPtr->xLastWakeTime, thisPtr->taskDelay);
            const ulong t = millis();
            thisPtr->loop(t);
            thisPtr->taskUpdateStats(t);
        }
    }
};

#endif
