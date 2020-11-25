#ifndef _UTILS_H_
#define _UTILS_H_

EXTERN void zeroMem(uint8_t* dst, uint8_t* dstEnd);
EXTERN void zeroMemSize(uint8_t* dst, size_t size);
EXTERN void copyMem(uint8_t* dst, const uint8_t* src, uint8_t* dstEnd);
EXTERN void copyMemSize(uint8_t* dst, const uint8_t* src, size_t size);

#endif
