#ifndef _WORKER_H_
#define _WORKER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef void (*WorkerCallback)(uintptr_t* data);

void workerInit(WorkerCallback startupCallback);
void workerSend(WorkerCallback callback, size_t args, ...);
void workerSendFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...);
bool inWorkerThread();

#endif // _WORKER_H_
