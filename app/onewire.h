#ifndef _ONEWIRE_H_
#define _ONEWIRE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

void ow_init();
void ow_read(uint32_t pinNumber);

void ow_read_result(uint8_t* buffer, size_t bits);

#endif // _ONEWIRE_H_
