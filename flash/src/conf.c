
#include "common.h" 

__attribute__((used))
__attribute__((section(".conf")))
const struct {
	uint8_t salt[8];
	uint8_t key[32];
	char name[23];
    uint8_t confSize;
} conf = {
    .confSize = 64,
};

/*
8	Frequency

	Trigger:
		Radio
7			Delay (0 - disable)
		GPIO
7			Pin number
1			Trigger level
7			Delay time
1			After delay: 0 - start bootloader, 1 - start app

	Reset reason:
1		Pin reset
1		Power on reset
1		Power off reset


TIME FORMAT time = (4 + value) << shift:
2	value
4	shift

*/