#ifndef _FAST_TIMER_H_
#define _FAST_TIMER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define _FAST_PRECALE ((double)(1 << FAST_TIMER_PRESCALER))
#define SEC2FAST(sec) MAX((uint32_t)1, (uint32_t)((double)(sec) * 16000000.0 / _FAST_PRECALE + 0.5))
#define MS2FAST(ms) MAX((uint32_t)1, (uint32_t)((double)(ms) * 16000.0 / _FAST_PRECALE + 0.5))
#define US2FAST(us) MAX((uint32_t)1, (uint32_t)((double)(us) * 16.0 / _FAST_PRECALE + 0.5))

#define FAST_TIMER_FLAG_ABSOLUTE 0x01000000 /**< `time` parameter is an absolute time */
#define FAST_TIMER_FLAG_REPEAT 0x02000000   /**< Timer will repeat if this flag is set. This flag have to be ORed with interval period. `time` parameter is delay before first callback execution. */
#define FAST_TIMER_FLAG_INTERVAL_MASK 0x00FFFFFF

struct FastTimer_tag;

typedef void (*FastTimerCallback)(bool* yield, struct FastTimer_tag* timer);

typedef struct FastTimer_tag
{
    FastTimerCallback callback;
    uint32_t fireTime;
    struct FastTimer_tag* _next;
    uint32_t _flags;
} FastTimer;

void fastTimerInit();
void fastTimerStart(FastTimer* timer, uint32_t time, uint32_t flags);
void fastTimerStop(FastTimer* timer);

#endif // _FAST_TIMER_H_
