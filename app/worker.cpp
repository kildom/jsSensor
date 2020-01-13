
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#include <functional>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "config.h"
#include "common.h"
#include "timer.h"
#include "worker.h"

namespace low {

static const uint8_t ITEM_TYPE_ARG = 0;
static const uint8_t ITEM_TYPE_CALLBACK = 1;
static const uint8_t ITEM_TYPE_STD_FUNCTION = 2;

struct ThreadData
{
    WorkerLevel level;
    WorkerCallback startup;
};

struct WorkerQueueItem
{
    uintptr_t data;
    uint8_t type;
    uint8_t _reserved[3];
};

typedef uint8_t WorkerQueueItemBuffer[5];


static StaticTask_t workerStaticTask[2];
static StackType_t workerLowStaticStack[WORKER_LOW_STACK_SIZE / sizeof(StackType_t)];
static StackType_t workerHighStaticStack[WORKER_HIGH_STACK_SIZE / sizeof(StackType_t)];

static StaticQueue_t workerStaticQueue[2];
static WorkerQueueItemBuffer workerLowStaticQueueBuffer[WORKER_LOW_QUEUE_LENGTH];
static WorkerQueueItemBuffer workerHighStaticQueueBuffer[WORKER_HIGH_QUEUE_LENGTH];

static QueueHandle_t workerQueue[2];
static TaskHandle_t workerTask[2];

static ThreadData workerThreadData[2] = { { WORKER_LEVEL_LOW }, { WORKER_LEVEL_HIGH } };


STATIC_ASSERT(offsetof(WorkerQueueItem, _reserved) == 5, "Unexpected location of members in WorkerQueueItem");
STATIC_ASSERT(sizeof(WorkerQueueItem) == 8, "Unexpected size of WorkerQueueItem");
STATIC_ASSERT(sizeof(workerLowStaticQueueBuffer) == 5 * WORKER_LOW_QUEUE_LENGTH, "Unexpected size of workerStaticQueueBuffer");
STATIC_ASSERT(sizeof(workerHighStaticQueueBuffer) == 5 * WORKER_HIGH_QUEUE_LENGTH, "Unexpected size of workerStaticQueueBuffer");


static void sendData(QueueHandle_t queue, uintptr_t data, uint8_t type)
{
    BaseType_t ret;
    WorkerQueueItem item;
    item.data = data;
    item.type = type;
    ret = xQueueSend(queue, &item, 0);
    PROD_ASSERT(ret == pdTRUE, "Work queue overflow");
}

static void workerAddToValist(WorkerLevel level, WorkerCallback callback, size_t args, va_list valist)
{
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    while (args--)
    {
        sendData(workerQueue[level], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendData(workerQueue[level], (uintptr_t)(void*)callback, ITEM_TYPE_CALLBACK);
}

void workerAdd(WorkerCallback callback, size_t args, ...)
{
    // TODOLATER: reconsider if this function is needed. How often it is used? Removing it will simplify the code.
    va_list valist;
    WorkerLevel level = workerGetLevel();
    va_start(valist, args);
    DBG_ASSERT(level != WORKER_LEVEL_UNKNOWN, "workerAdd called not from a worker context!");
    workerAddToValist(level, callback, args, valist);
}

void workerAddTo(WorkerLevel level, WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    va_start(valist, args);
    workerAddToValist(level, callback, args, valist);
}

static void sendDataFromISR(bool* yieldRequested, QueueHandle_t queue, uintptr_t data, uint8_t type)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret;
    WorkerQueueItem item;
    item.data = data;
    item.type = type;
    ret = xQueueSendFromISR(queue, &item, &woken);
    PROD_ASSERT(ret == pdTRUE, "Worker queue overflow");
    if (woken) *yieldRequested = true;
}

void workerAddToFromISR(bool* yieldRequested, WorkerLevel level, WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    va_start(valist, args);
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    while (args--)
    {
        sendDataFromISR(yieldRequested, workerQueue[level], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendDataFromISR(yieldRequested, workerQueue[level], (uintptr_t)(void*)callback, ITEM_TYPE_CALLBACK);
}


static void workerMainLoop(void *param)
{
    static uintptr_t argsBuffer[WORKER_MAX_ARGS];
    static WorkerQueueItem item;
    size_t argsBufferSize = 0;
    BaseType_t ret;
    uint32_t timeout;
    WorkerCallback callback;
    WorkerLevel level = ((ThreadData*)param)->level;

    callback = ((ThreadData*)param)->startup;
    if (callback) callback(argsBuffer);

    do {
        timeout = timerGetNextTimeout(level);
        ret = pdFALSE;
        if (timeout != 0)
        {
            ret = xQueueReceive(workerQueue[level], &item, timeout);
        }
        if (ret)
        {
            if (item.type == ITEM_TYPE_CALLBACK)
            {
                WorkerCallback callback = (WorkerCallback)(void*)item.data;
                callback(argsBuffer);
                argsBufferSize = 0;
            }
            else if (item.type == ITEM_TYPE_ARG)
            {
                DBG_ASSERT(argsBufferSize < WORKER_MAX_ARGS);
                argsBuffer[argsBufferSize++] = item.data;
            }
            else
            {
                std::function<void()>* func = (std::function<void()>*)item.data;
                (*func)();
                delete func;
            }
        }
        else
        {
            timerExecute(level);
        }
    } while (true);
}


void workerInit(WorkerCallback startups[WORKER_LEVELS_COUNT])
{
    workerThreadData[0].startup = startups[WORKER_LEVEL_LOW];
    workerThreadData[1].startup = startups[WORKER_LEVEL_HIGH];
    workerQueue[WORKER_LEVEL_LOW] = xQueueCreateStatic(ARRAY_LENGTH(workerLowStaticQueueBuffer),
        sizeof(workerLowStaticQueueBuffer[0]),
        (uint8_t*)workerLowStaticQueueBuffer, &workerStaticQueue[WORKER_LEVEL_LOW]);
    workerQueue[WORKER_LEVEL_HIGH] = xQueueCreateStatic(ARRAY_LENGTH(workerHighStaticQueueBuffer),
        sizeof(workerHighStaticQueueBuffer[0]),
        (uint8_t*)workerHighStaticQueueBuffer, &workerStaticQueue[WORKER_LEVEL_HIGH]);
    workerTask[WORKER_LEVEL_LOW] = xTaskCreateStatic(workerMainLoop, "Lo", ARRAY_LENGTH(workerLowStaticStack),
        (void*)&workerThreadData[WORKER_LEVEL_LOW],
        WORKER_LOW_PRIORITY, workerLowStaticStack, &workerStaticTask[WORKER_LEVEL_LOW]);
    workerTask[WORKER_LEVEL_HIGH] = xTaskCreateStatic(workerMainLoop, "Hi", ARRAY_LENGTH(workerHighStaticStack),
        (void*)&workerThreadData[WORKER_LEVEL_HIGH],
        WORKER_HIGH_PRIORITY, workerHighStaticStack, &workerStaticTask[WORKER_LEVEL_HIGH]);
}


bool workerOnLevel(WorkerLevel level)
{
    return xTaskGetCurrentTaskHandle() == workerTask[level];
}

WorkerLevel workerGetLevel()
{
    auto handle = xTaskGetCurrentTaskHandle();
    if (handle == workerTask[WORKER_LEVEL_LOW])
    {
        return WORKER_LEVEL_LOW;
    }
    else if (handle == workerTask[WORKER_LEVEL_HIGH])
    {
        return WORKER_LEVEL_HIGH;
    }
    else
    {
        return WORKER_LEVEL_UNKNOWN;
    }
}

}; // namespace low

using namespace low;

void workerAdd(const std::function<void()>& func)
{
    sendData(workerQueue[WORKER_LEVEL_HIGH], (uintptr_t)(void*)new std::function<void()>(func), ITEM_TYPE_STD_FUNCTION);
}
