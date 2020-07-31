
#include <stdio.h>
#include <coroutine>

#include <stack>

std::stack<std::coroutine_handle<>> stack;

template <typename RT>
class PromiseBase
{
public:
    RT result;
    std::coroutine_handle<> handle;
    std::exception_ptr exception;
};

template <>
class PromiseBase<void>
{
public:
    std::coroutine_handle<> handle;
};

template <typename RT>
class PromiseAsync;

#define _ //printf("--------------- %s::%s %p\n", typeid(*this).name(), __FUNCTION__, this);

template <typename RT>
class Async
{
public:
    using promise_type = PromiseAsync<RT>;
    PromiseBase<RT> *p;
    Async(PromiseBase<RT> *p) : p(p) { _ }
    bool await_ready() { _ return false; }
    void await_suspend(std::coroutine_handle<> h) { _ p->handle = h; }
    RT await_resume() { _ return p->result; }
};

template <>
class Async<void>
{
public:
    using promise_type = PromiseAsync<void>;
    PromiseBase<void> *p;
    Async(PromiseBase<void> *p) : p(p) { _ }
    bool await_ready() { _ return false; }
    void await_suspend(std::coroutine_handle<> h) { _ p->handle = h; }
    void await_resume() { _ }
};

template <typename RT>
class PromiseAsync : private PromiseBase<RT>
{
public:
    PromiseAsync(){_};
    PromiseAsync(const PromiseAsync &) = delete;
    PromiseAsync &operator=(const PromiseAsync &) = delete;
    Async<RT> get_return_object() { _ return Async<RT>(this); }
    auto initial_suspend() noexcept { _ return std::suspend_never(); }
    auto final_suspend() noexcept { _ return std::suspend_never(); }
    void unhandled_exception()
    {
        _
            _ printf("EX\n");
        std::terminate();
    }
    void return_value(const RT &value)
    {
        _ this->result = value;
        if (this->handle != nullptr)
            this->handle.resume();
    }
};

template <>
class PromiseAsync<void> : private PromiseBase<void>
{
public:
    PromiseAsync(){_};
    PromiseAsync(const PromiseAsync &) = delete;
    PromiseAsync &operator=(const PromiseAsync &) = delete;
    Async<void> get_return_object() { _ return Async<void>(this); }
    auto initial_suspend() noexcept { _ return std::suspend_never(); }
    auto final_suspend() noexcept { _ return std::suspend_never(); }
    void unhandled_exception()
    {
        _ printf("EX\n");
        std::terminate();
    }
    void return_void()
    {
        _ if (this->handle != nullptr) this->handle.resume();
    }
};

struct MyStream : private PromiseBase<int>
{

    Async<int> read()
    {
        _ return Async<int>(this);
    }

    void feed(int data)
    {
        _
            result = data;
        std::coroutine_handle<> h(handle);
        handle = nullptr;
        if (h != nullptr)
        {
            h.resume();
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
    b = co_await read_test(ms);
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
