
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
void ramAppStartup(void* connInfo, const void* key)
{
	__asm__ volatile ("");
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
	zeroMem(&__begin_rambss, &__end_rambss);
}

EXTERN void aes_dcfb_key(const uint8_t* key);
EXTERN void aes_dcfb(uint8_t* data, size_t size, uint32_t mode, const uint8_t* iv);
EXTERN void aes_hash(const uint8_t* data, size_t size, uint8_t* hash);

__attribute__((noreturn))
static int resetHandler()
{
	prepare();

    main();

	FORCE_LONG_JUMP(ramAppStartup)(aes_dcfb, aes_hash); // TODO: Fill with actual pointers

	__builtin_unreachable();
}

void startMbr()
{
	SCB->AIRCR = SCB_AIRCR_SYSRESETREQ_Msk | (0x05FA << SCB_AIRCR_VECTKEY_Pos);
}
