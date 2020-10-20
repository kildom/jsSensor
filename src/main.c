#include <stdint.h>

#define ISR_VECTOR_ITEMS 48
#define STACK_SIZE 512
#define APP_SIZE (16 * 1024 - 512)

__attribute__((section(".app")))
uint8_t app_data[APP_SIZE];

__attribute__((section(".noinit")))
uint32_t stack[STACK_SIZE / 4];

__attribute__ ((noreturn))
void Reset_Handler()
{
    int main();

    main();
    while (1);
}

__attribute__((used))
__attribute__((section(".isr_vector")))
const void* __isr_vector[ISR_VECTOR_ITEMS] = {
	&stack[sizeof(stack) / sizeof(stack[0]) - 1],
	Reset_Handler,
	// Magic values to detect valid bootloader binary before uploading.
	// They will be replaced by the application ISR vectors.
	(void*)0x41dec8ba, (void*)0x0230a515, (void*)0x0f1955be, (void*)0x1aee5b4d,
	(void*)0x2061ac4c, (void*)0x18d4f330, (void*)0x31beca84, (void*)0x11fa6d52,
};


int main()
{
    __asm__ volatile ("nop");
    return 0;
}
