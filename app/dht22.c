
#include <stdint.h>
#include <stdlib.h>

#include "onewire.h"
#include "dht22.h"

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
