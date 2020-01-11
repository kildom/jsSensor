#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "worker.h"

namespace low
{

static const uint32_t TIMER_FLAG_ABSOLUTE = 0x01000000; /**< `time` parameter is an absolute time */
static const uint32_t TIMER_FLAG_REPEAT = 0x02000000;   /**< Timer will repeat if this flag is set. This flag have to be ORed with interval period. `time` parameter is delay before first callback execution. */
static const uint32_t TIMER_FLAG_HIGH = 0x04000000;     /**< Timer will be executed in high worker */
static const uint32_t TIMER_FLAG_INTERVAL_MASK = 0x00FFFFFF;

struct Timer;

typedef void (*TimerCallback)(Timer* timer);

struct Timer
{
    TimerCallback callback;
    uint32_t fireTime;
    Timer* _next;
    uint32_t _flags;
};

void timerStart(Timer* timer, uint32_t time, uint32_t flags);
void timerStop(Timer* timer);
void timerStartFromISR(bool* yieldRequested, Timer* timer, uint32_t time, uint32_t flags);
void timerStopFromISR(bool* yieldRequested, Timer* timer);

uint32_t timerGetNextTimeout(WorkerLevel level);
void timerExecute(WorkerLevel level);

}

#endif // _TIMER_H_
