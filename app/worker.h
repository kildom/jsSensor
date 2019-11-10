#ifndef _WORKER_H_
#define _WORKER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <functional>

#include "common.h"

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
void workerHighEx(const std::function<void()>& func);
bool workerInThread(WorkerLevel level);

#endif // _WORKER_H_
