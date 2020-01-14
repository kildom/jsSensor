#ifndef _DHT22_H_
#define _DHT22_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <functional>

class Dht22
{
public:
    Dht22(uint32_t pinNumber);
    void measure(const std::function<void()> &callback);
    inline bool isValid() { return (bool)valid; }
    inline float getTemperature() { return temperature; }
    inline float getHumidity() { return humidity; }
    inline uint32_t getPinNumber() { return pinNumber; }
private:
    uint8_t pinNumber;
    uint8_t valid;
    float temperature;
    float humidity;
    static void done(uintptr_t* data);
};

#endif // _DHT22_H_
