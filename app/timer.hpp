#ifndef _TIMER_HPP_
#define _TIMER_HPP_

#include "ref.h"
#include "timer.h"

struct TimerHigh
{
    low::Timer timer;
    std::function<void()> func;
};

typedef Ref<TimerHigh> TimeoutHandle;

TimeoutHandle setTimeout(const std::function<void()>& func, float sec);
TimeoutHandle setInterval(const std::function<void()>& func, float intervalSec, float startSec = -1.0f);
void stopTimeout(TimeoutHandle timeout);
static inline void stopInterval(const TimeoutHandle& interval) { stopTimeout(interval); }

#endif // _TIMER_HPP_
