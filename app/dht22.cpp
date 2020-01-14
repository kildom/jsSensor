
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "worker.h"
#include "onewire.h"
#include "dht22.hpp"
#include "promise.hpp"
#include "fifo.hpp"

struct QueueItem
{
    Dht22* dht22;
    const std::function<void()>* callback;
};

Fifo<QueueItem, DHT22_MEASURE_QUEUE_SIZE> fifo;

void dht22_read(uint32_t pinNumber)
{
    ow_read(pinNumber);
}

void ow_read_result(uint8_t* buffer)
{
    uint8_t sum;
    if (buffer[0] + buffer[1] + buffer[2] + buffer[3] != buffer[4]
        || buffer[0] > 100
        || buffer[1] > 9
        || (int8_t)buffer[2] > 100 || (int8_t)buffer[2] < -60
        || buffer[3] > 9)
    {
        dht22_read_result(0, (int16_t)0x8000);
        return;
    }
    dht22_read_result((uint16_t)buffer[0] * 10 + (uint16_t)buffer[1], (int16_t)buffer[2] * 10 + (int16_t)buffer[3]);
}

void dht22_read_result(uint16_t rh, uint16_t temp)
{

}

Dht22::Dht22(uint32_t pinNumber) : pinNumber(pinNumber), valid(0)
{
}

bool running = false;

static void start(uint32_t pinNumber)
{

}


static float bcd2float(uint32_t x)
{
    return (float)(x >> 8) + (float)(x & 0xFF) * 0.01f;
}


void Dht22::done(uintptr_t* data)
{
    TASK_HIGH;
    QueueItem &item = fifo.pop();
    const std::function<void()>* callback = item.callback;
    Dht22* dht22 = item.dht22;
    dht22->valid = data[0];
    if (dht22->valid)
    {
        dht22->temperature = bcd2float(data[1]);
        dht22->humidity = bcd2float(data[2]);
    }
    (*callback)();
    delete callback;
    running = false;
    processQueue(data);
}

static void processQueue(uintptr_t*)
{
    TASK_HIGH;
    if (!running && fifo.length() > 0)
    {
        running = true;
        start(fifo.peek().dht22->getPinNumber());
    }
}

void Dht22::measure(const std::function<void()> &callback)
{
    TASK_HIGH;
    QueueItem item;
    item.dht22 = this;
    item.callback = new std::function<void()>(callback);
    fifo.push(item);
    low::worker::addToHigh(processQueue, 0);
}
