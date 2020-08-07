

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef INLINE
#define INLINE static inline
#endif


INLINE void svcPutString(const char *message)
{
    const int SYS_WRITE0 = 0x04;
    asm volatile(
        "mov r0, %[cmd]\n"
        "mov r1, %[msg]\n"
        "bkpt #0xAB\n"
        //"svc 0x00123456\n"
        :
        : [ cmd ] "r"(SYS_WRITE0), [ msg ] "r"(message)
        : "r0", "r1");
}

void svcExit()
{
    const int SYS_EXIT = 0x18;
    asm volatile(
        "mov r0, %[cmd]\n"
        "bkpt #0xAB\n"
        :
        : [ cmd ] "r"(SYS_EXIT)
        : "r0");
}

int mystrlen(const char* p)
{
    const char* a = p;
    while (*p) {
        p++;
    }
    return p - a;
}

void myPrintf(const char* format, ...)
{
    static char buffer[1024];
    const char* src = format;
    char* dst = buffer;
    va_list args;
    va_start(args, format);
#if 1
    while (*src) {
        if (*src == '%') {
            src++;
            if (*src == '%') {
                *dst++ = '%';
            } else if (*src == 'd') {
                itoa(va_arg(args, int), dst, 10);
                dst += mystrlen(dst);
            } else if (*src == 's') {
                const char *p = va_arg(args, const char*);
                int len = mystrlen(p);
                memcpy(dst, p, len);
                dst += len;
            }
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst++ = 0;
#else
    vsprintf(dst, format, args);
#endif
    va_end(args);
    svcPutString(buffer);
}
