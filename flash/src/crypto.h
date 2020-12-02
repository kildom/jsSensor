#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#define AES_DCFB_ENCRYPT 0
#define AES_DCFB_DECRYPT 48

EXTERN void aes_dcfb_key(const uint8_t* key);
EXTERN void aes_dcfb(uint8_t* data, size_t size, uint32_t mode, const uint8_t* iv);
EXTERN void aes_hash(const uint8_t* data, size_t size, uint8_t* hash);

#endif
