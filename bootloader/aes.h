#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>
#include <stdlib.h>

#define AES_DCTF_ENCRYPT 0
#define AES_DCTF_DECRYPT 48

void aes_dctf_key(const uint8_t* key);
void aes_dctf(uint8_t* data, size_t size, uint32_t mode, const uint8_t* iv);
void aes_sahf_init();
void aes_sahf_calc(const uint8_t* data, size_t size);
void aes_sahf_verify(const uint8_t* hash);

#endif // _AES_H_