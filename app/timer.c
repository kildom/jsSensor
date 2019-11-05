#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "worker.h"
#include "timer.h"

#define TIMER_INT_FLAG_RUNNING 0x08000000

static Timer* timersList = NULL;


static void addItem(Timer* timer)
{
    Timer** curPtr = &timersList;
    Timer* cur = timersList;
    while (cur && (int32_t)(cur->fireTime - timer->fireTime) <= 0)
    {
        curPtr = &cur->_next;
        cur = cur->_next;
    }
    *curPtr = timer;
    timer->_next = cur;
}


static void removeItem(Timer* timer)
{
    Timer** curPtr = &timersList;
    Timer* cur = timersList;
    while (cur)
    {
        if (cur == timer)
        {
            *curPtr = timer->_next;
            return;
        }
        curPtr = &cur->_next;
        cur = cur->_next;
    }
}


static uint32_t calcAbsoluteTime(uint32_t time, uint32_t flags)
{
    if (!(flags & TIMER_FLAG_ABSOLUTE))
    {
        time += xTaskGetTickCount();
    }
    return time;
}


static void timerStartFW(uintptr_t* args)
{
    timerStart((Timer*)args[0], args[1], args[2]);
}


void timerStart(Timer* timer, uint32_t time, uint32_t flags)
{
    time = calcAbsoluteTime(time, flags);
    if (!inWorkerThread())
    {
        workerSend(timerStartFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
        return;
    }
    if (timer->_flags & TIMER_INT_FLAG_RUNNING)
    {
        removeItem(timer);
    }
    timer->_flags = flags | TIMER_INT_FLAG_RUNNING;
    timer->fireTime = time;
    addItem(timer);
}


static void timerStopFW(uintptr_t* args)
{
    timerStop((Timer*)args[0]);
}


void timerStop(Timer* timer)
{
    if (!inWorkerThread())
    {
        workerSend(timerStopFW, 1, (uintptr_t)timer);
        return;
    }
    removeItem(timer);
    timer->_flags &= ~TIMER_INT_FLAG_RUNNING;
}


void timerStartFromISR(bool* yieldRequested, Timer* timer, uint32_t time, uint32_t flags)
{
    time = calcAbsoluteTime(time, flags);
    workerSendFromISR(yieldRequested, timerStartFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
}


void timerStopFromISR(bool* yieldRequested, Timer* timer)
{
    workerSendFromISR(yieldRequested, timerStartFW, 1, (uintptr_t)timer);
}


uint32_t timerGetNextTimeout()
{
    if (timersList == NULL)
    {
        return portMAX_DELAY;
    }
    int32_t t = timersList->fireTime - xTaskGetTickCount();
    if (t <= 0) return 0;
    return (uint32_t)t;
}


void timerExecute()
{
    uint32_t t = timerGetNextTimeout();
    if (t != 0) return;
    Timer* timer = timersList;
    timersList = timer->_next;
    if (timer->_flags & TIMER_FLAG_REPEAT)
    {
        timer->fireTime += timer->_flags & TIMER_FLAG_INTERVAL_MASK;
        addItem(timer);
    }
    else
    {
        timer->_flags &= ~TIMER_INT_FLAG_RUNNING;
    }
    timer->callback(timer);
}
