#ifndef _UTILS_H_
#define _UTILS_H_

EXTERN void zeroMem(uint8_t* dst, size_t size);
EXTERN void copyMem(uint8_t* dst, const uint8_t* src, size_t size);
EXTERN bool compareMem(const uint8_t* a, const uint8_t* b, size_t size);

#endif
