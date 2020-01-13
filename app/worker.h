#ifndef _WORKER_H_
#define _WORKER_H_

#include <utility>

#include "common.h"

namespace low {

enum WorkerLevel {
    WORKER_LEVEL_LOW = 0,
    WORKER_LEVEL_HIGH = 1,
    WORKER_LEVEL_UNKNOWN = 2,
    WORKER_LEVELS_COUNT = 2,
};

typedef void (*WorkerCallback)(uintptr_t* data);

void workerInit(WorkerCallback startups[WORKER_LEVELS_COUNT]);
void workerAdd(WorkerCallback callback, size_t args, ...);
void workerAddTo(WorkerLevel level, WorkerCallback callback, size_t args, ...);
void workerAddToFromISR(bool* yieldRequested, WorkerLevel level, WorkerCallback callback, size_t args, ...);
bool workerOnLevel(WorkerLevel level);
WorkerLevel workerGetLevel();

template<class... A>
static inline void workerAddFromISR(bool* yieldRequested, A&&... arg)
{
    workerAddToFromISR(yieldRequested, WORKER_LEVEL_LOW, std::forward<A>(arg)...);
}

};

#endif // _WORKER_H_
