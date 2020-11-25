
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
