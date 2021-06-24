#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#define TASKS_MAX 16

class Task
{
public:
    TaskHandle_t *taskHandle = nullptr;
    uint16_t taskFreq = 10; // Hz
    uint16_t taskDelay;
    uint32_t taskStack = 1024;
    uint8_t taskPriority = 1;

    void taskStart(const char *taskName)
    {
        taskStart(taskName, taskFreq, taskStack, taskPriority);
    }
    void taskStart(const char *taskName, uint16_t freq, uint32_t stack, uint8_t priority)
    {
        taskFreq = freq;
        taskDelay = 10000 / freq / portTICK_PERIOD_MS;
        xTaskCreate(taskLoop, taskName, stack, this, priority, taskHandle);
    }

    static void taskLoop(void *parameter)
    {
        Task *thisPtr = (Task *)parameter;
        for (;;)
        {
            thisPtr->loop();
            vTaskDelay(thisPtr->taskDelay);
        }
    }

    void taskStop()
    {
        if (taskHandle != nullptr)
            vTaskDelete(taskHandle);
    }

    virtual void loop();
};

/*
class Scheduler {
    public:
    Task *tasks[TASKS_MAX];
    uint8_t taskCount = 0;

    void addTask(Task *t) {
        tasks[taskCount] = t;
        taskCount++;
    }

    bool taskEnabled() {
        return 
    }
};
*/

#endif
