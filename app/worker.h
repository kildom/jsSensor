#ifndef _WORKER_H_
#define _WORKER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <functional>

#include "common.h"

namespace low {

enum WorkerLevel {
    WORKER_LOW = 0,
    WORKER_HIGH = 1,
};

typedef void (*WorkerCallback)(uintptr_t* data);

void workerInit(WorkerCallback lowStartup, WorkerCallback highStartup);
void workerLow(WorkerCallback callback, size_t args, ...);
void workerHigh(WorkerCallback callback, size_t args, ...);
void workerLowFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...);
void workerHighFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...);
bool workerInThread(WorkerLevel level);

}; // namespace low
TODO: reconsider naming and API to be more flexible for more workers, e.g.:
    levels:
        - WORKER_SLOW
        - WORKER_FAST
        - WORKER_FASTEST (new if needed)
    API:
        workerAdd(callback, args, ...) - add to the same level
        workerAddFromISR(yieldRequested, callback, args, ...) - add to the fastest
        workerAddTo(level, callback, args, ...) - add to specific level
        workerAddToFromISR(yieldRequested, level, callback, args, ...) - add to specific level
    C++:
        workerAdd(func) - add to slow
*/

#endif // _WORKER_H_
