#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <memory>

#include "ref.h"
#include "worker.h"

#define TIMER_FLAG_ABSOLUTE 0x01000000 /**< `time` parameter is an absolute time */
#define TIMER_FLAG_REPEAT 0x02000000   /**< Timer will repeat if this flag is set. This flag have to be ORed with interval period. `time` parameter is delay before first callback execution. */
#define TIMER_FLAG_HIGH 0x04000000     /**< Timer will be executed in high worker */
#define TIMER_FLAG_INTERVAL_MASK 0x00FFFFFF

struct Timer;

typedef void (*TimerCallback)(Timer* timer);

struct Timer
{
    TimerCallback callback;
    uint32_t fireTime;
    Timer* _next;
    uint32_t _flags;
};

struct TimerEx
{
    Timer timer;
    std::function<void()> func;
    std::shared_ptr<TimerEx> self;
};

/* How to add user data to timer:
struct MyTimer_tag
{
    Timer timer;
    uint8_t some;
    bool data;
} myTimer;
void callback(struct Timer_tag* timer)
{
    struct MyTimer_tag *data = (struct MyTimer_tag*)timer;
    f2(data->some);
}
void main()
{
    initTimer(&myTimer.timer, callback);
}
*/
 
void timerStart(Timer* timer, uint32_t time, uint32_t flags);
void timerStop(Timer* timer);
void timerStartFromISR(bool* yieldRequested, Timer* timer, uint32_t time, uint32_t flags);
void timerStopFromISR(bool* yieldRequested, Timer* timer);

std::shared_ptr<TimerEx> setTimeout(const std::function<void()>& func, float sec);
std::shared_ptr<TimerEx> setInterval(const std::function<void()>& func, float intervalSec, float startSec = -1.0f);

uint32_t timerGetNextTimeout(WorkerLevel level);
void timerExecute(WorkerLevel level);

#endif // _TIMER_H_
