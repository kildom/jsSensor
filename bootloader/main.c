
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
    // Rest will be initialized with:
    // 0, 0, 0, 0, ...
};

UNINITIALIZED_DATA
static bool appValid;
         
#define LONG_JUMP(func) ((typeof(&(func)))_LONG_JUMP_helper(&(func)))
static inline void* _LONG_JUMP_helper(void* p) {
	__asm__ volatile ("":"+r"(p));
	return p;
}

              
UNINITIALIZED_DATA
uint32_t shadowBuffer[BOOTLOADER_SIZE / 4];
              
bool shadowCreated = false;
              
void createShadow()
{
    if (!shadowCreated) {
        wordCopy(shadowBuffer, (void*)FLASH_START_ADDR, BOOTLOADER_SIZE);
        shadowCreated = true;
    }
}
              
void checkISRTable()
{
    if (memcmp((uint8_t*)FLASH_START_ADDR + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8) != 0) {
        createShadow();
        wordCopy((uint8_t*)bootloaderShadowCopy + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8);
        const uint32_t copyPages = (IRQ_VECTOR_TABLE_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        flashErasePages(0, copyPages - 1);
        wordCopy((void*)FLASH_START_ADDR, shadowBuffer, copyPages * PAGE_SIZE);
    }
}
              
void _start();
#define _start_L LONG_JUMP(_start);

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
    
    appValid = *(uint32_t*)BOOTLOADER_SIZE != 0
        && *(uint32_t*)(BOOTLOADER_SIZE + 4) != 0
        && *(uint32_t*)BOOTLOADER_SIZE != 0xFFFFFFFF
        && *(uint32_t*)(BOOTLOADER_SIZE + 4) != 0xFFFFFFFF;
    
    if (appValid)
    {
        checkISRTable_L();
    }
    
    if (appValid && resetReasonIsSwReset())
    {
        // TODO: lock boot region
        __asm__ volatile("mov SP, %1    \n"
                         "bx  %2        \n"
                         :: "r"(*(uint32_t*)BOOTLOADER_SIZE), "r"(*(uint32_t*)(BOOTLOADER_SIZE + 4)));
    }
    
    SystemInit();
    _start_L();
}
        

#define timerStart_L LONG_JUMP(timerStart)
#define radioRx_L LONG_JUMP(radioRx)
#define randInRange_L LONG_JUMP(randInRange)

void mainLoop()
{
    createShadow();
    
    timerDelay(BOOTLOADER_RUNNING_DELAY);
    packetBootloaderRunning();
    
    while (true)
    {
        radioSendRecv();
        
        if (packetIsRunBootloader())
        {
            timerDelay(BOOTLOADER_RUNNING_DELAY);
            packetBootloaderRunning();
            continue;
        }

        PacketFlags flags = packetFlags();

        if (flags & CHECK_HASH)
        {
            aes_sahf_init();
            if (flags & USE_BOOTLOADER)
            {
                aes_sahf_calc(shadowBuffer, BOOTLOADER_SIZE);
            }
            else
            {
		        uint32_t firstBlock = packetFirstBlock();
		        uint32_t lastBlock = packetLastBlock();
                aes_sahf_calc(packetData(), BLOCK_SIZE);
		        if (firstBlock <= lastBlock && firstBlock < FLASH_SIZE / BLOCK_SIZE && lastBlock < FLASH_SIZE / BLOCK_SIZE) {
                    aes_sahf_calc((void*)(BOOTLOADER_SIZE + firstBlock * BLOCK_SIZE),
                        (lastBlock - firstBlock + 1) * BLOCK_SIZE);
                }
            }
            if (!aes_sahf_verify(packetHash()))
            {
                packetRspFlag(RSP_INVALID_HASH);
                continue;
            }
            packetRspFlag(RSP_VALID_HASH);
        }

        if (flags & ERASE_PAGES)
        {
            flashErasePages(packetPageFrom(), packetPageTo());
            packetRspFlag(RSP_ERASE_DONE);
        }

        if (flags & WRITE_BLOCK)
        {
            uint32_t block = packetBlock();
            uint32_t bank = block >> 5;
            if (blockBank != bank)
            {
                blockBitmap = 0;
                blockBank = bank;
            }
            blockBitmap |= 1 << (block & 31);

            if (flags & USE_BOOTLOADER)
            {
                if (block < (BOOTLOADER_SIZE / BLOCK_SIZE)) {
                    blockCopy((uint8_t*)bootloaderCopy + block * BLOCK_SIZE, packetData());
                }
            }
            else if (block < (FLASH_SIZE - BOOTLOADER_SIZE) / BLOCK_SIZE)
            {
                blockCopy((uint8_t*)FLASH_START_ADDR + BOOTLOADER_SIZE + block * BLOCK_SIZE, packetData());
            }
        }

        if (flags & GET_STATUS)
        {
            packetRspFlag(RSP_STATUS);
            packetRspBank(blockBank);
            packetRspStatus(blockBitmap);
        }

        if (flags & FLASH_BOOTLOADER)
        {
            if (appValid)
                memcpy((uint8_t*)bootloaderCopy + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8);
            flashErasePages(0, BOOTLOADER_SIZE / PAGE_SIZE - 1);
            wordCopy((void*)FLASH_START_ADDR, bootloaderCopy, BOOTLOADER_SIZE);
            packetRspFlag(RSP_BOOTLOADER_FLASHED);
        }

        if (flags & CHECK_IRQ_TABLE)
        {
            checkISRTable();
            packetRspFlag(RSP_IRQ_TABLE_CHECKED);
        }

        if (flags & RUN_APP)
        {
            deviceSoftReset();
        }
    }
}

#define mainLoop_L LONG_JUMP(mainLoop)
              
FLASH_ONLY
int main()
{
    int i;
    enum { IDLE_DELAY, CATCH_DELAY } state = IDLE_DELAY;
    uint32_t longTimeout = appValid ? WAIT_FOR_CATCH_TIMEOUT : MAX_TIMEOUT;
    uint32_t timeout = longTimeout;

    radioInit();
    randInit();

    do {
        radioRecvCatchOrBootloader(timeout); // if 'run bootloader' is not prepared yet then only receive 'catch'
        if (radioIsRunBootloader()) {
            break;
        } else if (radioIsCatch()) {
            state = CATCH_DELAY;
            timeout = randInRange(TIMER_MS(1), TIMER_MS(18));
        } else if (state == CATCH_DELAY) {
            radioSendCaughtPacket(); // also prepares 'run bootloader' to compare later
            state = IDLE_DELAY;
            timeout = longTimeout;
        } else if (state == IDLE_DELAY && appValid) {
            deviceSoftReset(0);
        }
    } while (true);
    
    mainLoop_L();
    
    while(1){};
}
