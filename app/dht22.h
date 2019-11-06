#ifndef _DHT22_H_
#define _DHT22_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

void dht22_read(uint32_t pinNumber);

void dht22_read_result(uint16_t rh, uint16_t temp);

#endif // _DHT22_H_
