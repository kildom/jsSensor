#include "common.h"

#define KEY 0
#define PT 16
#define CT 32

#define KEY1 0
#define PT1 16
#define CT1 32
#define KEY2 48
#define PT2 64
#define CT2 80

#define HASH_IN1 64
#define HASH_IN2 80
#define HASH_OUT 96

#define BUFFER_BYTES 112

static uint8_t aes_buffer[BUFFER_BYTES];

static void do_aes(uint8_t* data)
{
    NRF_ECB->ECBDATAPTR = (uint32_t)data;
    NRF_ECB->TASKS_STARTECB = 1;
    while (!NRF_ECB->EVENTS_ENDECB);
    NRF_ECB->EVENTS_ENDECB = 0;
}

static void do_xor(uint8_t* data, const uint8_t* source, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++)
    {
        data[i] ^= source[i];
    }
}

static void aes_dcfb_block(uint8_t* data, size_t size, uint32_t mode)
{
    uint8_t* buffer1 = &aes_buffer[KEY1 + mode];
    uint8_t* buffer2 = &aes_buffer[KEY2 - mode];
    do_aes(buffer1); // buffer1 contains key=first_key, plain=iv
    do_xor(data, &buffer1[CT], size);
    copyMem(&buffer1[PT], data, size);
    do_aes(buffer2); // buffer2 contains key=second_key, plain=iv
    do_xor(data, &buffer2[CT], size);
    copyMem(&buffer2[PT], &buffer1[PT], size);
}

EXTERN void aes_dcfb_key(const uint8_t* key)
{
    copyMem(&aes_buffer[KEY1], key, 16);
    copyMem(&aes_buffer[KEY2], &key[16], 16);
}

EXTERN void aes_dcfb(uint8_t* data, size_t size, uint32_t mode, const uint8_t* iv)
{
    copyMem(&aes_buffer[PT1], iv, 16);
    copyMem(&aes_buffer[PT2], iv, 16);
    while (size > 0)
    {
        size_t this_size = (size > 16) ? 16 : size;
        aes_dcfb_block(data, this_size, mode);
        size -= this_size;
        data += this_size;
    }
}

static void aes_hash_block(const uint8_t* data, size_t size)
{
    copyMem(&aes_buffer[HASH_IN1], data, size);
    do_xor(&aes_buffer[HASH_IN2], &aes_buffer[HASH_OUT], 16);
    do_aes(&aes_buffer[HASH_IN1]);
}

EXTERN void aes_hash(const uint8_t* data, size_t size, uint8_t* hash)
{
    zeroMem(&aes_buffer[HASH_IN1], 48);
    while (size > 0)
    {
        size_t this_size = (size > 32) ? 32 : size;
        aes_hash_block(data, this_size);
        size -= this_size;
        data += this_size;
    }
    copyMem(hash, &aes_buffer[HASH_OUT], 16);
}
