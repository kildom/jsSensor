#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "worker.h"
#include "timer.h"
#include "timer.hpp"

namespace low {
namespace timer {

static const uint32_t FLAG_RUNNING = 0x08000000;

static Timer* timersList[worker::LEVELS_COUNT] = { NULL };

static Timer** getListPtr(Timer* timer)
{
    return (timer->_flags & FLAG_HIGH) ? &timersList[1] : &timersList[0];
}

static worker::Level getLevel(Timer* timer)
{
    return (timer->_flags & FLAG_HIGH) ? worker::LEVEL_HIGH : worker::LEVEL_LOW;
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
    if (!(flags & FLAG_ABSOLUTE))
    {
        time += xTaskGetTickCount();
    }
    return time;
}


static void startFW(uintptr_t* args)
{
    start((Timer*)args[0], args[1], args[2]);
}


void start(Timer* timer, uint32_t time, uint32_t flags)
{
    worker::Level level = getLevel(timer);
    time = calcAbsoluteTime(time, flags);
    if (worker::onLevel(level))
    {
        if (timer->_flags & FLAG_RUNNING)
        {
            removeItem(timer);
        }
        if ((flags & FLAG_REPEAT) && ((flags & FLAG_INTERVAL_MAX) == 0))
        {
            flags |= time;
        }
        timer->_flags = flags | FLAG_RUNNING;
        timer->fireTime = time;
        addItem(timer);
    }
    else
    {
        addTo(level, startFW, 3, (uintptr_t)timer, time, flags | FLAG_ABSOLUTE);
    }
}


static void stopFW(uintptr_t* args)
{
    stop((Timer*)args[0]);
}


void stop(Timer* timer)
{
    worker::Level level = getLevel(timer);
    if (worker::onLevel(level))
    {
        removeItem(timer);
        timer->_flags &= ~FLAG_RUNNING;
    }
    else
    {
        worker::addTo(level, stopFW, 1, (uintptr_t)timer);
    }
}


void startFromISR(bool* yield, Timer* timer, uint32_t time, uint32_t flags)
{
    worker::Level level = getLevel(timer);
    time = calcAbsoluteTime(time, flags);
    worker::addToFromISR(yield, level, startFW, 3, (uintptr_t)timer, time, flags | FLAG_ABSOLUTE);
}


void stopFromISR(bool* yield, Timer* timer)
{
    worker::Level level = getLevel(timer);
    worker::addToFromISR(yield, level, startFW, 1, (uintptr_t)timer);
}


uint32_t getNextTimeout(worker::Level level)
{
    if (timersList[level] == NULL)
    {
        return portMAX_DELAY;
    }
    int32_t t = timersList[level]->fireTime - xTaskGetTickCount();
    if (t <= 0) return 0;
    return (uint32_t)t;
}


void execute(worker::Level level)
{
    uint32_t t = getNextTimeout(level);
    if (t != 0) return;
    Timer* timer = timersList[level];
    timersList[level] = timer->_next;
    if (timer->_flags & FLAG_REPEAT)
    {
        timer->fireTime += timer->_flags & FLAG_INTERVAL_MAX;
        addItem(timer);
    }
    else
    {
        timer->_flags &= ~FLAG_RUNNING;
    }
    timer->callback(timer);
}

} // namespace timer
} // namespace low


using namespace low;

struct _TimerHighInternal
{
    low::Timer timer;
    std::function<void()> func;
};

void _destruct_TimerHighInternal(struct _TimerHighInternal* ptr)
{
    ptr->~_TimerHighInternal();
}


static uint32_t float2ticks(float sec)
{
    int32_t x = (int32_t)((32768.0f / (float)_PRESCALE) * sec + 0.5f);
    return x <= 0 ? 1 : x;
}

static void highLevelTimeoutCallback(Timer* timer)
{
    DBG_ASSERT(inThread(WORKER_HIGH), "Must be called from high level worker.");
    _TimerHighInternal* t = (_TimerHighInternal*)timer;
    t->func();
    if (!(timer->_flags & timer::FLAG_RUNNING))
    {
        TimeoutHandle::deleteUnmanaged(t);
    }
}

static TimeoutHandle setHighLevelTimeout(const std::function<void()>& func, float sec, uint32_t flags)
{
    DBG_ASSERT(inThread(WORKER_HIGH), "Must be called from high level worker.");
    TimeoutHandle ref = TimeoutHandle::make();
    _TimerHighInternal* t = ref.createUnmanaged();
    t->func = func;
    t->timer.callback = highLevelTimeoutCallback;
    timer::start(&t->timer, float2ticks(sec), flags);
    return ref;
}

TimeoutHandle setTimeout(const std::function<void()>& func, float sec)
{
    return setHighLevelTimeout(func, sec, timer::FLAG_HIGH);
}

TimeoutHandle setInterval(const std::function<void()>& func, float intervalSec, float startSec)
{
    return setHighLevelTimeout(func, startSec < 0.0f ? intervalSec : startSec, float2ticks(intervalSec) | timer::FLAG_HIGH | timer::FLAG_REPEAT);
}

void stopTimeout(TimeoutHandle timeout)
{
    DBG_ASSERT(inThread(LEVEL_HIGH), "Must be called from high level worker.");
    if (timeout->timer._flags & timer::FLAG_RUNNING)
    {
        timer::removeItem(&timeout->timer);
        timeout->timer._flags &= ~timer::FLAG_RUNNING;
        TimeoutHandle::deleteUnmanaged(timeout.ptr);
    }
}

