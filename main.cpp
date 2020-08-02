
#include <stdio.h>
#include <coroutine>

#include <deque>

std::deque<std::coroutine_handle<>> resumeQueue;
bool coActive = false;

void resume(std::coroutine_handle<> handle)
{
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
}

template<typename RT>
class Async;

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
class PromiseAsync;

template<typename RT>
class Async
{
public:
    using promise_type = PromiseAsync<RT>;
    PromiseBase<RT> *p;
    RT result;
    Async() = delete;// : p(nullptr) { printf("()\n"); }
    Async(PromiseBase<RT> *p) : p(p) { p->async = this; printf("(PromiseBase *p)\n"); }
    Async(const Async &a) = delete;// : p(a.p) { printf("(const Async &a)\n"); }
    Async(Async &&a) : p(a.p), result(a.result) { a.p = nullptr; if (p) p->async = this; }
    ~Async() { if (p != nullptr) p->async = nullptr; printf("~\n"); }
    Async &operator=(const Async &a) = delete;
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) { p->handle = h; }
    RT await_resume() { return result; }
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
    Async<RT> get_return_object() { printf("Create\n"); return Async<RT>(this); }
    auto initial_suspend() noexcept { return std::suspend_never(); }
    auto final_suspend() noexcept { return std::suspend_never(); }
    void unhandled_exception() { std::terminate(); }
    void return_value(RT x) { if (this->async != nullptr) this->async->result = x; resume(this->handle); }
};

template<>
class PromiseAsync<void> : private PromiseBase<void>
{
public:
    PromiseAsync() {}
    PromiseAsync(const PromiseAsync &) = delete;
    PromiseAsync &operator=(const PromiseAsync &) = delete;
    Async<void> get_return_object() { printf("Create\n"); return Async<void>(this); }
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
            async->result = data;
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
    auto y = std::move(x);
    b = co_await y;
    printf("ab %d %d\n", a, b);
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
    return 0;
}
