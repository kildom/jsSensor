#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "worker.h"
#include "timer.h"

#define TIMER_INT_FLAG_RUNNING 0x08000000

static Timer* timersList[2] = { NULL, NULL };

static Timer** getListPtr(Timer* timer)
{
    return (timer->_flags & TIMER_FLAG_HIGH) ? &timersList[1] : &timersList[0];
}

static Timer* getList(Timer* timer)
{
    return (timer->_flags & TIMER_FLAG_HIGH) ? timersList[1] : timersList[0];
}

static WorkerLevel getLevel(Timer* timer)
{
    return (timer->_flags & TIMER_FLAG_HIGH) ? WORKER_HIGH : WORKER_LOW;
}

static void addItem(Timer* timer)
{
    Timer** curPtr = getListPtr(timer);
    Timer* cur = *curPtr;
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
    Timer** curPtr = getListPtr(timer);
    Timer* cur = *curPtr;
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
    WorkerLevel level = getLevel(timer);
    time = calcAbsoluteTime(time, flags);
    if (!workerInThread(level))
    {
        if (level == WORKER_LOW)
        {
            workerLow(timerStartFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
        }
        else
        {
            workerHigh(timerStartFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
        }
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
    WorkerLevel level = getLevel(timer);
    if (!workerInThread(level))
    {
        if (level == WORKER_LOW)
        {
            workerLow(timerStopFW, 1, (uintptr_t)timer);
        }
        else
        {
            workerHigh(timerStopFW, 1, (uintptr_t)timer);
        }
        return;
    }
    removeItem(timer);
    timer->_flags &= ~TIMER_INT_FLAG_RUNNING;
}


void timerStartFromISR(bool* yieldRequested, Timer* timer, uint32_t time, uint32_t flags)
{
    WorkerLevel level = getLevel(timer);
    time = calcAbsoluteTime(time, flags);
    if (level == WORKER_LOW)
    {
        workerLowFromISR(yieldRequested, timerStartFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
    }
    else
    {
        workerHighFromISR(yieldRequested, timerStartFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
    }
}


void timerStopFromISR(bool* yieldRequested, Timer* timer)
{
    WorkerLevel level = getLevel(timer);
    if (level == WORKER_LOW)
    {
        workerLowFromISR(yieldRequested, timerStartFW, 1, (uintptr_t)timer);
    }
    else
    {
        workerHighFromISR(yieldRequested, timerStartFW, 1, (uintptr_t)timer);
    }
}


uint32_t timerGetNextTimeout(WorkerLevel level)
{
    if (timersList == NULL)
    {
        return portMAX_DELAY;
    }
    int32_t t = timersList[level]->fireTime - xTaskGetTickCount();
    if (t <= 0) return 0;
    return (uint32_t)t;
}


void timerExecute(WorkerLevel level)
{
    uint32_t t = timerGetNextTimeout(level);
    if (t != 0) return;
    Timer* timer = timersList[level];
    timersList[level] = timer->_next;
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

static void highLevelTimeoutCallback(Timer* timer)
{
    TimerEx* t = (TimerEx*)timer;
    t->func();
    if (!(timer->_flags & TIMER_FLAG_REPEAT))
        t->self = NULL;
}

static uint32_t float2ticks(float sec)
{
    return 0;
}

std::shared_ptr<TimerEx> setHighLevelTimeout(const std::function<void()>& func, float sec, uint32_t flags)
{
    std::shared_ptr<TimerEx> shptr = std::make_shared<TimerEx>();
    TimerEx* t = shptr.get();
    t->func = func;
    t->timer.callback = highLevelTimeoutCallback;
    t->self = shptr;
    timerStart(&t->timer, float2ticks(sec), flags);
    return shptr;
}

std::shared_ptr<TimerEx> setTimeout(const std::function<void()>& func, float sec)
{
    return setHighLevelTimeout(func, sec, TIMER_FLAG_HIGH);
}

std::shared_ptr<TimerEx> setInterval(const std::function<void()>& func, float intervalSec, float startSec)
{
    return setHighLevelTimeout(func, startSec < 0.0f ? intervalSec : startSec, float2ticks(intervalSec) | TIMER_FLAG_HIGH | TIMER_FLAG_REPEAT);
}
