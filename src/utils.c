
#include "common.h"

EXTERN void zeroMem(uint8_t* dst, size_t size)
{
    uint8_t* dstEnd = dst + size;
	while (dst < dstEnd)
	{
		*dst = 0;
		dst++;
	}
}

EXTERN void copyMem(uint8_t* dst, const uint8_t* src, size_t size)
{
    uint8_t* dstEnd = dst + size;
	while (dst < dstEnd)
	{
		*dst = *src;
		dst++;
        src++;
	}
}

EXTERN bool compareMem(const uint8_t* a, const uint8_t* b, size_t size)
{
    const uint8_t* bEnd = b + size;
	while (b < bEnd)
	{
		if (*a != *b)
		{
			return false;
		}
		a++;
        b++;
	}
	return true;
}
