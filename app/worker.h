#ifndef _WORKER_H_
#define _WORKER_H_

#include <utility>

#include "common.h"

namespace low {

namespace worker {

enum Level {
    LEVEL_LOW = 0,
    LEVEL_HIGH = 1,
    LEVEL_UNKNOWN = 2,
    LEVELS_COUNT = 2,
};

typedef void (*Callback)(uintptr_t* data);

void init(Callback startups[LEVELS_COUNT]);
void add(Callback callback, size_t args, ...);
void addTo(Level level, Callback callback, size_t args, ...);
void addToFromISR(bool* yield, Level level, Callback callback, size_t args, ...);
bool onLevel(Level level);
Level getLevel();

template<class... A>
static inline void addFromISR(bool* yield, A&&... arg)
{
    addToFromISR(yield, LEVEL_LOW, std::forward<A>(arg)...);
}

template<class... A>
static inline void addToLow(A&&... arg)
{
    addTo(LEVEL_LOW, std::forward<A>(arg)...);
}

template<class... A>
static inline void addToHigh(A&&... arg)
{
    addTo(LEVEL_HIGH, std::forward<A>(arg)...);
}

template<class... A>
static inline void addToLowToFromISR(bool* yield, A&&... arg)
{
    addToFromISR(yield, LEVEL_LOW, std::forward<A>(arg)...);
}

template<class... A>
static inline void addToHighToFromISR(bool* yield, A&&... arg)
{
    addToFromISR(yield, LEVEL_HIGH, std::forward<A>(arg)...);
}

};

};

#endif // _WORKER_H_
