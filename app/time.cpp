#include <limits>
#include <algorithm>

#include "FreeRTOS.h"
#include "task.h"

#include "timer.h"


namespace low {

static uint64_t increment = 0;
static TickType_t lastTick = 0;

void checkOverflow(Timer*)
{
    getTime();
}

Timer timer = { checkOverflow };

void timeInit()
{
    uint32_t t = std::min(std::numeric_limits<TickType_t>::max / 8, TIMER_FLAG_INTERVAL_MAX);
    timerStart(&timer, t, t | TIMER_FLAG_REPEAT | TIMER_FLAG_HIGH);
}

}

using namespace low;


int64_t getTime()
{
    TickType_t currentTick = xTaskGetTickCount();
    BaseType_t saved = taskENTER_CRITICAL_FROM_ISR();
    uint64_t incrementCopy = increment;
    TickType_t lastTickCopy = lastTick;
    taskEXIT_CRITICAL_FROM_ISR(saved);
    if (currentTick < lastTickCopy)
    {
        incrementCopy += (uint64_t)std::numeric_limits<TickType_t>::max + (uint64_t)1;
    }
    return incrementCopy + currentTick;
}


