#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aes.h"


#define KEY 0
#define PT  16
#define CT  32

#define KEY1 0
#define PT1  16
#define CT1  32
#define KEY2 48
#define PT2  64
#define CT2  80

#define HASH_IN1 64
#define HASH_IN2 80
#define HASH_OUT 96

#define BUFFER_BYTES 112


static uint8_t aes_buffer[BUFFER_BYTES];


static void do_aes(uint8_t* data)
{
    ECB->ECBDATAPTR = (uint32_t)data;
    ECB->TASKS_STARTECB = 1;
    while (!ECB->EVENTS_ENDECB)
    {
        // just wait
    }
    ECB->EVENTS_ENDECB = 0;
}


static void do_xor(uint8_t* data, const uint8_t* source, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++)
    {
        data[i] ^= source[i];
    }
}


static void aes_dctf_block(uint8_t* data, size_t size, uint32_t mode)
{
    uint8_t* buffer1 = &aes_buffer[KEY1 + mode];
    uint8_t* buffer2 = &aes_buffer[KEY2 - mode];
    do_aes(buffer1); // buffer1 contains key=first_key, plain=iv
    do_xor(data, &buffer1[CT], size);
    memcpy(&buffer1[PT], data, size);
    do_aes(buffer2); // buffer2 contains key=second_key, plain=iv
    do_xor(data, &buffer2[CT], size);
    memcpy(&buffer2[PT], &buffer1[PT], size);
}


void aes_dctf_key(const uint8_t* key)
{
    memcpy(&aes_buffer[KEY1], key, 16);
    memcpy(&aes_buffer[KEY2], &key[16], 16);
}


void aes_dctf(uint8_t* data, size_t size, uint32_t mode, const uint8_t* iv)
{
    memcpy(&aes_buffer[PT1], iv, 16);
    memcpy(&aes_buffer[PT2], iv, 16);
    while (size > 0)
    {
        size_t this_size = (size > 16) ? 16 : size;
        aes_dctf_block(data, this_size, mode);
        size -= this_size;
        data += this_size;
    }
}


static void aes_sahf_block(const uint8_t* data, size_t size)
{
    memcpy(&aes_buffer[HASH_IN1], data, size);
    do_xor(&aes_buffer[HASH_IN2], &aes_buffer[HASH_OUT], 16);
    do_aes(&aes_buffer[HASH_IN1]);
}


void aes_sahf(const uint8_t* data, size_t size, uint8_t* hash)
{
    memset(&aes_buffer[HASH_IN1], 0, 48);
    while (size > 0)
    {
        size_t this_size = (size > 32) ? 32 : size;
        aes_sahf_block(data, this_size);
        size -= this_size;
        data += this_size;
    }
    memcpy(hash, &aes_buffer[HASH_OUT], 16);
}
