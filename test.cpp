

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


#ifndef INLINE
#define INLINE static inline
#endif


INLINE void svcPutString(const char *message)
{
    const int SYS_WRITE0 = 0x04;
    asm volatile(
        "mov r0, %[cmd]\n"
        "mov r1, %[msg]\n"
        "bkpt #0xAB\n"
        //"svc 0x00123456\n"
        :
        : [ cmd ] "r"(SYS_WRITE0), [ msg ] "r"(message)
        : "r0", "r1");
}

INLINE void svcExit()
{
    const int SYS_EXIT = 0x18;
    asm volatile(
        "mov r0, %[cmd]\n"
        "bkpt #0xAB\n"
        :
        : [ cmd ] "r"(SYS_EXIT)
        : "r0");
}

int mystrlen(const char* p)
{
    const char* a = p;
    while (*p) {
        p++;
    }
    return p - a;
}

void myPrintf(const char* format, ...)
{
    static char buffer[1024];
    const char* src = format;
    char* dst = buffer;
    va_list args;
    va_start(args, format);
    while (*src) {
        if (*src == '%') {
            src++;
            if (*src == '%') {
                *dst++ = '%';
            } else if (*src == 'd') {
                itoa(va_arg(args, int), dst, 10);
                dst += mystrlen(dst);
            }
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst++ = 0;
    va_end(args);
    svcPutString(buffer);
}

#define printf myPrintf

int main2()
{
    myPrintf("OK %d\n", 123);
    for (int i = 0; i < 3; i++)
    {
        svcPutString("line\n");
    }
    svcExit();
    return 0;
}

//=========================================================================================================


//#include <stdio.h>
#include <coroutine>

#include <deque>


//std::deque<std::coroutine_handle<>> resumeQueue;
//bool coActive = false;


static inline void resume(std::coroutine_handle<> handle)
{
    handle.resume();
    #if 0
    if (handle == nullptr)
    {
        return;
    }
    else if (coActive)
    {
        resumeQueue.push_back(handle);
    }
    else
    {
        coActive = true;
        handle.resume();
        while (!resumeQueue.empty())
        {
            handle = resumeQueue.front();
            resumeQueue.pop_front();
            handle.resume();
        }
        coActive = false;
    }
    #endif
}


template<typename RT>
class Async;


template<typename RT>
class PromiseAsync;


template<typename RT>
class PromiseBase
{
public:
    std::coroutine_handle<> handle;
    Async<RT>* async;
};


template<>
class PromiseBase<void>
{
public:
    std::coroutine_handle<> handle;
};


template<typename RT>
class Async
{
public:
    using promise_type = PromiseAsync<RT>;
    PromiseBase<RT> *p;
    RT r;
    Async() : p(nullptr) {};
    Async(PromiseBase<RT> *p) : p(p) { p->async = this; }
    Async(const Async &a) = delete;
    Async(Async &&a) : p(a.p), r(std::move(a.r)) { a.p = nullptr; if (p) p->async = this; }
    ~Async() { if (p != nullptr) p->async = nullptr; }
    Async &operator=(const Async &a) = delete;
    Async &operator=(Async &&a) { p = a.p; r = std::move(a.r); a.p = nullptr; if (p) p->async = this; return *this; }
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) { p->handle = h; }
    RT await_resume() { return r; }
};

template<>
class Async<void>
{
public:
    using promise_type = PromiseAsync<void>;
    PromiseBase<void> *p;
    Async() {}
    Async(PromiseBase<void> *p) : p(p) { }
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) { p->handle = h; }
    void await_resume() { }
};

template<typename RT>
class PromiseAsync : private PromiseBase<RT>
{
public:
    PromiseAsync() {}
    PromiseAsync(const PromiseAsync &) = delete;
    ~PromiseAsync() { if (this->async != nullptr) this->async->p = nullptr; }
    PromiseAsync &operator=(const PromiseAsync &) = delete;
    Async<RT> get_return_object() { return Async<RT>(this); }
    auto initial_suspend() noexcept { return std::suspend_never(); }
    auto final_suspend() noexcept { return std::suspend_never(); }
    void unhandled_exception() { std::terminate(); }
    void return_value(RT x) { if (this->async != nullptr) this->async->r = x; resume(this->handle); }
};

template<>
class PromiseAsync<void> : private PromiseBase<void>
{
public:
    PromiseAsync() {}
    PromiseAsync(const PromiseAsync &) = delete;
    PromiseAsync &operator=(const PromiseAsync &) = delete;
    Async<void> get_return_object() { return Async<void>(this); }
    auto initial_suspend() noexcept { return std::suspend_never(); }
    auto final_suspend() noexcept { return std::suspend_never(); }
    void unhandled_exception() { std::terminate(); }
    void return_void() { resume(this->handle); }
};

struct MyStream : private PromiseBase<int>
{

    Async<int> read()
    {
        return Async<int>(this);
    }

    void feed(int data)
    {
        if (async != nullptr)
            async->r = data;
        std::coroutine_handle<> h(handle);
        handle = nullptr;
        if (h != nullptr)
        {
            resume(h);
        }
    }
};

Async<int> read_test(MyStream &ms)
{
    int sum = 0;
    for (int i = 0; i < 10; i++)
    {
        int data = co_await ms.read();
        sum += data;
        printf("DATA: %d\n", data);
    }
    co_return sum;
}

Async<void> ff(MyStream &ms)
{
    printf(">>1\n");
    int a = co_await read_test(ms);
    printf(">>2\n");
    int b = co_await read_test(ms);
    printf("ab %d %d\n", a, b);
    printf(">>3\n");
    auto x = read_test(ms);
    Async<int> y;
    y = std::move(x);
    b = co_await y;
    printf("ab %d %d\n", a, b);
}

Async<void> abc(MyStream &ms) {
    
    co_await ff(ms);
    co_await read_test(ms);
}

int main()
{
    MyStream ms;
    ff(ms);
    for (int i = 10; i < 110; i++)
    {
        //printf("--%d\n", i);
        ms.feed(i);
    }
    svcExit();
    return 0;
}
