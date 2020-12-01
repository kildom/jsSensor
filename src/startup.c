
#include "common.h"

extern uint32_t __stack_end;

__attribute__ ((noreturn))
static int resetHandler();

__attribute__((used))
__attribute__((section(".isr_vector")))
const void* __isr_vector[2] = {
	&__stack_end - 1,
	resetHandler,
};

__attribute__((used))
__attribute__((section(".mbr_reset_copy")))
const volatile void* mbr_isr_vector_copy[2] = {
	0,
	0,
};

__attribute__((naked))
__attribute__((noinline))
__attribute__((noreturn))
__attribute__((section(".ramapp")))
void ramAppStartup(void* connInfo)
{
	__asm__ volatile ("");
	__builtin_unreachable();
}

__attribute__((noinline))
__attribute__((noreturn))
void startRamApp()
{
	uint32_t buf[4];
	copyMem((uint8_t*)&buf, getConnState(), sizeof(buf));
	FORCE_LONG_JUMP(ramAppStartup)(buf);
	__builtin_unreachable();
}

__attribute__((noinline))
static void prepare()
{
	extern uint8_t __begin_rambss;
	extern uint8_t __end_rambss;
	uint32_t reas = NRF_POWER->RESETREAS;
	reas &= ~POWER_RESETREAS_RESETPIN_Msk;
	if (reas != 0)
	{
		__asm__ volatile (
			"mov PC, %0 \n"
			"bx  %1     \n"
			:: "r" (mbr_isr_vector_copy[0]), "r" (mbr_isr_vector_copy[1])
		);
		__builtin_unreachable();
	}
	zeroMem(&__begin_rambss, &__end_rambss - &__begin_rambss);
}

__attribute__((noreturn))
static int resetHandler()
{
	prepare();

    main();

	startRamApp();
}

__attribute__((noreturn))
EXTERN void startMbr()
{
	SCB->AIRCR = SCB_AIRCR_SYSRESETREQ_Msk | (0x05FA << SCB_AIRCR_VECTKEY_Pos);
	while (true) { }
}
