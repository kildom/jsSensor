
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "nrf.h"
#include "FreeRTOS.h"
#include "task.h"

#include "config.h"
#include "common.h"
#include "timer.h"
#include "fast_timer.h"
#include "worker.h"
#include "dht22.hpp"
#include "fifo.hpp"

using namespace low;

namespace _internal_Dht22 {

static const size_t BUFFER_SIZE = 128; // 128 bytes => 1024 bits => 10.24 ms

static const uint8_t BITS_LO = 0x00;
static const uint8_t BITS_HI = 0x80;

struct QueueItem
{
    QueueItem(Dht22* dht22, const std::function<void()> &callback) : dht22(dht22), callback(callback) { }
    Dht22* dht22;
    std::function<void()> callback;
};

static bool initialized = false;
static Fifo<QueueItem*, DHT22_MEASURE_QUEUE_SIZE> fifo;

static uint8_t* buffer = NULL;
static size_t samples;
static uint32_t currentPinNumber;
static FastTimer fastTimer;

static int nextByte;
static uint8_t shiftingByte;
static uint8_t bits;
static bool bitsError;

static void init()
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

    // Setup TIMER for 100kbit SPI clock
    OW_TIMER->BITMODE = 1;
    OW_TIMER->PRESCALER = 4; // 1MHz => 1µs / step
    OW_TIMER->CC[0] = 5;     // 200kHz => 5µs / half-period => 10µs / SPI bit
    OW_TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
}

static int getBits(uint8_t bit, int min, int max)
{
    TASK_HIGH;

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
    TASK_HIGH;

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
            dataByte <<= 1;
            if (len >= 5) dataByte |= 1;
        }
        buffer[i] = dataByte;
    }
    uint8_t checksum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    if (bitsError || checksum != buffer[4])
    {
        if (bitsError)
        {
            LOG_WARNING("DHT22 transmission error on bits level!");
        }
        else
        {
            LOG_WARNING("DHT22 transmission checksum error!");
        }
        worker::addToHigh(done, 3, 0, 0, 0);
        return;
    }
    uintptr_t h = ((uintptr_t)buffer[0] << 8) + (uintptr_t)buffer[1];
    uintptr_t t = ((uintptr_t)buffer[2] << 8) + (uintptr_t)buffer[3];
    worker::addToHigh(done, 3, 1, h, t);
}

static void finalizeAndParse(uintptr_t*)
{
    TASK_HIGH;

    volatile uint8_t timeout = 255;
    NRF_GPIO->OUTCLR = (1 << OW_SCK_LOOPBACK);
    while (!OW_SPIS->EVENTS_END)
    {
        if (!(timeout--))
        {
            // CSN loopback may be broken if this happens
            LOG_WARNING("DHT22 SPIS disable timeout!");
            worker::addToHigh(done, 3, 0, 0, 0);
            return;
        }
    }
    samples = OW_SPIS->AMOUNTRX;
    OW_SPIS->ENABLE = SPIS_ENABLE_ENABLE_Disabled;
    parseSamples();
}


static void stopSampling(bool* yield, FastTimer* t)
{
    TASK_IRQ;

    OW_TIMER->TASKS_STOP = 1;
    NRF_PPI->CHENCLR = 1 << OW_PPI_CHANNEL;
    NRF_GPIO->OUTSET = 1 << OW_CSN_LOOPBACK;
    worker::addToHighFromISR(yield, finalizeAndParse, 0);
}


static void startSampling(bool* yield, FastTimer*)
{
    TASK_IRQ;

    BaseType_t saved;

    saved = taskENTER_CRITICAL_FROM_ISR(); // TODO: Check why this critical section is so long

    OW_SPIS->ENABLE = SPIS_ENABLE_ENABLE_Enabled;

    OW_SPIS->TASKS_ACQUIRE = 1;
    while (!OW_SPIS->EVENTS_ACQUIRED); // TODO: check if not too long for interrupt
    OW_SPIS->EVENTS_ACQUIRED = 0;
    OW_SPIS->EVENTS_END = 0;
    OW_SPIS->RXDPTR = (uint32_t)buffer;
    OW_SPIS->MAXRX = BUFFER_SIZE;
    OW_SPIS->TXDPTR = 0;
    OW_SPIS->MAXTX = 0;
    OW_SPIS->TASKS_RELEASE = 1;

    NRF_GPIO->OUTCLR = (1 << OW_CSN_LOOPBACK);

    NRF_PPI->CHENSET = 1 << OW_PPI_CHANNEL;
    OW_TIMER->TASKS_CLEAR = 1;
    OW_TIMER->TASKS_START = 1;

    taskEXIT_CRITICAL_FROM_ISR(saved);
    
    NRF_GPIO->DIRCLR = (1 << currentPinNumber);

    fastTimer.callback = stopSampling;
    fastTimerStart(&fastTimer, MS2FAST(8), 0);
}

static void start(uintptr_t* data)
{
    TASK_LOW;

    if (!initialized)
    {
        initialized = true;
        init();
    }

    currentPinNumber = data[0];
    NRF_GPIO->PIN_CNF[currentPinNumber] = 0;
    OW_SPIS->PSELMOSI = currentPinNumber;
    
    NRF_GPIO->OUTCLR = (1 << currentPinNumber);
    NRF_GPIO->DIRSET = (1 << currentPinNumber);

    fastTimer.callback = startSampling;
    fastTimerStart(&fastTimer, MS2FAST(2), 0);
}


static float dht22ToFloat(uint32_t x)
{
    TASK_HIGH;
    float f = (float)(x & 0x7FF) * 0.1f;
    if (x & 0x8000) f = -f;
    return f;
}


static void done(uintptr_t* data)
{
    TASK_HIGH;
    QueueItem* item = fifo.pop();
    Dht22* dht22 = item->dht22;
    dht22->valid = data[0];
    if (dht22->valid)
    {
        dht22->humidity = dht22ToFloat(data[1]);
        dht22->temperature = dht22ToFloat(data[2]);
    }
    item->callback();
    delete item;

    if (fifo.length() > 0)
    {
        worker::addToLow(start, 1, (uintptr_t)fifo.peek()->dht22->pinNumber);
    }
    else
    {
        delete[] buffer;
        buffer = NULL;
    }
}

}; // namespace _internal_Dht22

using namespace _internal_Dht22;

Dht22::Dht22(uint32_t pinNumber) : pinNumber(pinNumber), valid(0)
{
    TASK_ANY;
}

void Dht22::measure(const std::function<void()> &callback)
{
    TASK_HIGH;
    QueueItem* item = new QueueItem(this, callback);
    fifo.push(item);
    if (buffer == NULL)
    {
        buffer = new uint8_t[BUFFER_SIZE];
        worker::addToLow(start, 1, (uintptr_t)fifo.peek()->dht22->pinNumber);
    }
}
