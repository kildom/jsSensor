#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "fast_timer.h"

#include "nrf.h"

#define FAST_TIMER_INT_FLAG_RUNNING 0x04000000

static FastTimer* timersList = NULL;
static bool timerIsRunning = false;


static bool addItem(FastTimer* timer)
{
    bool headChanged = true;
    FastTimer** curPtr = &timersList;
    FastTimer* cur = timersList;
    while (cur && (int32_t)(cur->fireTime - timer->fireTime) <= 0)
    {
        curPtr = &cur->_next;
        cur = cur->_next;
        headChanged = false;
    }
    *curPtr = timer;
    timer->_next = cur;
    return headChanged;
}


static bool removeItem(FastTimer* timer)
{
    bool headChanged = true;
    FastTimer** curPtr = &timersList;
    FastTimer* cur = timersList;
    while (cur)
    {
        if (cur == timer)
        {
            *curPtr = timer->_next;
            return headChanged;
        }
        curPtr = &cur->_next;
        cur = cur->_next;
        headChanged = false;
    }
    return headChanged;
}


static uint32_t calcAbsoluteTime(uint32_t time, uint32_t flags)
{
    if (!(flags & FAST_TIMER_FLAG_ABSOLUTE))
    {
        FAST_TIMER_TIMER->TASKS_CAPTURE[3] = 1;
        time += FAST_TIMER_TIMER->CC[3];
    }
    return time;
}


static void updateTimeout()
{
    int32_t diff;
    uint32_t current;
    uint32_t nextTimeout;
    if (timersList == NULL)
    {
        FAST_TIMER_TIMER->TASKS_STOP = 1;
        timerIsRunning = false;
        return;
    }
    else if (!timerIsRunning)
    {
        FAST_TIMER_TIMER->TASKS_START = 1;
        timerIsRunning = true;
    }
    nextTimeout = timersList->fireTime;
    FAST_TIMER_TIMER->TASKS_CAPTURE[3] = 1;
    current = FAST_TIMER_TIMER->CC[3];
    diff = nextTimeout - current;
    do {
        if (diff <= 0) nextTimeout = current + 1;
        FAST_TIMER_TIMER->CC[0] = nextTimeout;
        FAST_TIMER_TIMER->CC[1] = nextTimeout + 1;
        FAST_TIMER_TIMER->TASKS_CAPTURE[3] = 1;
        current = FAST_TIMER_TIMER->CC[3];
        diff = nextTimeout - current;
    } while (diff <= 0);
}


void fastTimerStart(FastTimer* timer, uint32_t time, uint32_t flags)
{
    BaseType_t saved;
    bool headChanged = false;
    time = calcAbsoluteTime(time, flags);
    saved = taskENTER_CRITICAL_FROM_ISR();
    if (timer->_flags & FAST_TIMER_INT_FLAG_RUNNING)
    {
        headChanged = removeItem(timer);
    }
    taskEXIT_CRITICAL_FROM_ISR(saved);
    timer->_flags = flags | FAST_TIMER_INT_FLAG_RUNNING;
    timer->fireTime = time;
    saved = taskENTER_CRITICAL_FROM_ISR();
    headChanged = addItem(timer) || headChanged;
    taskEXIT_CRITICAL_FROM_ISR(saved);
    if (headChanged)
    {
        saved = taskENTER_CRITICAL_FROM_ISR();
        updateTimeout();
        taskEXIT_CRITICAL_FROM_ISR(saved);
    }
}


void fastTimerStop(FastTimer* timer)
{
    BaseType_t saved;
    bool headChanged;
    saved = taskENTER_CRITICAL_FROM_ISR();
    headChanged = removeItem(timer);
    timer->_flags &= ~FAST_TIMER_INT_FLAG_RUNNING;
    taskEXIT_CRITICAL_FROM_ISR(saved);
    if (headChanged)
    {
        saved = taskENTER_CRITICAL_FROM_ISR();
        updateTimeout();
        taskEXIT_CRITICAL_FROM_ISR(saved);
    }
}


void fastTimerInit()
{
    FAST_TIMER_TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
    FAST_TIMER_TIMER->INTENSET = TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos;
    FAST_TIMER_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    FAST_TIMER_TIMER->PRESCALER = 6;
    FAST_TIMER_TIMER->TASKS_CLEAR = 1;
    NVIC_EnableIRQ(FAST_TIMER_IRQn);
    timerIsRunning = false;
}

extern "C"
void FAST_TIMER_IRQ()
{
    BaseType_t saved;
    bool yieldRequested = false;
    int32_t diff;
    uint32_t current;
    uint32_t nextTimeout;
    FastTimer* timer;
    FAST_TIMER_TIMER->TASKS_CAPTURE[3] = 1;
    FAST_TIMER_TIMER->EVENTS_COMPARE[0] = 0;
    FAST_TIMER_TIMER->EVENTS_COMPARE[1] = 0;
    current = FAST_TIMER_TIMER->CC[3];
    saved = taskENTER_CRITICAL_FROM_ISR();
    while (true)
    {
        timer = timersList;
        if (timer == NULL) break;
        nextTimeout = timer->fireTime;
        diff = nextTimeout - current;
        if (diff > 0) break;
        timersList = timer->_next;
        if (timer->_flags & FAST_TIMER_FLAG_REPEAT)
        {
            timer->fireTime += timer->_flags & FAST_TIMER_FLAG_INTERVAL_MASK;
            addItem(timer);
        }
        else
        {
            timer->_flags &= ~FAST_TIMER_INT_FLAG_RUNNING;
        }
        taskEXIT_CRITICAL_FROM_ISR(saved);
        timer->callback(&yieldRequested, timer);
        saved = taskENTER_CRITICAL_FROM_ISR();
    }
    updateTimeout();
    taskEXIT_CRITICAL_FROM_ISR(saved);
    portYIELD_FROM_ISR(yieldRequested);
}
