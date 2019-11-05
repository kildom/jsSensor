#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define TIMER_FLAG_ABSOLUTE 0x01000000 /**< `time` parameter is an absolute time */
#define TIMER_FLAG_REPEAT 0x02000000   /**< Timer will repeat if this flag is set. This flag have to be ORed with interval period. `time` parameter is delay before first callback execution. */
#define TIMER_FLAG_FAST 0x04000000     /**< Timer will use HW timer to provide precise time mesurement. `callback` is executed from ISR context. */

struct Timer_tag;

typedef void (*TimerCallback)(struct Timer_tag* timer);

typedef struct Timer_tag
{
    struct Timer_tag* next;
    uint32_t fireTime;
    uint32_t flags;
    TimerCallback callback;
} Timer;

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

void timerInit(Timer* timer, TimerCallback callback);
void timerStart(Timer* timer, uint32_t time, uint32_t flags);
void timerStop(Timer* timer);
void timerStartFromISR(bool* yieldRequested, Timer* timer, uint32_t time, uint32_t flags);
void timerStopFromISR(bool* yieldRequested, Timer* timer);

uint32_t timerGetNextTimeout();
void timerExecute();

#endif // _TIMER_H_
