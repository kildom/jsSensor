
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

namespace worker {

static const uint8_t ITEM_TYPE_ARG = 0xFE;
static const uint8_t ITEM_TYPE_STD_FUNCTION = 0xFF;

struct ThreadData
{
    Level level;
    Callback startup;
};

struct QueueItem
{
    uintptr_t data;
    uint8_t type_or_args;
    uint8_t _reserved[3];
};

typedef uint8_t QueueItemBuffer[5];


static StaticTask_t staticTask[2];
static StackType_t lowStaticStack[WORKER_LOW_STACK_SIZE / sizeof(StackType_t)];
static StackType_t highStaticStack[WORKER_HIGH_STACK_SIZE / sizeof(StackType_t)];

static StaticQueue_t staticQueue[2];
static QueueItemBuffer lowStaticQueueBuffer[WORKER_LOW_QUEUE_LENGTH];
static QueueItemBuffer highStaticQueueBuffer[WORKER_HIGH_QUEUE_LENGTH];

static QueueHandle_t queue[2];
static TaskHandle_t task[2];

static ThreadData threadData[2] = { { LEVEL_LOW }, { LEVEL_HIGH } };

STATIC_ASSERT(offsetof(QueueItem, _reserved) == 5, "Unexpected location of members in QueueItem");
STATIC_ASSERT(sizeof(QueueItem) == 8, "Unexpected size of QueueItem");
STATIC_ASSERT(sizeof(lowStaticQueueBuffer) == 5 * WORKER_LOW_QUEUE_LENGTH, "Unexpected size of staticQueueBuffer");
STATIC_ASSERT(sizeof(highStaticQueueBuffer) == 5 * WORKER_HIGH_QUEUE_LENGTH, "Unexpected size of staticQueueBuffer");

#if !defined(configUSE_TIME_SLICING) || (configUSE_TIME_SLICING != 0)
STATIC_ASSERT(0, "Worker queue implementation is not safe for Time Slicing");
#endif


static void sendData(QueueHandle_t queue, uintptr_t data, uint8_t type_or_args)
{
    BaseType_t ret;
    QueueItem item;
    item.data = data;
    item.type_or_args = type_or_args;
    ret = xQueueSend(queue, &item, 0);
    PROD_ASSERT(ret == pdTRUE, "Work queue overflow");
}

static void addToValist(Level level, Callback callback, size_t args, va_list valist)
{
    size_t i;
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    for (i = 0; i < args; i++)
    {
        sendData(queue[level], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendData(queue[level], (uintptr_t)(void*)callback, (uint8_t)args);
}

void add(Callback callback, size_t args, ...)
{
    // TODOLATER: reconsider if this function is needed. How often it is used? Removing it will simplify the code.
    va_list valist;
    Level level = getLevel();
    va_start(valist, args);
    DBG_ASSERT(level != LEVEL_UNKNOWN, "add called not from a worker context!");
    addToValist(level, callback, args, valist);
}

void addTo(Level level, Callback callback, size_t args, ...)
{
    va_list valist;
    va_start(valist, args);
    addToValist(level, callback, args, valist);
}

static void sendDataFromISR(bool* yield, QueueHandle_t queue, uintptr_t data, uint8_t type_or_args)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret;
    QueueItem item;
    item.data = data;
    item.type_or_args = type_or_args;
    ret = xQueueSendFromISR(queue, &item, &woken);
    PROD_ASSERT(ret == pdTRUE, "Worker queue overflow");
    if (woken) *yield = true;
}

void addToFromISR(bool* yield, Level level, Callback callback, size_t args, ...)
{
    size_t i;
    va_list valist;
    va_start(valist, args);
    DBG_ASSERT(args <= WORKER_MAX_ARGS)
    for (i = 0; i < args; i++)
    {
        sendDataFromISR(yield, queue[level], va_arg(valist, uintptr_t), ITEM_TYPE_ARG);
    }
    sendDataFromISR(yield, queue[level], (uintptr_t)(void*)callback, (uint8_t)args);
}


static void mainLoop(void *param)
{
    static uintptr_t argsBuffer[WORKER_ARGS_BUFFER_SIZE];
    static QueueItem item;
    size_t argsBufferSize = 0;
    BaseType_t ret;
    uint32_t timeout;
    Callback callback;
    Level level = ((ThreadData*)param)->level;

    callback = ((ThreadData*)param)->startup;
    if (callback) callback(argsBuffer);

    do {
        timeout = timer::getNextTimeout(level);
        ret = pdFALSE;
        if (timeout != 0)
        {
            ret = xQueueReceive(queue[level], &item, timeout);
        }
        if (ret)
        {
            if (item.type_or_args == ITEM_TYPE_ARG)
            {
                PROD_ASSERT(argsBufferSize < ARRAY_LENGTH(argsBuffer));
                argsBuffer[argsBufferSize++] = item.data;
            }
            else if (item.type_or_args == ITEM_TYPE_STD_FUNCTION)
            {
                std::function<void()>* func = (std::function<void()>*)item.data;
                (*func)();
                delete func;
            }
            else
            {
                Callback callback = (Callback)(void*)item.data;
                argsBufferSize -= item.type_or_args;
                callback(&argsBuffer[argsBufferSize]);
            }
        }
        else
        {
            timer::execute(level);
        }
    } while (true);
}


void init(Callback startups[LEVELS_COUNT])
{
    threadData[0].startup = startups[LEVEL_LOW];
    threadData[1].startup = startups[LEVEL_HIGH];
    queue[LEVEL_LOW] = xQueueCreateStatic(ARRAY_LENGTH(lowStaticQueueBuffer),
        sizeof(lowStaticQueueBuffer[0]),
        (uint8_t*)lowStaticQueueBuffer, &staticQueue[LEVEL_LOW]);
    queue[LEVEL_HIGH] = xQueueCreateStatic(ARRAY_LENGTH(highStaticQueueBuffer),
        sizeof(highStaticQueueBuffer[0]),
        (uint8_t*)highStaticQueueBuffer, &staticQueue[LEVEL_HIGH]);
    task[LEVEL_LOW] = xTaskCreateStatic(mainLoop, "Lo", ARRAY_LENGTH(lowStaticStack),
        (void*)&threadData[LEVEL_LOW],
        WORKER_LOW_PRIORITY, lowStaticStack, &staticTask[LEVEL_LOW]);
    task[LEVEL_HIGH] = xTaskCreateStatic(mainLoop, "Hi", ARRAY_LENGTH(highStaticStack),
        (void*)&threadData[LEVEL_HIGH],
        WORKER_HIGH_PRIORITY, highStaticStack, &staticTask[LEVEL_HIGH]);
}


bool onLevel(Level level)
{
    return xTaskGetCurrentTaskHandle() == task[level];
}

Level getLevel()
{
    auto handle = xTaskGetCurrentTaskHandle();
    if (handle == task[LEVEL_LOW])
    {
        return LEVEL_LOW;
    }
    else if (handle == task[LEVEL_HIGH])
    {
        return LEVEL_HIGH;
    }
    else
    {
        return LEVEL_UNKNOWN;
    }
}

}; // namespace worker

}; // namespace low

using namespace low;

void setAsync(const std::function<void()>& func)
{
    worker::sendData(worker::queue[worker::LEVEL_HIGH], (uintptr_t)(void*)new std::function<void()>(func), worker::ITEM_TYPE_STD_FUNCTION);
}
