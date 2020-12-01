
#include "common.h"


EXTERN void initTimer()
{
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	NRF_TIMER0->PRESCALER = 4;
	NRF_TIMER0->TASKS_START = 1;
}

EXTERN void shutdownTimer()
{
	NRF_TIMER0->TASKS_STOP = 1;
}

EXTERN uint32_t getTime()
{
	NRF_TIMER0->TASKS_CAPTURE[0] = 1;
	return NRF_TIMER0->CC[0];
}

EXTERN bool timedOut(uint32_t timeout)
{
    int32_t diff = timeout - getTime();
	return diff <= 0;
}
