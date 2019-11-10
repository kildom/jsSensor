#ifndef _WORKER_H_
#define _WORKER_H_

#include <functional>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef void (*WorkerCallback)(uintptr_t* data);

void workerInit(WorkerCallback startupCallback){}
void workerSend(WorkerCallback callback, size_t args, ...){}
void workerSendFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...){}
void sendXXXXXX(const std::function<void(uintptr_t* data)>& f){}

#endif // _WORKER_H_
