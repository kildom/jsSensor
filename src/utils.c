
#include "common.h"

EXTERN void zeroMem(uint8_t* dst, uint8_t* dstEnd)
{
	while (dst < dstEnd)
	{
		*dst = 0;
		dst++;
	}
}

EXTERN void zeroMemSize(uint8_t* dst, size_t size)
{
    zeroMem(dst, dst + size);
}

EXTERN void copyMem(uint8_t* dst, const uint8_t* src, uint8_t* dstEnd)
{
	while (dst < dstEnd)
	{
		*dst = *src;
		dst++;
        src++;
	}
}

EXTERN void copyMemSize(uint8_t* dst, const uint8_t* src, size_t size)
{
    copyMem(dst, src, dst + size);
}


