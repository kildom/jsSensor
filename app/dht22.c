
#include <stdint.h>
#include <stdlib.h>


void dht22_read(uint32_t pinNumber)
{
    onewire_read(pinNumber);
}

void ow_read_result(uint8_t* buffer, size_t bits)
{
    uint8_t sum;
    if (bits < 40
        || buffer[0] + buffer[1] + buffer[2] + buffer[3] != buffer[4]
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
