
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


#define ITEM_TYPE_ARG 0
#define ITEM_TYPE_CALLBACK 1
#define ITEM_TYPE_FUNC 2


struct ThreadData
{
    WorkerLevel worker;
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

static ThreadData workerThreadData[2] = { { WORKER_LOW }, { WORKER_HIGH } };


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


void workerLow(WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    va_start(valist, args);
    while (args--)
    {
        sendData(workerQueue[WORKER_LOW], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendData(workerQueue[WORKER_LOW], (uintptr_t)(void*)callback, ITEM_TYPE_CALLBACK);
}


void workerHigh(WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    va_start(valist, args);
    while (args--)
    {
        sendData(workerQueue[WORKER_HIGH], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendData(workerQueue[WORKER_HIGH], (uintptr_t)(void*)callback, ITEM_TYPE_CALLBACK);
}


void workerHighEx(const std::function<void()>& func)
{
    sendData(workerQueue[WORKER_HIGH], (uintptr_t)(void*)new std::function<void()>(func), ITEM_TYPE_FUNC);
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


void workerLowFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    va_start(valist, args);
    while (args--)
    {
        sendDataFromISR(yieldRequested, workerQueue[WORKER_LOW], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendDataFromISR(yieldRequested, workerQueue[WORKER_LOW], (uintptr_t)(void*)callback, ITEM_TYPE_CALLBACK);
}


void workerHighFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    va_start(valist, args);
    while (args--)
    {
        sendDataFromISR(yieldRequested, workerQueue[WORKER_HIGH], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendDataFromISR(yieldRequested, workerQueue[WORKER_HIGH], (uintptr_t)(void*)callback, ITEM_TYPE_CALLBACK);
}



static void workerMainLoop(void *param)
{
    static uintptr_t argsBuffer[WORKER_MAX_ARGS];
    static WorkerQueueItem item;
    size_t argsBufferSize = 0;
    BaseType_t ret;
    uint32_t timeout;
    WorkerCallback callback;
    WorkerLevel worker = ((ThreadData*)param)->worker;

    callback = ((ThreadData*)param)->startup;
    callback(argsBuffer);

    do {
        timeout = timerGetNextTimeout(worker);
        ret = pdFALSE;
        if (timeout != 0)
        {
            ret = xQueueReceive(workerQueue[worker], &item, timeout);
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
            timerExecute(worker);
        }
    } while (true);
}


void workerInit(WorkerCallback lowStartup, WorkerCallback highStartup)
{
    workerThreadData[0].startup = lowStartup;
    workerThreadData[1].startup = highStartup;
    workerQueue[WORKER_LOW] = xQueueCreateStatic(ARRAY_LENGTH(workerLowStaticQueueBuffer),
        sizeof(workerLowStaticQueueBuffer[0]),
        (uint8_t*)workerLowStaticQueueBuffer, &workerStaticQueue[WORKER_LOW]);
    workerQueue[WORKER_HIGH] = xQueueCreateStatic(ARRAY_LENGTH(workerHighStaticQueueBuffer),
        sizeof(workerHighStaticQueueBuffer[0]),
        (uint8_t*)workerHighStaticQueueBuffer, &workerStaticQueue[WORKER_HIGH]);
    workerTask[WORKER_LOW] = xTaskCreateStatic(workerMainLoop, "Lo", ARRAY_LENGTH(workerLowStaticStack),
        (void*)&workerThreadData[WORKER_LOW],
        WORKER_LOW_PRIORITY, workerLowStaticStack, &workerStaticTask[WORKER_LOW]);
    workerTask[WORKER_HIGH] = xTaskCreateStatic(workerMainLoop, "Hi", ARRAY_LENGTH(workerHighStaticStack),
        (void*)&workerThreadData[WORKER_HIGH],
        WORKER_HIGH_PRIORITY, workerHighStaticStack, &workerStaticTask[WORKER_HIGH]);
}


bool workerInThread(WorkerLevel level)
{
    return xTaskGetCurrentTaskHandle() == workerTask[level];
}

