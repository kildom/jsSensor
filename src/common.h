#ifndef _COMMON_H_
#define _COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define FLASH_FUNC __attribute__((section(".flashtext")))
#define FLASH_DATA __attribute__((section(".flashrodata")))
#define NOINIT_DATA __attribute__((section(".noinit")))

#define FORCE_LONG_JUMP(func) ((typeof(&(func)))_LONG_JUMP_helper(&(func)))
static inline void* _LONG_JUMP_helper(void* p) {
	__asm__ volatile ("":"+r"(p));
	return p;
}

extern uint8_t __begin_boot_flash;
extern uint8_t __size_boot_flash;
extern uint8_t __begin_app_flash;
extern uint8_t __size_app_flash;
extern uint8_t __begin_stack;

#define BEGIN_BOOT_FLASH ((uintptr_t)&__begin_boot_flash)
#define SIZE_BOOT_FLASH ((uintptr_t)&__size_boot_flash)
#define BEGIN_APP_FLASH ((uintptr_t)&__begin_app_flash)
#define SIZE_APP_FLASH ((uintptr_t)&__size_app_flash)
#define BEGIN_STACK ((uintptr_t)&__begin_stack)

#define BLOCK_SIZE 128
#define CHUNK_BLOCKS 32
#define CHUNK_SIZE (CHUNK_BLOCKS * BLOCK_SIZE)
#define RADIO_FREQUENCY_MHZ 2478

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#if defined(NRF52832_XXAA)
#include "conf_nrf52832.h"
#include "system_nrf52.h"
#elif defined(NRF51822_XXAA)
#include "conf_nrf51822.h"
#include "system_nrf51.h"
#else
#error No configuration for this target
#endif

#ifdef __unix
#undef __unix
#endif

#endif