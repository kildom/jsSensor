#ifndef _LITE_TRACE_H_
#define _LITE_TRACE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* EV ^ PARAM1[24], PARAM2[32] */
#define LTEV_EXTEND 0x00000000

/* EV ^ TIME */
#define LTEV_START 0x0C000000

/* EV ^ TIME, buffer cycle counter (only for fast overload checks ) */
#define LTEV_IDLE 0x01000000

/* EV ^ TIME, RESOURCE_ID ^ PARAM1[8] << 24 ^ PARAM2 [4]*/
#define LTEV_TASK_CREATE 0x02000000
#define LTEV_TASK_START_EXEC 0x03000000
#define LTEV_TASK_START_READY 0x04000000
#define LTEV_TASK_STOP_READY 0x05000000 // PARAM1 = REASON
#define LTEV_TASK_TERMINATE 0x06000000
#define LTEV_TIMER_ENTER 0x07000000
#define LTEV_TIMER_EXIT 0x08000000
#define LTEV_MARK_START 0x0D000000
#define LTEV_MARK_EVENT 0x0E000000
#define LTEV_MARK_STOP 0x0F000000

/* EV ^ PARAM1[0..23], RESOURCE_ID ^ PARAM1[24..32] << 24 ^ PARAM2 [4]*/
// TODO: reconsider those events format
#define LTEV_TASK_PRIORITY 0x09000000 // PARAM1 = priority
#define LTEV_TASK_NAME 0x0A000000 // name = EXT.PARAM1 & EXT.PARAM2 & PARAM1 (or more EXT)
#define LTEV_TASK_STACK 0x0B000000 // size = EXT.PARAM1 + PARAM1 << 24, location = EXT.PARAM2

/* EV ^ TIME ^ IRQ_NUM << 24 */
#define LTEV_IRQ_ENTER 0x40000000
#define LTEV_IRQ_EXIT 0x80000000
#define LTEV_IRQ_EXIT_TO_SCHEDULER 0xC0000000

/* EV */
#define LTEV_BUFFER_LAST 0x1F000001

#define LTEV_USER_FIRST 0x20000000
#define LTEV_USER_LAST 0x3F000000

uint32_t ltev_get_time()
{
    return 0;
}


//#define LTEV_NO_OVERFLOW_CHECK
//#define LTEV_FAST_OVERFLOW_CHECK
#define LTEV_FULL_OVERFLOW_CHECK
#define LTEV_FULL_OVERFLOW_CHECK_WITH_STATS

/*
LTEV_FAST_OVERFLOW_CHECK:
    - it is not perfect: if overrun happen before end of transmission then it may be not detected
    Last item in buffer: LTEV_BUFFER_LAST, counter counting on each buffer cycle
    If one item before last in buffer is written then index is to 0 to send also LTEV_BUFFER_LAST
    Additionally LTEV_IDLE sends current buffer cycle counter for faster detection.
LTEV_FULL_OVERFLOW_CHECK:
    If it is place packet in buffer then put there LTEV_OVERFLOW with counter 1.
    If there is no place in buffer then previous packet is LTEC_OVERFLOW, so increase counter. 
*/

extern volatile uint32_t ltev_min_size;

static inline void ltev_send(uint32_t event_number, uint32_t params)
{
    event_number |= ltev_get_time();
    enter_critical();
#if defined(LTEV_NO_OVERFLOW_CHECK)
    uint32_t index = ltev_index;
    *((volatile uint32_t*)&ltev_buffer[index]) = event_number;
    *((volatile uint32_t*)&ltev_buffer[index + 4]) = params;
    index = (index + 8) & LTEV_BUFFER_INDEX_MAX;
    ltev_index = index;
#elif defined(LTEV_FAST_OVERFLOW_CHECK)
    uint32_t index = ltev_index;
    *((volatile uint32_t*)&ltev_buffer[index]) = event_number;
    *((volatile uint32_t*)&ltev_buffer[index + 4]) = params;
    index = index + 8;
    if (index == LTEV_BUFFER_INDEX_MAX)
    {
        *((uint32_t*)&ltev_buffer[index + 4])++;
        index = 0;
    }
    ltev_index = index;
#else
    uint32_t index = ltev_index;
    uint32_t size = (ltev_rd_index - index) & LTEV_BUFFER_INDEX_MAX;
#if defined(LTEV_FULL_OVERFLOW_CHECK_WITH_STATS)
    // TODO: If size is smaller than minimum then if previous is LTEV_BUFFER_MIN then update param else add new LTEV_BUFFER_MIN event.
#endif
    if (size <= 16)
    {
        if (size == 8)
        {
            (*((volatile uint32_t*)&ltev_buffer[(index - 4) & LTEV_BUFFER_INDEX_MAX]))++;
            return;
        }
        event_number = LTEV_OVERFLOW;
        params = 1;
    }
    *((volatile uint32_t*)&ltev_buffer[index]) = event_number;
    *((volatile uint32_t*)&ltev_buffer[index + 4]) = params;
    index = (index + 8) & LTEV_BUFFER_INDEX_MAX;
#endif
    exit_critical();
}

static inline void ltev_send_no_time(uint32_t event_number, uint32_t params)
{
}

static inline void ltev_send_short(uint32_t event_number, uint32_t params)
{
}

static inline void ltev_idle()
{
    ltev_send_short(LTEV_IDLE, *((uint32_t*)&ltev_buffer[LTEV_BUFFER_INDEX_MAX + 4]));
}

static inline void ltev_start()
{
    ltev_send_short(LTEV_START, 0);
}

static inline void ltev_task_create(void* handle, const char* name, int priority, void* stack_location, size_t stack_size)
{
    ltev_send(LTEV_TASK_CREATE, (uint32_t)handle);
    ltev_send_no_time(LTEV_TASK_PRIORITY | (priority & 0x00FFFFFF), (uint32_t)handle ^ ((uint32_t)priority << 24));
    ltev_send_no_time(LTEV_TASK_STACK)
}

#endif // _LITE_TRACE_H_
