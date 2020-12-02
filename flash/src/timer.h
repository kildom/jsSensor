#ifndef _TIMER_H_
#define _TIMER_H_

#define MS2TICKS(x) ((x) * 1000)

EXTERN void initTimer();
EXTERN void shutdownTimer();
EXTERN uint32_t getTime();
EXTERN bool timedOut(uint32_t timeout);

#endif
