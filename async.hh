#ifndef _ASYNC_HH_
#define _ASYNC_HH_

#include "myprintf.hh"
#define LOG_DBG(text, ...) //myPrintf("%s:%d: DBG: (%s) " text "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

class Promise;
class Awaitable;

template<typename T>
using Async = Awaitable*;

typedef void (*CoroutineBodyFuncT)(Promise*);
typedef void (*CoroutineResolveFuncT)(Awaitable*, void*);

class Awaitable {
public:

    void* result;

    Awaitable();
    bool onResolve(Promise* p);
    bool onResolve(Promise* p, void* result);
    bool onResolve(CoroutineResolveFuncT resolveCallback, void* data);
    bool onResolve(CoroutineResolveFuncT resolveCallback, void* data, void* result);
    void resolve();
    bool postpone(CoroutineResolveFuncT callback, void* data);

private:

    enum { NONE, RESOLVE, IMMEDIATE } callbackType;
    CoroutineResolveFuncT callback;
    void* callbackData;

    static void resumeOnResolve(Awaitable* awaitable, void* data);
};


class Promise : public Awaitable
{
public:
    Promise* next;
    void* resumePoint;
    CoroutineBodyFuncT body;

    Promise(CoroutineBodyFuncT body);
    void resume();
    bool suspend(void* resumePoint, Awaitable* awaitable);
    bool suspend(void* resumePoint, Awaitable* awaitable, void* result);
    bool yield(void* resumePoint);

    static bool execute();
};

class PromiseAlloc {};

extern PromiseAlloc promiseAlloc;


#endif
