#ifndef _WORKER_HPP_
#define _WORKER_HPP_

#include <stdint.h>
#include <stdlib.h>

#include <functional>

#include "common.h"

void workerHighEx(const std::function<void()>& func);

#endif // _WORKER_HPP_
