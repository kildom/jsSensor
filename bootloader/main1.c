
#include <stdbool.h>


#define WAIT_FOR_CATCH_TIMEOUT 300

#define FLASH_START_ADDR    0x00000000
#define RAM_START_ADDR      0x08000000
#define SECOND_STAGE_OFFSET 1024
#define SECOND_STAGE_SIZE   3072

#define BOOTLOADER_SIZE 4096

#define IRQ_VECTOR_TABLE_SIZE (4 * 32)

__attribute__((section(".isr_vector"))
const void* __isr_vector[IRQ_VECTOR_TABLE_SIZE / 4] = {
    (void*)RAM_END,
    (void*)&Reset_Handler,
    // 0, 0, 0, 0, ...
};

UNINITIALIZED_DATA
static bool isrTableDirty;

UNINITIALIZED_DATA
static bool appValid;
         
#define LONG_JUMP(func) ((typeof(&(func)))_LONG_JUMP_helper(&(func)))
static inline void* _LONG_JUMP_helper(void* p) {
	__asm__ volatile ("":"+r"(p));
	return p;
}

FLASH_ONLY
void Reset_Handler()
{
    extern uint32_t _ram_copy_dst;
    extern uint32_t _ram_copy_src;
    extern uint32_t _bss_start;
    extern uint32_t _bss_end;

    uint32_t* dst = &_ram_copy_dst;
    const uint32_t* src = &_ram_copy_src;

    while (dst < &_bss_start) {
        *dst++ = *src++;
    }
    while (dst < &_bss_end) {
        *dst++ = 0;
    }
    
    appValid = *(uint64_t*)BOOTLOADER_SIZE != 0 && *(uint64_t*)BOOTLOADER_SIZE != 0xFFFFFFFFFFFFFFFFuLL;
    
    isrTableDirty = (memcmp_L((uint8_t*)FLASH_START_ADDR + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8) != 0);
    
    if (appValid && !isrTableDirty && resetReasonIsSwReset())
    {
        __asm__ volatile("mov SP, %1    \n"
                         "bx  %2        \n"
                         :: "r"(*(uint32_t*)BOOTLOADER_SIZE), "r"(*(uint32_t*)(BOOTLOADER_SIZE + 4)));
    }
    
    SystemInit();
    _start();
}
        

#define timerStart_L LONG_JUMP(timerStart)
#define radioRx_L LONG_JUMP(radioRx)
#define randInRange_L LONG_JUMP(randInRange)

              
FLASH_ONLY
int main()
{
    int i;

    radioInit();

    // Wait for `catch` packet
    timerStart_L();
    radioRx_L();
    uint32_t delay = randInRange_L(TIMER_MS(1), TIMER_MS(10));
    if (!radioRecvCatch(WAIT_FOR_CATCH_TIMEOUT))
    {
        // There is no `catch` packet, so do soft reset to start application
        deviceSoftReset();
    }
    
    prepareCaughtPacket();
    bool packetSend = false;
    enum { CAUGHT_DELAY, 
    
    do {
        timerStart();
        radioRecvCatchOrBootloader(delay);
        if (radioIsRunBootloader()) {
            break;
        } else if (radioIsCatch()) {
            delay = randInRange(TIMER_MS(1), TIMER_MS(18));
            continue;
        }
        radioSendCaughtPacket();
        radioRecvCatchOrBootloader(WAIT_FOR_CATCH_TIMEOUT);
        if (radioIsRunBootloader()) {
            break;
        } else if (radioIsCatch()) {
            delay = randInRange(TIMER_MS(1), TIMER_MS(18));
            continue;
        }
        deviceSoftReset();
    } while (true);

    secondStage(0);

    return 0;
}
