#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "FreeRTOSConfig.h"
#include "config.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define _PRESCALE ((double)((32768 + (configTICK_RATE_HZ / 2)) / configTICK_RATE_HZ))
#define SEC2TICKS(sec) MAX((uint32_t)1, (uint32_t)(32768.0 / _PRESCALE * (double)(sec) + 0.5))
#define MS2TICKS(ms) MAX((uint32_t)1, (uint32_t)(32.768 / _PRESCALE * (double)(ms) + 0.5))

#define ARRAY_LENGTH(a) (sizeof((a)) / sizeof((a)[0]))

#define PROD_ASSERT(cond, ...) // Production assert - always enabled
#define DBG_ASSERT(cond, ...) // Debug assert - enabled only in debug build
#define STATIC_ASSERT static_assert // Static assert - compilation time assert

#define PROD_STATIC // Static only in production build. Debug build will not be static to simplify debugging.

// TODO: Check if correct task with DBG_ASSERT
// TODO: Add at the beginning of each function
// TODO: Use VECTACTIVE from ICSR from SCB to detect if we are in interrupt
#define TASK_HIGH
#define TASK_LOW
#define TASK_IRQ
#define TASK_LOW_OR_HIGH
#define TASK_HIGH_OR_LOW
#define TASK_IRQ_OR_LOW
#define TASK_LOW_OR_IRQ
#define TASK_ANY

#define LOG(...)
#define LOG_WARNING(...)

#endif // _COMMON_H_
