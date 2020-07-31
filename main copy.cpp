
#include <stdio.h>
#include <experimental/coroutine>

#include <stack>

struct resumable;

struct promise
{
    using promise_type = promise;
    using coro_handle = std::experimental::coroutine_handle<>;
    promise() {
        volatile int x;
        printf("---- %s %p %p\n", __FUNCTION__, (void*)this, (void*)&x);
    }
    promise* get_return_object()
    {
        //auto x = (std::experimental::coroutine_handle<promise>::from_promise(*this));
        //printf("%p %p\n", (void*)this, x.address());
        return this;
    }
    auto initial_suspend()
    {
        printf("initial\n");
        return std::experimental::suspend_always();
    }
    auto final_suspend() noexcept
    {
        printf("final\n");
        return std::experimental::suspend_always();
    }
    void unhandled_exception()
    {
        std::terminate();
    }
    void return_value(int value)
    {
        printf("RETURN %d\n", (int)value);
    }
};

std::stack<std::experimental::coroutine_handle<>> stack;

char x[sizeof(promise::coro_handle)] = {0,0,0,0,0,0,0,0};

/**struct resumable
{
    using promise_type = promise;
    using coro_handle = std::experimental::coroutine_handle<>;
    coro_handle handle;
    resumable(const coro_handle &handle) : handle(handle)
    {
    }
    bool await_ready()
    {
        return false;
    }
    void await_suspend(const coro_handle &x)
    {
        stack.push(handle);
        printf("PUSH %d\n", (int)(long long)stack.top().address());
    }
    int await_resume()
    {
        return 100;
    }
};*/


/*promise* bar()
{
    printf("Hello 2\n");
    co_await std::experimental::suspend_always();
    printf("World 2\n");
    co_return 12;
}*/

template<>
struct
std::experimental::coroutines_v1::coroutine_traits<promise*>
{
    using promise_type = promise;
};

promise* foo()
{
    printf("Hello\n");
    co_await std::experimental::suspend_always();
    printf("World\n");
    //int x = co_await *bar();
    //printf("BAR %d\n", x);
    co_return 12;
}

int main()
{
    printf("start\n");
    promise* x = foo();
    stack.push(std::experimental::coroutine_handle<promise>::from_promise(*x));
    printf("PUSH %d\n", (int)(long long)stack.top().address());
    printf("return\n");
    while (stack.size() > 0)
    {
        printf("stack %d\n", (int)stack.size());
        promise::coro_handle &top = stack.top();
        if (top.done())
        {
            printf("stack.top.done\n");
            stack.pop();
        }
        else
        {
            printf("stack.top.resume\n");
            top.resume();
        }
    }
    return 0;
}
