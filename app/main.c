
#include "nrf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "worker.h"
#include "timer.h"



static QueueHandle_t xQueue = NULL;

static void taskA( void *pvParameters )
{
    uint8_t data;
    volatile int i;
    TickType_t xNextWakeTime;

    while (1)
    {
		xQueueReceive( xQueue, &data, portMAX_DELAY );
        NRF_GPIO->OUTCLR = (1 << 21);
       	xNextWakeTime = xTaskGetTickCount();
        vTaskDelayUntil( &xNextWakeTime, MS2TICKS(200));
        NRF_GPIO->OUTSET = (1 << 21);
    }
}


void ledBlink(uintptr_t* data)
{
    TickType_t xNextWakeTime;
    NRF_GPIO->OUTCLR = (1 << 24);
   	xNextWakeTime = xTaskGetTickCount();
    vTaskDelayUntil( &xNextWakeTime, MS2TICKS(200));
    NRF_GPIO->OUTSET = (1 << 24);
}

static void taskB( void *pvParameters )
{
    uint8_t data = 0;
    volatile int i;
    TickType_t xNextWakeTime;
   	xNextWakeTime = xTaskGetTickCount();

    while (1)
    {
        xQueueSend(xQueue, &data, portMAX_DELAY);
        vTaskDelayUntil( &xNextWakeTime, MS2TICKS(500));
        workerSend(ledBlink, 0);
        vTaskDelayUntil( &xNextWakeTime, MS2TICKS(500));
    }
}


void vApplicationIdleHook( void )
{
}

void ledBlink2(Timer* timer);

struct LedTimer
{
    Timer timer;
    uint8_t data;
} ledTimer = { { ledBlink2 }, 0 };

void ledBlink2(Timer* timer)
{
    struct LedTimer* t = (struct LedTimer*)timer;
    if (t->data)
    {
        NRF_GPIO->OUTCLR = (1 << 23);
        timerStart(timer, timer->fireTime + MS2TICKS(200), TIMER_FLAG_ABSOLUTE);
    }
    else
    {
        NRF_GPIO->OUTSET = (1 << 23);
        timerStart(timer, timer->fireTime + MS2TICKS(800), TIMER_FLAG_ABSOLUTE);
    }
    t->data = !t->data;
}


void test1(Timer* timer)
{
}

Timer timer2 = { test1 };

void workerStartup(uintptr_t* data)
{
    timerStart(&ledTimer.timer, SEC2TICKS(1), 0);
    //timerStart(&timer2, 0, MS2TICKS(10) | TIMER_FLAG_REPEAT);
}

int main()
{
    NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSTAT_SRC_Xtal << CLOCK_LFCLKSTAT_SRC_Pos;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    NRF_GPIO->PIN_CNF[21] = 1;
    NRF_GPIO->PIN_CNF[22] = 1;
    NRF_GPIO->PIN_CNF[23] = 1;
    NRF_GPIO->PIN_CNF[24] = 1;

    NRF_GPIO->OUTSET = (1 << 21);
    NRF_GPIO->OUTCLR = (1 << 22);
    NRF_GPIO->OUTSET = (1 << 23);
    NRF_GPIO->OUTCLR = (1 << 24);
    
    static uint8_t queueBuffer[16 * 1];
    static StaticQueue_t queueData;
    xQueue = xQueueCreateStatic(ARRAY_LENGTH(queueBuffer), sizeof(queueBuffer[0]), queueBuffer, &queueData);

    static StackType_t stackA[128];
    static StackType_t stackB[128];
    static StaticTask_t dataA;
    static StaticTask_t dataB;

    xTaskCreateStatic(taskA, "A", ARRAY_LENGTH(stackA), NULL, tskIDLE_PRIORITY + 3, stackA, &dataA);
    xTaskCreateStatic(taskB, "B", ARRAY_LENGTH(stackB), NULL, tskIDLE_PRIORITY + 2, stackB, &dataB);

    workerInit(workerStartup);

    vTaskStartScheduler();

    return 0;
}

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
    static StaticTask_t data;
    static StackType_t stack[128];
    *ppxIdleTaskTCBBuffer = &data;
    *ppxIdleTaskStackBuffer = stack;
    *pulIdleTaskStackSize = sizeof(stack) / sizeof(stack[0]);
}

#if 0

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "nrf.h"

uint8_t bufrx[192];
uint32_t buftx[192];

volatile int x;

#define OW_EVENT_STARTUP_DONE 1

#define OW_MOSI 27
#define OW_SCK 22
#define OW_CSN 23
#define OW_SCK_LOOPBACK 20
#define OW_CSN_LOOPBACK 24
#define OW_TIMER NRF_TIMER0
#define OW_TIMER_HANDLER TIMER0_IRQHandler
#define OW_SPIS NRF_SPIS0
#define OW_PPI_CHANNEL 0
#define OW_GPIOTE_CHANNEL 0

uint8_t ow_buffer[90]; // 90 bytes => 720 bits => 7.2 ms
size_t ow_samples;

void ow_init()
{
    // Setup PINS
    NRF_P0->PIN_CNF[OW_MOSI] = 0;
    NRF_P0->PIN_CNF[OW_SCK] = 0;
    NRF_P0->PIN_CNF[OW_CSN] = 0;
    NRF_P0->PIN_CNF[OW_SCK_LOOPBACK] = 1;
    NRF_P0->PIN_CNF[OW_CSN_LOOPBACK] = 1;
    NRF_P0->OUTCLR = (1 << OW_SCK_LOOPBACK);
    NRF_P0->OUTSET = (1 << OW_CSN_LOOPBACK);

    // Configure GPIOTE and PPI to output 50% PWM based on TIMER
    NRF_GPIOTE->CONFIG[OW_GPIOTE_CHANNEL] =
        (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos)|
        (OW_SCK_LOOPBACK << GPIOTE_CONFIG_PSEL_Pos) |
        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
    NRF_PPI->CH[OW_PPI_CHANNEL].EEP = (uint32_t)&OW_TIMER->EVENTS_COMPARE[0];
    NRF_PPI->CH[OW_PPI_CHANNEL].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[OW_GPIOTE_CHANNEL];

    // Configure SPI Slave
    OW_SPIS->PSELSCK = OW_SCK;
    OW_SPIS->PSELMOSI = OW_MOSI;
    OW_SPIS->PSELCSN = OW_CSN;
    OW_SPIS->CONFIG = 0;
}

void ow_start_sampling()
{
    // Setup TIMER for 100kbit SPI clock
    OW_TIMER->BITMODE = 1;
    OW_TIMER->PRESCALER = 4; // 1MHz => 1µs / step
    OW_TIMER->CC[0] = 5;     // 200kHz => 5µs / half-period => 10µs / SPI bit
    OW_TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);

    OW_SPIS->ENABLE = SPIS_ENABLE_ENABLE_Enabled;

    OW_SPIS->TASKS_ACQUIRE = 1;
    while (!OW_SPIS->EVENTS_ACQUIRED);
    OW_SPIS->EVENTS_ACQUIRED = 0;
    OW_SPIS->EVENTS_END = 0;
    OW_SPIS->RXDPTR = (uint32_t)ow_buffer;
    OW_SPIS->MAXRX = sizeof(ow_buffer);
    OW_SPIS->TXDPTR = 0;
    OW_SPIS->MAXTX = 0;
    OW_SPIS->TASKS_RELEASE = 1;

    NRF_P0->OUTCLR = (1 << OW_CSN_LOOPBACK);

    NRF_PPI->CHENSET = 1 << OW_PPI_CHANNEL;
    OW_TIMER->TASKS_CLEAR = 1;
    OW_TIMER->TASKS_START = 1;
}

bool ow_stop_sampling()
{
    volatile uint8_t timeout = 255;
    OW_TIMER->TASKS_STOP = 1;
    NRF_PPI->CHENCLR = 1 << OW_PPI_CHANNEL;
    NRF_P0->OUTSET = (1 << OW_CSN_LOOPBACK);
    NRF_P0->OUTCLR = (1 << OW_SCK_LOOPBACK);
    while (!OW_SPIS->EVENTS_END)
    {
        if (!(timeout--)) return false;
    }
    ow_samples = OW_SPIS->AMOUNTRX;
    OW_SPIS->ENABLE = SPIS_ENABLE_ENABLE_Disabled;
    return ow_samples > 24;
}

void send_event(uint32_t event)
{

}

void invoke(void (*callback)(void*), void* data)
{

}

uint32_t pop_event()
{

}

void ow_stop_init_signal(void* data)
{
    /*
    TODO:
    - stop wat was started in ow_start 
    - call ow_start_sampling
    - call ow_stop_sampling() and finalize after 6 ms
     */
}

void OW_TIMER_HANDLER()
{
    if (OW_TIMER->EVENTS_COMPARE[0])
    {
        OW_TIMER->EVENTS_COMPARE[0] = 0;
        invoke(ow_stop_init_signal, NULL);
    }
}

void ow_start()
{
    OW_TIMER->BITMODE = 1;
    OW_TIMER->PRESCALER = 7; // 16MHz / 2^7 = 125kHz => 8µs
    OW_TIMER->CC[0] = 150;   // 8µs * 150 = 1.2ms
    OW_TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos);
    NRF_P0->OUTCLR = (1 << OW_MOSI);
    NRF_P0->PIN_CNF[OW_MOSI] = 1;
    OW_TIMER->INTENSET = (TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos);
    OW_TIMER->TASKS_CLEAR = 1;
    OW_TIMER->TASKS_START = 1;
}

int main()
{
    volatile size_t x;

    __enable_irq();
    NVIC_EnableIRQ(TIMER0_IRQn);

    NRF_P0->PIN_CNF[17] = 1;
    NRF_P0->PIN_CNF[18] = 1;
    NRF_P0->PIN_CNF[19] = 1;

    NRF_P0->OUTSET = (1 << 17);
    NRF_P0->OUTSET = (1 << 18);
    NRF_P0->OUTSET = (1 << 19);

    ow_init();
    ow_start();
    while(1);
    ow_start_sampling();
    for (x = 0; x < 20000; x++);
    ow_stop_sampling();
    while(1);
}


int main_old()
{
    int i, k;

    NRF_TIMER0->BITMODE = 3;
    NRF_TIMER0->PRESCALER = 9;
    NRF_TIMER0->CC[0] = 4;
    NRF_TIMER0->CC[2] = 1;
    NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Enabled;
    NRF_TIMER0->TASKS_CLEAR = 1;
    NRF_TIMER0->TASKS_START = 1;

    uint32_t old = -1;
    uint32_t now;
    uint32_t cnt;
    for (i = 0; i < 100; i++)
    {
        cnt = 0;
        do {
            NRF_TIMER0->TASKS_CAPTURE[1] = 1;
            now = NRF_TIMER0->CC[1];
            cnt++;
        } while (now == old);
        buftx[i] = cnt;
        old = now;
    }


    NRF_P0->PIN_CNF[17] = 1;
    NRF_P0->PIN_CNF[18] = 1;
    NRF_P0->PIN_CNF[19] = 1;
    NRF_P0->PIN_CNF[20] = 1;
    NRF_P0->PIN_CNF[22] = 0;
    NRF_P0->PIN_CNF[23] = 0;
    NRF_P0->PIN_CNF[24] = 1;
    NRF_P0->PIN_CNF[27] = 0;

    NRF_P0->OUTSET = (1 << 24);

    NRF_GPIOTE->CONFIG[0] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) | (20 << GPIOTE_CONFIG_PSEL_Pos) | (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[2];
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
    NRF_PPI->CHENSET = 1 << 0;

    NRF_SPIS0->PSELSCK = 22;
    NRF_SPIS0->PSELMOSI = 27;
    NRF_SPIS0->PSELCSN = 23;
    NRF_SPIS0->CONFIG = 0;
    NRF_SPIS0->ENABLE = SPIS_ENABLE_ENABLE_Enabled;

    NRF_SPIS0->TASKS_ACQUIRE = 1;
    while (!NRF_SPIS0->EVENTS_ACQUIRED);
    NRF_SPIS0->EVENTS_ACQUIRED = 0;
    NRF_SPIS0->RXDPTR = (uint32_t)bufrx;
    NRF_SPIS0->MAXRX = sizeof(bufrx);
    NRF_SPIS0->TXDPTR = (uint32_t)buftx;
    NRF_SPIS0->MAXTX = 0;
    NRF_SPIS0->TASKS_RELEASE = 1;

    NRF_SPIS0->EVENTS_END = 0;

    for (x = 0; x < 100000; x++);

    NRF_P0->OUTCLR = (1 << 24);

    for (i = 0; i < 64; i++)
    {
        for (x = 0; x < 100000; x++);
        for (x = 0; x < 100000; x++);
    }
    
    NRF_P0->OUTSET = (1 << 24);

    do {
        i = NRF_SPIS0->EVENTS_END;
    } while (!i);

    i = NRF_SPIS0->AMOUNTRX;

    NRF_SPIS0->TASKS_ACQUIRE = 1;
    while (!NRF_SPIS0->EVENTS_ACQUIRED);
    NRF_SPIS0->EVENTS_ACQUIRED = 0;
    i = NRF_SPIS0->AMOUNTRX;
    NRF_SPIS0->TASKS_RELEASE = 1;
    while (i)
    {
        NRF_P0->OUTCLR = (1 << 19);
        for (x = 0; x < 100000; x++);
        NRF_P0->OUTSET = (1 << 19);
        for (x = 0; x < 1000000; x++);
        i--;
    }
    
    NRF_P0->OUTSET = (1 << 17);
    NRF_P0->OUTCLR = (1 << 18);

    while(1);

    NRF_P0->PIN_CNF[20] = 1;
    NRF_PPI->CHENSET = 1 << 0;

    while(1)
    {
        if (NRF_P0->IN & (1 << 22))
        {
            NRF_P0->OUTCLR = (1 << 17);
        }
        else
        {
            NRF_P0->OUTSET = (1 << 17);            
        }
        
    }
}

#endif
