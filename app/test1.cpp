
#include "nrf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common.h"
#include "worker.h"
#include "timer.h"
#include "fast_timer.h"

#include "dht22.hpp"

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


void ledSw(low::Timer* timer)
{
    led(1, 1);
}

low::Timer t1 = { ledSw };

void start1(uintptr_t* data)
{
}

Dht22 sens(1);

void start2(uintptr_t* data)
{
    t1.startInterval(SEC2TICKS(1));
    led(0, 1);
    sens.measure([]()
    {
        led(2, 1);
    });
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

    low::worker::Callback startups[2] = {start1, start2};

    low::worker::init(startups);
    fastTimerInit();

    vTaskStartScheduler();

    /*async()
    .then([]()
    {
        log("Async job");
    });

    setInterval(6, []()
    {
        dht22.masure()
        .then([]()
        {
            static int invalidCounter = 0;
            if (dht22.isValid())
            {
                outTemperature.set(dht22.getTemperature());
                outHumidity.set(dht22.getHumidity());
                invalidCounter = 0;
            }
            else
            {
                invalidCounter++;
                if (invalidCounter == 10)
                {
                    outTemperature.setInvalid();
                    outHumidity.setInvalid();
                    log::error("DHT22 problem. Reporting invalid.");
                }
            }
        });
    });*/

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
