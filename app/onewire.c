
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "nrf.h"
#include "FreeRTOS.h"
#include "task.h"

#include "config.h"
#include "common.h"
#include "timer.h"
#include "worker.h"
#include "onewire.h"

static uint8_t buffer[128]; // 128 bytes => 1024 bits => 10.24 ms
static size_t samples;
static bool running;
static uint32_t currentPinNumber;
static Timer timer;

void ow_init()
{
    // Setup PINS
    NRF_GPIO->PIN_CNF[OW_SCK] = 0;
    NRF_GPIO->PIN_CNF[OW_CSN] = 0;
    NRF_GPIO->PIN_CNF[OW_SCK_LOOPBACK] = 1;
    NRF_GPIO->PIN_CNF[OW_CSN_LOOPBACK] = 1;
    NRF_GPIO->OUTCLR = (1 << OW_SCK_LOOPBACK);
    NRF_GPIO->OUTSET = (1 << OW_CSN_LOOPBACK);

    // Configure GPIOTE and PPI to output 50% PWM based on TIMER
    NRF_GPIOTE->CONFIG[OW_GPIOTE_CHANNEL] =
        (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos)|
        (OW_SCK_LOOPBACK << GPIOTE_CONFIG_PSEL_Pos) |
        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
    NRF_PPI->CH[OW_PPI_CHANNEL].EEP = (uint32_t)&OW_TIMER->EVENTS_COMPARE[0];
    NRF_PPI->CH[OW_PPI_CHANNEL].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[OW_GPIOTE_CHANNEL];

    // Configure SPI Slave
    OW_SPIS->PSELSCK = OW_SCK;
    OW_SPIS->PSELCSN = OW_CSN;
    OW_SPIS->CONFIG = 0;

    // Set current state
    running = false;
}

static int nextByte;
static uint8_t shiftingByte;
static uint8_t bits;
static bool bitsError;

#define BITS_LO 0x00
#define BITS_HI 0x80

static int getBits(uint8_t bit, int min, int max)
{
    int result = 0;
    if (bitsError) return 0;
    while (true)
    {
        if (bits == 0)
        {
            if (nextByte >= samples)
            {
                break;
            }
            shiftingByte = buffer[nextByte];
            nextByte++;
            bits = 8;
        }
        if ((shiftingByte & 0x80) == bit)
        {
            result++;
            shiftingByte <<= 1;
            bits--;
        }
        else
        {
            break;
        }
    }
    bitsError = (result < min) || (result > max);
    return result;
}

static void parseSamples()
{
    int i;
    int j;
    nextByte = 0;
    bits = 0;
    bitsError = false;
    getBits(BITS_LO, 0, 10); // skip low bits if there some left from start signal
    getBits(BITS_HI, 1, 10); // wait for sensor response
    getBits(BITS_LO, 2, 16); // start transmission LO
    getBits(BITS_HI, 2, 16); // start transmission HI
    for (i = 0; i < 5 && !bitsError; i++)
    {
        uint8_t dataByte = 0;
        for (j = 0; j < 8; j++)
        {
            getBits(BITS_LO, 2, 8);
            int len = getBits(BITS_HI, 1, 10);
            if (len >= 5) dataByte |= 1;
            dataByte <<= 1;
        }
        buffer[i] = dataByte;
    }
    running = false;
    ow_read_result(bitsError ? NULL : buffer);
}

static void stopSampling(Timer* t)
{
    volatile uint8_t timeout = 255;
    OW_TIMER->TASKS_STOP = 1;
    NRF_PPI->CHENCLR = 1 << OW_PPI_CHANNEL;
    NRF_GPIO->OUTSET = (1 << OW_CSN_LOOPBACK);
    NRF_GPIO->OUTCLR = (1 << OW_SCK_LOOPBACK);
    while (!OW_SPIS->EVENTS_END)
    {
        if (!(timeout--))
        {
            running = false;
            ow_read_result(NULL);
            return;
        }
    }
    samples = OW_SPIS->AMOUNTRX;
    OW_SPIS->ENABLE = SPIS_ENABLE_ENABLE_Disabled;
    parseSamples();
}

static void startSampling(Timer* t)
{

    // Setup TIMER for 100kbit SPI clock
    OW_TIMER->BITMODE = 1;
    OW_TIMER->PRESCALER = 4; // 1MHz => 1µs / step
    OW_TIMER->CC[0] = 5;     // 200kHz => 5µs / half-period => 10µs / SPI bit
    OW_TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);

    taskENTER_CRITICAL();

    OW_SPIS->ENABLE = SPIS_ENABLE_ENABLE_Enabled;

    OW_SPIS->TASKS_ACQUIRE = 1;
    while (!OW_SPIS->EVENTS_ACQUIRED); // TODO: check if not too long for critical section
    OW_SPIS->EVENTS_ACQUIRED = 0;
    OW_SPIS->EVENTS_END = 0;
    OW_SPIS->RXDPTR = (uint32_t)buffer;
    OW_SPIS->MAXRX = sizeof(buffer);
    OW_SPIS->TXDPTR = 0;
    OW_SPIS->MAXTX = 0;
    OW_SPIS->TASKS_RELEASE = 1;

    NRF_GPIO->OUTCLR = (1 << OW_CSN_LOOPBACK);

    NRF_PPI->CHENSET = 1 << OW_PPI_CHANNEL;
    OW_TIMER->TASKS_CLEAR = 1;
    OW_TIMER->TASKS_START = 1;

    taskEXIT_CRITICAL();
    
    NRF_GPIO->DIRCLR = (1 << currentPinNumber);

    timer.callback = stopSampling;
    timerStart(&timer, MS2TICKS(8), 0);
}


void ow_read(uint32_t pinNumber)
{
    PROD_ASSERT(!running, "1-wire read already running!");

    running = true;
    currentPinNumber = pinNumber;
    NRF_GPIO->PIN_CNF[currentPinNumber] = 0;
    OW_SPIS->PSELMOSI = currentPinNumber;
    
    NRF_GPIO->OUTCLR = (1 << currentPinNumber);
    NRF_GPIO->DIRSET = (1 << currentPinNumber);

    timer.callback = startSampling;
    timerStart(&timer, MS2TICKS(2), 0);
}
