
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <utility>
#include <type_traits>
#include <malloc.h>

#include "myprintf.hh"
#include "async.hh"

class PromiseAlloc;

void* operator new(size_t size, const PromiseAlloc&)
{
    return malloc(size);
}

void operator delete(void *ptr, const PromiseAlloc&)
{
    return free(ptr);
}

void operator ^(const PromiseAlloc&, Promise* promise)
{
    promise->~Promise();
    free(promise);
}


class MyStream : private Awaitable {
    public:

    int temp;

    Awaitable* read()
    {
        return this;
    }

    static void immediateResolve(Awaitable* a, void*)
    {
        MyStream* th = (MyStream*)a;
        *(int*)th->result = th->temp;
    }

    void feed(int data)
    {
        if (postpone(immediateResolve, nullptr))
        {
            temp = data;
        }
        else
        {
            *(int*)result = data;
            resolve();
        }
    }

};

struct _sum_state : public Promise
{
    MyStream& s;
    int sum;
    int i;
    int x;
    _sum_state(CoroutineBodyFuncT body, MyStream& s) : Promise(body), s(s) { }
};


Async<int> sum(MyStream& s)
{
    void sum_body_(Promise *_p);
    LOG_DBG("Allocating coroutine");
    return new(promiseAlloc) _sum_state(sum_body_, s);
}

void sum_body_(Promise *_p)
{
    _sum_state *_state = (_sum_state*)_p;
    MyStream &s = _state->s;
    int &sum = _state->sum;
    int &i = _state->i;
    int &x = _state->x;

    if (_state->resumePoint) {
        LOG_DBG("Coroutine jumps to resume point");
        goto *(_state->resumePoint);
    } else {
        LOG_DBG("Coroutine started");
    }

    __asm__ volatile ("nop\nnop\nnop\nnop\n");

    sum = 0;
    for (i = 0; i < 10; i++) {
        if (_state->suspend(&&_coro_resume0, s.read(), &x)) {
            LOG_DBG("Coroutine suspended");
            return;
        }
        _coro_resume0:;
    __asm__ volatile ("nop\nnop\nnop\nnop\n");
        myPrintf("Got: %d\n", x);
        sum += x;
    }

    __asm__ volatile ("nop\nnop\nnop\nnop\n");
    LOG_DBG("Returning from coroutine");
    if (_state->result) {
        LOG_DBG("Result moved to destination");
        *(int*)_state->result = std::move(sum);
    } else {
        LOG_DBG("Result ignored");
    }
    LOG_DBG("Resolving this coroutine");
    __asm__ volatile ("nop\nnop\nnop\nnop\n");
    _state->resolve();
    LOG_DBG("Deallocating this coroutine");
    promiseAlloc ^ _state;
}


int main() {
    int res;
    myPrintf("JEST %d '%s'\n", 12, "OK");
    MyStream str;
    Async<int> a = sum(str);
    a->result = &res;
    str.feed(10);
    Promise::execute();
    Promise::execute();
    str.feed(11);
    str.feed(12);
    Promise::execute();
    for (int i = 13; i < 23; i++) {
        str.feed(i);
        Promise::execute();
        Promise::execute();
    }
    myPrintf("RES: %d\n", res);
    svcExit();
    return 0;
}