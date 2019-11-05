
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "config.h"
#include "common.h"
#include "timer.h"
#include "worker.h"


typedef struct WorkerQueueItem_tag
{
    uintptr_t data;
    uint8_t flag;
    uint8_t _reserved[3];
} WorkerQueueItem;

typedef uint8_t WorkerQueueItemBuffer[5];


static StaticTask_t workerStaticTask;
static StackType_t workerStaticStack[WORKER_STACK_SIZE / sizeof(StackType_t)];
static StaticQueue_t workerStaticQueue;
static WorkerQueueItemBuffer workerStaticQueueBuffer[WORKER_QUEUE_LENGTH];
static QueueHandle_t workerQueue;
static TaskHandle_t workerTask;


STATIC_ASSERT(offsetof(WorkerQueueItem, _reserved) == 5, "Unexpected location of members in WorkerQueueItem");
STATIC_ASSERT(sizeof(WorkerQueueItem) == 8, "Unexpected size of WorkerQueueItem");
STATIC_ASSERT(sizeof(workerStaticQueueBuffer) == 5 * WORKER_QUEUE_LENGTH, "Unexpected size of workerStaticQueueBuffer");


static void sendData(uintptr_t data, uint8_t flag)
{
    BaseType_t ret;
    WorkerQueueItem item;
    item.data = data;
    item.flag = flag;
    ret = xQueueSend(workerQueue, &item, 0);
    PROD_ASSERT(ret == pdTRUE, "Work queue overflow");
}


void workerSend(WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    va_start(valist, args);
    while (args--)
    {
        sendData(va_arg(valist, uintptr_t), 0);
    }
    sendData((uintptr_t)(void*)callback, 1);
}


static void sendDataFromISR(bool* yieldRequested, uintptr_t data, uint8_t flag)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret;
    WorkerQueueItem item;
    item.data = data;
    item.flag = flag;
    ret = xQueueSendFromISR(workerQueue, &item, &woken);
    PROD_ASSERT(ret == pdTRUE, "Worker queue overflow");
    if (woken) *yieldRequested = true;
}


void workerSendFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    va_start(valist, args);
    while (args--)
    {
        sendDataFromISR(yieldRequested, va_arg(valist, uintptr_t), 0);
    }
    sendDataFromISR(yieldRequested, (uintptr_t)(void*)callback, 1);
}


static void workerMainLoop(void *param)
{
    static uintptr_t argsBuffer[WORKER_MAX_ARGS];
    static WorkerQueueItem item;
    size_t argsBufferSize = 0;
    BaseType_t ret;
    uint32_t timeout;
    WorkerCallback callback;

    callback = (WorkerCallback)param;
    callback(argsBuffer);

    do {
        timeout = timerGetNextTimeout();
        ret = pdFALSE;
        if (timeout != 0)
        {
            ret = xQueueReceive(workerQueue, &item, timeout);
        }
        if (ret)
        {
            if (item.flag)
            {
                WorkerCallback callback = (WorkerCallback)(void*)item.data;
                callback(argsBuffer);
                argsBufferSize = 0;
            }
            else
            {
                DBG_ASSERT(argsBufferSize < WORKER_MAX_ARGS);
                argsBuffer[argsBufferSize++] = item.data;
            }
        }
        else
        {
            timerExecute();
        }
    } while (true);
}


void workerInit(WorkerCallback startupCallback)
{
    workerQueue = xQueueCreateStatic(ARRAY_LENGTH(workerStaticQueueBuffer), sizeof(workerStaticQueueBuffer[0]),
        (uint8_t*)workerStaticQueueBuffer, &workerStaticQueue);
    workerTask = xTaskCreateStatic(workerMainLoop, "W", ARRAY_LENGTH(workerStaticStack), (void*)startupCallback,
        WORKER_PRIORITY, workerStaticStack, &workerStaticTask);
}


bool inWorkerThread()
{
    return xTaskGetCurrentTaskHandle() == workerTask;
}
