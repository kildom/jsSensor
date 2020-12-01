#ifndef _COMMON_H_
#define _COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <nrf.h>

#ifdef DUMMY_LTO
#define EXTERN static
#else
#define EXTERN
#endif

#define NOINIT_DATA __attribute__((section(".noinit")))

#define BLOCK_SIZE 128
#define RADIO_FREQUENCY_MHZ 2478

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define FORCE_LONG_JUMP(func) ((typeof(&(func)))_LONG_JUMP_helper(&(func)))
static inline void* _LONG_JUMP_helper(void* p) {
	__asm__ volatile ("":"+r"(p));
	return p;
}

#ifdef __unix
#undef __unix
#endif

#include "main.h"
#include "utils.h"
#include "crypto.h"
#include "startup.h"
#include "radio.h"
#include "timer.h"
#include "rand.h"

#endif