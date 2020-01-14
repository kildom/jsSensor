#ifndef _DHT22_H_
#define _DHT22_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <functional>

namespace _internal_Dht22 {
    static void finalizeAndParse(uintptr_t*);
    static void parseSamples();
    static void done(uintptr_t* data);
    static void processQueue();
};

class Dht22
{
public:
    Dht22(uint32_t pinNumber);
    void measure(const std::function<void()> &callback);
    inline bool isValid() { return (bool)valid; }
    inline float getTemperature() { return temperature; }
    inline float getHumidity() { return humidity; }
private:
    uint8_t pinNumber;
    uint8_t valid;
    float temperature;
    float humidity;
    friend void _internal_Dht22::done(uintptr_t* data);
    friend void _internal_Dht22::finalizeAndParse(uintptr_t*);
    friend void _internal_Dht22::parseSamples();
    friend void _internal_Dht22::processQueue();
};

#endif // _DHT22_H_
