#include <functional>
#include <queue>

struct A
{
    static int x;
};

int A::x;

A a;

int main(int argc, char* argv[])
{
    return a.x;
}

#if 0

#include <worker.h>

class DHT22Cls
{
    static void measure(int pinNumber, const std::function<void(float h, float t, const char* errorString)>& callback);
};

extern DHT22Cls DHT22;

////////////////////////////////////////////////////////

struct Item
{
    std::function<void(float h, float t, const char* errorString)> callback;
    int pinNumber;
    Item(const std::function<void(float h, float t, const char* errorString)>& c, int p)
        : callback(c), pinNumber(p) {}
};

static std::queue<Item> queue;
static bool running = false;
static std::function<void(float h, float t, const char* errorString)> currentCallback;

static void startNow(int pinNumber, const std::function<void(float h, float t, const char* errorString)>& callback)
{
    currentCallback = callback;
    running = true;
    send([](uintptr_t* args)
    {
        dht22Read(args[0]);
    }, 1, pinNumber);
}

namespace DHT22
{
void measure(int pinNumber, const std::function<void(float h, float t, const char* errorString)>& callback)
{
    if (running)
    {
        queue.push(Item(callback, pinNumber));
        return;
    }
    else
    {
        startNow(pinNumber, callback);
    }
}
}

void hiSend(const std::function<void()>& callback);

extern "C"
void dht22ReadResult(uint16_t rh, uint16_t temp, const char* errorString)
{
    hiSend([rh, temp, errorString]()
    {
        currentCallback((float)rh / 10.0f, (float)temp / 10.0f, errorString);
        if (queue.empty())
        {
            running = false;
        }
        else
        {
            const Item& item = queue.front();
            startNow(item.pinNumber, item.callback);
        }
    });
}

///////////////////////////////////////////////////////////////////

struct Exception { Exception(char* f, int l, char* m){} };
#define EXCEPTION(str) Exception(__FILE__, __LINE__, str)

static bool running = false;
static std::function<void(float h, float t, const char* errorString)> currentCallback;

static void startNow(int pinNumber, const std::function<void(float h, float t, const char* errorString)>& callback)
{
}

namespace DHT22
{
void measure(int pinNumber, const std::function<void(float h, float t, const char* errorString)>& callback)
{
    if (running)
    {
        hiSend([callback](){
            callback(0, 0, "DHT22 measurement is already running!");
        });
        return;
    }
    currentCallback = callback;
    running = true;
    send([](uintptr_t* args)
    {
        dht22Read(args[0]);
    }, 1, pinNumber);
}
}

extern "C"
void dht22ReadResult(uint16_t rh, uint16_t temp, const char* errorString)
{
    send([](uintptr_t* arg)
    {
        currentCallback((float)arg[0] / 10.0f, (float)(int16_t)arg[1] / 10.0f, (const char*)arg[2]);
    }, 3, rh, temp, errorString);
}
#endif