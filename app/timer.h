#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "worker.h"

namespace low
{

struct Timer;

namespace timer
{

typedef void (*Callback)(Timer* timer);

static const uint32_t FLAG_ABSOLUTE = 0x01000000;     /**< `time` parameter is an absolute time. */
static const uint32_t FLAG_REPEAT = 0x02000000;       /**< Timer will repeat if this flag is set. This flag have to be ORed with interval period. `time` parameter is delay before first callback execution. */
static const uint32_t FLAG_HIGH = 0x04000000;         /**< Timer will be executed in high level worker. */
static const uint32_t FLAG_INTERVAL_MAX = 0x00FFFFFF; /**< Maximum interval time value that can be ORed with flags. This is also mask that can be used to get interval from flags. */

void start(Timer* timer, uint32_t time, uint32_t flags);
void stop(Timer* timer);
void startFromISR(bool* yield, Timer* timer, uint32_t time, uint32_t flags);
void stopFromISR(bool* yield, Timer* timer);

uint32_t getNextTimeout(worker::Level level);
void execute(worker::Level level);

}

struct Timer
{
    timer::Callback callback;
    uint32_t fireTime;
    Timer* _next;
    uint32_t _flags;

    inline void start(uint32_t time, uint32_t flags = 0)
    {
        timer::start(this, time, flags);
    }

    inline void startInterval(uint32_t firstTime, uint32_t periodTime = 0, uint32_t flags = 0)
    {
        timer::start(this, firstTime, periodTime | flags | timer::FLAG_REPEAT);
    }

    inline void startContinuos(uint32_t time, uint32_t flags = 0)
    {
        timer::start(this, fireTime + time, flags | timer::FLAG_ABSOLUTE);
    }

    inline void stop()
    {
        timer::stop(this);
    }

    inline void startFromISR(bool* yield, uint32_t time, uint32_t flags = 0)
    {
        timer::startFromISR(yield, this, time, flags);
    }

    inline void startIntervalFromISR(bool* yield, uint32_t firstTime, uint32_t periodTime = 0, uint32_t flags = 0)
    {
        timer::startFromISR(yield, this, firstTime, periodTime | flags | timer::FLAG_REPEAT);
    }

    inline void startContinuosFromISR(bool* yield, uint32_t time, uint32_t flags = 0)
    {
        timer::startFromISR(yield, this, fireTime + time, flags | timer::FLAG_ABSOLUTE);
    }

    inline void stopFromISR(bool* yield)
    {
        timer::stopFromISR(yield, this);
    }

    inline void setCallback(timer::Callback callback)
    {
        this->callback = callback;
    }
};

}

#endif // _TIMER_H_
