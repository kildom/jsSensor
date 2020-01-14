#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <functional>
#include <ref.h>

class SharedLock;

// TODO: Implement this if some resource requires sharing at high level, e.g. SPIS for both 1-wire and DHT22
class Shared
{
public:
    Shared(uint32_t count = 1);
    Ref<SharedLock> acquire();
    void acquire(const std::function<void(Ref<SharedLock>& lock)> &callback);
private:
};

#endif // _SHARED_H_
