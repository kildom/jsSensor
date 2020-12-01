#ifndef _STARTUP_H_
#define _STARTUP_H_

#define BL_ASSERT(cond) do { if (!cond) shutdown(); } while (0)

__attribute__((noreturn))
EXTERN void startMbr();

#endif
