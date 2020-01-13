
#include "nrf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "worker.h"
#include "timer.h"
#include "fast_timer.h"

#ifndef NRF_P0
#define NRF_P0 NRF_GPIO
#endif

void led(int num, int state)
{
    if (state)
        NRF_P0->OUTCLR = 1 << (21 + num);
    else
        NRF_P0->OUTSET = 1 << (21 + num);
}

uint8_t staticQueueBuffer1[128];
uint8_t staticQueueBuffer2[128];

QueueHandle_t q1;
QueueHandle_t q2;

static void mainLoop1(void *param)
{
    volatile int i;
    while (1)
    {
        for (i = 0; i < 1000000; i++);
        NRF_P0->OUTCLR = 1 << 21;
        for (i = 0; i < 1000000; i++);
        NRF_P0->OUTSET = 1 << 21;
    }
}

static void mainLoop2(void *param)
{
    volatile int i;
    while (1)
    {
        for (i = 0; i < 1000000; i++);
        NRF_P0->OUTCLR = 1 << 22;
        for (i = 0; i < 1000000; i++);
        NRF_P0->OUTSET = 1 << 22;
    }
}

void ledOff(bool* yieldRequested, FastTimer* timer)
{
    NRF_P0->OUTSET = 1 << 21;
    NRF_P0->OUTCLR = 1 << 22;
}

FastTimer t2 = { ledOff };

void ledSw(low::Timer* timer)
{
    led(1, 1);
    //fastTimerStart(&t2, MS2FAST(1), 0);
}

low::Timer t1 = { ledSw };

void WorkerStart1(uintptr_t* data)
{
    //timerStart(&t1, 250, 250 | TIMER_FLAG_REPEAT);
    //mainLoop2(0);
}

void WorkerStart2(uintptr_t* data)
{
    timerStart(&t1, SEC2TICKS(1), SEC2TICKS(1) | low::TIMER_FLAG_REPEAT);
    //mainLoop2(0);
    led(0, 1);
}

int main()
{
    NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSTAT_SRC_Xtal << CLOCK_LFCLKSTAT_SRC_Pos;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    NRF_P0->PIN_CNF[21] = 1;
    NRF_P0->PIN_CNF[22] = 1;
    NRF_P0->PIN_CNF[23] = 1;
    NRF_P0->PIN_CNF[24] = 1;
    led(0, 0);
    led(1, 0);
    led(2, 0);
    led(3, 0);

    /*static StaticQueue_t sq1;
    q1 = xQueueCreateStatic(ARRAY_LENGTH(staticQueueBuffer1),
        sizeof(staticQueueBuffer1[0]),
        (uint8_t*)staticQueueBuffer1, &sq1);

    static StaticQueue_t sq2;
    q2 = xQueueCreateStatic(ARRAY_LENGTH(staticQueueBuffer2),
        sizeof(staticQueueBuffer2[0]),
        (uint8_t*)staticQueueBuffer2, &sq2);

    static StaticTask_t st1;
    static StackType_t stack1[256];
    TaskHandle_t t1 = xTaskCreateStatic(mainLoop1, "1", ARRAY_LENGTH(stack1),
        NULL, WORKER_LOW_PRIORITY, stack1, &st1);

    static StaticTask_t st2;
    static StackType_t stack2[256];
    TaskHandle_t t2 = xTaskCreateStatic(mainLoop2, "2", ARRAY_LENGTH(stack2),
        NULL, WORKER_HIGH_PRIORITY, stack2, &st2);*/

    low::WorkerCallback startups[2] = {WorkerStart1, WorkerStart2};

    low::workerInit(startups);
    fastTimerInit();

    vTaskStartScheduler();

    return 0;
}

extern "C"
void vApplicationIdleHook( void )
{
}

extern "C"
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
    static StaticTask_t data;
    static StackType_t stack[128];
    *ppxIdleTaskTCBBuffer = &data;
    *ppxIdleTaskStackBuffer = stack;
    *pulIdleTaskStackSize = sizeof(stack) / sizeof(stack[0]);
}
