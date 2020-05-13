
#include <stdbool.h>
#include <stdint.h>


#define WAIT_FOR_RUN_BOOTLOADER_TIMEOUT 700
#define CAUGHT_DELAY_MIN                10
#define CAUGHT_DELAY_MAX                100
#define BOOTLOADER_RUNNING_DELAY        2

#define PAGE_SIZE 1024

#define FLASH_START_ADDR 0x00000000
#define SECOND_STAGE_SIZE   3072

#define FLASH_SIZE (128 * 1024)

#define BOOTLOADER_SIZE 4096

#define BLOCK_SIZE 128

#define IRQ_VECTOR_TABLE_SIZE (4 * 32)

typedef enum PacketFlags_tag
{
    ERASE_PAGES = 1,
    WRITE_BLOCK = 2,
    USE_BOOTLOADER = 4,
    RUN_APP = 8,
    GET_STATUS = 16,
    CHECK_HASH = 32,
    FLASH_BOOTLOADER = 64,
    CHECK_IRQ_TABLE = 128,
} PacketFlags;

typedef enum ResponseFlags_tag
{
    RSP_IRQ_TABLE_CHECKED = 1,
    RSP_BOOTLOADER_FLASHED = 2,
    RSP_STATUS = 4,
    RSP_ERASE_DONE = 8,
    RSP_INVALID_HASH = 16,
} ResponseFlags;


static uint32_t bootloaderCopy[BOOTLOADER_SIZE / 4];


static void blockCopy(void* dst, void* src)
{
    int i = 0;
    uint8_t* from = (uint8_t*)src;
    uint32_t* to = (uint32_t*)dst;
    for (i = 0; i < BLOCK_SIZE / 4; i++)
    {
        uint32_t a = (uint32_t)from[0] | ((uint32_t)from[1] << 8) | ((uint32_t)from[2] << 16)
            | ((uint32_t)from[3] << 24);
        *to = a;
        from += 4;
        to += 1;
    }
}

static void checkIRQTable()
{
    if (memcmp((uint8_t*)bootloaderCopy + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8) != 0)
    {
        memcpy((uint8_t*)bootloaderCopy + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8);
        const uint32_t copyPages = (IRQ_VECTOR_TABLE_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        flashErasePages(0, copyPages - 1);
        wordCopy((void*)FLASH_START_ADDR, bootloaderCopy, copyPages * PAGE_SIZE);
    }
}

int main()
{
    int i;
    uint32_t flags;
    uint32_t blockBitmap = 0;
    uint32_t blockBank = 0xFFFFFFFF;
    uint32_t firstBlock = 0xFFFFFFFF;
    uint32_t lastBlock = 0;

    memcpy(bootloaderCopy, (void*)FLASH_START_ADDR, BOOTLOADER_SIZE);

    checkIRQTable();

    if (*((uint32_t*)SECOND_STAGE_SIZE)) {
        deviceSoftReset();
    }

    // Send 10 caught packets
    for (i = 9; i >= 0; i--)
    {
        packetCaught(i);
        if (i != 0)
        {
            timerDelay(randomGetInt(CAUGHT_DELAY_MIN, CAUGHT_DELAY_MAX));
        }
    }

    // Wait for `run bootloader`
    timerStart();
    radioRx();
    do
    {
        radioRecv(WAIT_FOR_RUN_BOOTLOADER_TIMEOUT);
        if (timerGet() >= WAIT_FOR_RUN_BOOTLOADER_TIMEOUT)
        {
            // There is no `run bootloader` packet, so soft reset to start application
            deviceSoftReset();
        }
    } while (!packetIsRunBootloader());

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
                aes_sahf_calc(bootloaderCopy, BOOTLOADER_SIZE);
            }
            else if (firstBlock <= lastBlock)
            {
                aes_sahf_calc(packetData(), BLOCK_SIZE);
                aes_sahf_calc((void*)(BOOTLOADER_SIZE + firstBlock * BLOCK_SIZE),
                    (lastBlock - firstBlock + 1) * BLOCK_SIZE);
            }
            if (!aes_sahf_verify(packetHash()))
            {
                packetRspFlag(RSP_INVALID_HASH);
                continue;
            }
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
                if (block < firstBlock)
                {
                    firstBlock = block;
                }
                if (block > lastBlock)
                {
                    lastBlock = block;
                }
            }
        }

        if (flags & GET_STATUS)
        {
            packetRspFlag(RSP_STATUS);
            packetRspStatus(blockBitmap);
        }

        if (flags & FLASH_BOOTLOADER)
        {
            memcpy((uint8_t*)bootloaderCopy + 8, (uint8_t*)BOOTLOADER_SIZE + 8, IRQ_VECTOR_TABLE_SIZE - 8);
            flashErasePages(0, BOOTLOADER_SIZE / PAGE_SIZE - 1);
            wordCopy((void*)FLASH_START_ADDR, bootloaderCopy, BOOTLOADER_SIZE);
            packetRspFlag(RSP_BOOTLOADER_FLASHED);
        }

        if (flags & CHECK_IRQ_TABLE)
        {
            checkIRQTable();
            packetRspFlag(RSP_IRQ_TABLE_CHECKED);
        }

        if (flags & RUN_APP)
        {
            deviceSoftReset();
        }
    }
}
