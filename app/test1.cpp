
#include "nrf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "worker.h"
#include "timer.h"
#include "fast_timer.h"

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
        NRF_P0->OUTCLR = 1 << 16;
        for (i = 0; i < 1000000; i++);
        NRF_P0->OUTSET = 1 << 16;
    }
}

static void mainLoop2(void *param)
{
    volatile int i;
    while (1)
    {
        for (i = 0; i < 1000000; i++);
        NRF_P0->OUTCLR = 1 << 15;
        for (i = 0; i < 1000000; i++);
        NRF_P0->OUTSET = 1 << 15;
    }
}

void ledOff(bool* yieldRequested, FastTimer* timer)
{
    NRF_P0->OUTSET = 1 << 14;
    NRF_P0->OUTCLR = 1 << 15;
}

FastTimer t2 = { ledOff };

void ledSw(Timer* timer)
{
    NRF_P0->OUTCLR = 1 << 14;
    NRF_P0->OUTSET = 1 << 15;
    fastTimerStart(&t2, MS2FAST(1), 0);
}

Timer t1 = { ledSw };

void WorkerStart1(uintptr_t* data)
{
    //timerStart(&t1, 250, 250 | TIMER_FLAG_REPEAT);
    //mainLoop2(0);
}

void WorkerStart2(uintptr_t* data)
{
    timerStart(&t1, SEC2TICKS(1), SEC2TICKS(1) | TIMER_FLAG_REPEAT);
    //mainLoop2(0);
}

int main()
{
    NRF_P0->PIN_CNF[13] = 1;
    NRF_P0->PIN_CNF[14] = 1;
    NRF_P0->PIN_CNF[15] = 1;
    NRF_P0->PIN_CNF[16] = 1;

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

    workerInit(WorkerStart1, WorkerStart2);
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
