#ifndef _WORKER_HPP_
#define _WORKER_HPP_

#include <functional>

#include "common.h"

void setAsync(const std::function<void()>& func);

#endif // _WORKER_HPP_
