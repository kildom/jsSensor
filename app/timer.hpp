#ifndef _TIMER_HPP_
#define _TIMER_HPP_

#include <functional>

#include "ref.h"

typedef Ref<struct _TimerHighInternal> TimeoutHandle;

template<>
void destruct<struct _TimerHighInternal>(struct _TimerHighInternal* ptr)
{
    // Special destructor for incomplete type (used by Ref<>)
    void _destruct_TimerHighInternal(struct _TimerHighInternal* ptr);
    _destruct_TimerHighInternal(ptr);
}

TimeoutHandle setTimeout(const std::function<void()>& func, float sec);
TimeoutHandle setInterval(const std::function<void()>& func, float intervalSec, float startSec = -1.0f);
void stopTimeout(TimeoutHandle timeout);
static inline void stopInterval(const TimeoutHandle& interval) { stopTimeout(interval); }

#endif // _TIMER_HPP_
