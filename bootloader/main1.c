
#include <stdbool.h>


#define WAIT_FOR_CATCH_TIMEOUT 300

#define FLASH_START_ADDR    0x00000000
#define RAM_START_ADDR      0x08000000
#define SECOND_STAGE_OFFSET 1024
#define SECOND_STAGE_SIZE   3072

#define BOOTLOADER_SIZE 4096

#define IRQ_VECTOR_TABLE_SIZE (4 * 32)


typedef void (*SecondStageCall)();

static const uint32_t* const appInitialSP = ;
static const uint32_t* const appResetVect = ;

static void secondStage(uint32_t param)
{
    memcpy((void*)RAM_START_ADDR, (void*)(FLASH_START_ADDR + SECOND_STAGE_OFFSET), SECOND_STAGE_SIZE);
    *((uint32_t*)SECOND_STAGE_SIZE) = param;
    ((SecondStageCall)(RAM_START_ADDR))();
}

void _pre_data_init()
{
    if (memcmp((uint8_t*)FLASH_START_ADDR + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8) != 0)
    {
        secondStage(1);
    }

    if (resetReasonIsSwReset())
    {
        __asm__ volatile("mov SP, %1    \n"
                         "bx  %2        \n"
                         :: "r"(*(uint32_t*)BOOTLOADER_SIZE), "r"(*(uint32_t*)(BOOTLOADER_SIZE + 4)));
    }
}

int main()
{
    int i;

    radioInit();

    // Wait for `catch` packet
    timerStart();
    radioRx();
    do
    {
        radioRecv(WAIT_FOR_CATCH_TIMEOUT);
        if (timerGet() >= WAIT_FOR_CATCH_TIMEOUT)
        {
            // There is no `catch` packet, so do soft reset to start application
            deviceSoftReset();
        }
    } while (!packetIsCatch());

    secondStage(0);

    return 0;
}
