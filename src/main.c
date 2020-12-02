#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <nrf.h>

#define ISR_VECTOR_ITEMS 48
#define STACK_SIZE 512
#define RAM_APP_SIZE (16 * 1024 - 1000)
#define BOOTLOADER_START 0x000
#define FLASH_APP_START 0x400
#define FLASH_SIZE 0x40000
#define DIRECT_ISR 1
#define INDIRECT_ISR 0
#define MOVED_ISR 0
#define ICSR_ADDRESS 0xE000ED04
#define PAGE_SIZE 1024
#define RAM_END 0x20004000

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define NO_RETURN __builtin_unreachable()

#define _STR2(x) #x
#define _STR1(x) _STR2(x)
#define STR(x) _STR1(x)

#define FORCE_LONG_JUMP(func) ((typeof(&(func)))_LONG_JUMP_helper(&(func)))
static inline void* _LONG_JUMP_helper(void* p) {
    __asm__ volatile ("":"+r"(p));
    return p;
}

__attribute__((section(".noinit")))
uint32_t stack[STACK_SIZE / 4];

uint32_t* const bootloaderIRQTable = (uint32_t*)BOOTLOADER_START;
uint32_t* const flashAppIRQTable = (uint32_t*)FLASH_APP_START;

volatile uint32_t* const flashAppStack = (uint32_t*)FLASH_APP_START;
volatile uint32_t* const flashAppReset = (uint32_t*)(FLASH_APP_START + 4);


__attribute__ ((noreturn))
void resetHandler();

#if INDIRECT_ISR
__attribute__((naked))
void indirectISR();
#define ISR_ARRAY_1 indirectISR
#else
#define ISR_ARRAY_1 NULL
#endif

#define ISR_ARRAY_2 ISR_ARRAY_1, ISR_ARRAY_1
#define ISR_ARRAY_4 ISR_ARRAY_2, ISR_ARRAY_2
#define ISR_ARRAY_8 ISR_ARRAY_4, ISR_ARRAY_4
#define ISR_ARRAY_16 ISR_ARRAY_8, ISR_ARRAY_8
#define ISR_ARRAY_32 ISR_ARRAY_16, ISR_ARRAY_16
#define ISR_ARRAY_64 ISR_ARRAY_32, ISR_ARRAY_32


__attribute__((used))
__attribute__((section(".isr_vector")))
static const void* isr_vector[] = {
    &stack[sizeof(stack) / sizeof(stack[0]) - 1],
    resetHandler,
    ISR_ARRAY_32, ISR_ARRAY_8, ISR_ARRAY_4, ISR_ARRAY_2 // 46 = 32 + 8 + 4 + 2
};

_Static_assert(ISR_VECTOR_ITEMS == ARRAY_SIZE(isr_vector), "Number of items in isr_vector is invalid.");


__attribute__((section(".app_entry")))
__attribute__((naked))
__attribute__((noinline))
void appEntry(void* connInfo, const void* key)
{
    asm volatile("");
}

__attribute__((section(".app_area")))
uint8_t appArea[RAM_APP_SIZE];


#if NRF51

#define WRITE_PROTECTION_PATTERN 0xD34257774C69E7AAuLL
volatile uint64_t* const writeProtectionData = (volatile uint64_t*)RAM_END - 1;

__attribute__((section(".selfProgPage1")))
__attribute__((noinline))
void selfProgPage1(uint32_t* src)
{
    uint32_t* dst = (uint32_t*)(1 * PAGE_SIZE);
    uint32_t* srcEnd = (uint32_t*)((uint8_t*)src + PAGE_SIZE);
    // Enable erase
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    // Erase page
    if (*writeProtectionData == WRITE_PROTECTION_PATTERN) {
        NRF_NVMC->ERASEPCR0 = (uint32_t)dst;
    }
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    // Enable write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    // Write
    while (src < srcEnd && *writeProtectionData == WRITE_PROTECTION_PATTERN) {
        *dst++ = *src++;
    }
    // Disable write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
}


__attribute__((section(".selfProgPage0")))
__attribute__((noinline))
void selfProgPage0(uint32_t* src)
{
    /*
    TODO: More safe programming.

        page 0-1  - ISR, reset handler <= 0x1FF, bootloader, selfprog page 2
        page 2    - selfprog page 0-2, emergency recover, configuration
        -- END OF PROTECTION --
        page 3    - safety copy of page 2 during booloader programming
        page 4-5  - safety copy of page 0-1 during booloader programming (flash app at normal state)

        Page 0-2 programming:
            1. (done be caller) Program pages 4-5 as safety copy of page 0-1
            2. Erase page 0
            3. Program reset vector to point emergency recover
            4. Program page 0 except reset vector
            5. Erase page 1
            6. Program page 1
            7. Program reset vector

        Emergency recover:
            1. Set SP in runtime (may be invalid)
            2. Program page 0 except reset vector (compare each word before programming to avoid to many writes)
            5. Erase page 1-2
            6. Program page 1-2
            7. Program reset vector
            8. Jump to bootloader reset handler

        Page 3 programming:
            1. (done be caller) Program page 3 as safety copy of page 2
            2. Erase page 2
            3. Program page
            4. Program page validation words
                if bootloader see that validation words are invalid it will jump into point 2

        reset vector:
        ...111111111111111 - after erase
        ...000110111111111 - emergency recover (0xDFF)
        ...000000xxxxxxxxx - allowed reset handler in bootloader

    */
    uint32_t* dst = (uint32_t*)(0 * PAGE_SIZE);
    uint32_t* srcEnd = (uint32_t*)((uint8_t*)src + PAGE_SIZE);
    // Enable erase
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    // Erase page
    if (*writeProtectionData == WRITE_PROTECTION_PATTERN) {
        NRF_NVMC->ERASEPCR0 = (uint32_t)dst;
    }
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    // Enable write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    // Write
    while (src < srcEnd && *writeProtectionData == WRITE_PROTECTION_PATTERN) {
        *dst++ = *src++;
    }
    // Disable write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
}

#endif


static void memZero(uint32_t* dst, size_t size)
{
    uint32_t* e = (uint32_t*)((uint8_t*)dst + size);
    while (dst < e) {
        *dst++ = 0;
    }
}

static uint32_t memNotEqual(const uint32_t* a, const uint32_t* b, size_t size)
{
    uint32_t* e = (uint32_t*)((uint8_t*)a + size);
    uint32_t r = 0;
    while (a < e) {
        r |= *a++ ^ *b++;
    }
    return r;
}

static void memCopy(uint32_t* dst, const uint32_t* src, size_t size)
{
    uint32_t* e = (uint32_t*)((uint8_t*)dst + size);
    while (dst < e) {
        *dst++ = *src++;
    }
}


__attribute__ ((noreturn))
void startFlashApp() /* TODO: move to page 1 if there is no space on page 0. Before entering check if page 1 is valid. */
{
#if MOVED_ISR
    // Just move vector table offset to flash app
    SCB->VTOR = (uint32_t)flashAppIRQTable;
#elif INDIRECT_ISR
    // Nothing else to do, each ISR will be redirected in runtime by indirectISR()
#elif DIRECT_ISR
    // If ISR vector table on flash app was updated copy it and reprogram page 0
    if (memNotEqual(&flashAppIRQTable[2], &bootloaderIRQTable[2], 4 * (ISR_VECTOR_ITEMS - 2)))
    {
        memCopy((uint32_t*)appArea, (uint32_t*)BOOTLOADER_START, PAGE_SIZE);
        memCopy((uint32_t*)appArea + 2, (uint32_t*)&flashAppIRQTable[2], 4 * (ISR_VECTOR_ITEMS - 2));
        selfProgPage0((uint32_t*)appArea);
    }
#endif
    asm volatile (
        "mov SP, %0\n"
        "bx %1\n"
        :: "r" (*flashAppStack), "r" (*flashAppReset)
    );
    NO_RETURN;
}

bool appValid;

__attribute__ ((noreturn))
void resetHandler()
{
    extern uint32_t __begin_rambss;
    extern uint32_t __size_rambss;
    int main();

    bool valid = (*flashAppReset < FLASH_SIZE); // TODO: && *flashAppReset > FLASH_APP_START

    if ((NRF_POWER->RESETREAS & ~POWER_RESETREAS_RESETPIN_Msk) && valid) {
        startFlashApp();
    }

    memZero(&__begin_rambss, (size_t)(&__size_rambss));

    appValid = valid;

    main();

    {
        uint32_t connInfo[4];
        //memcpy(&connInfo, randData.connId, 16);
        FORCE_LONG_JUMP(appEntry)(connInfo, NULL/*conf.key*/);
    }

    NO_RETURN;
}

#if INDIRECT_ISR

__attribute__((naked))
void indirectISR()
{
    asm volatile(
        "ldr r0, =" STR(FLASH_APP_START) "\n"
        "ldr r1, =" STR(ICSR_ADDRESS) "\n"
        "ldr r1, [r1]\n"
        "mov r2, #63\n"
        "and r1, r2\n"
        "lsl r1, #2\n"
        "add r0, r1\n"
        "ldr r0, [r0]\n"
        "bx r0\n"
    );
}

#endif

int main()
{
    
    return 0;
}
