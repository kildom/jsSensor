
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <utility>
#include <type_traits>

struct Promise;

typedef void (*coro_inner_func)(Promise*);
typedef void (*resolve_func_t)(Promise*, void*);

bool in_coroutine;
struct {
    bool empty(){ return true; }
    void push(Promise*){}
    Promise* pop(){return nullptr; }
}resume_fifo;

struct Promise
{
    Promise(coro_inner_func func) : result(nullptr), loc(nullptr), func(func), resolve_func(nullptr), suspended(true) { }
    void* result;
    void* loc;
    coro_inner_func func;
    resolve_func_t resolve_func;
    bool suspended;
    void* resolve_data;

    void resolve() {
        if (resolve_func != nullptr) {
            resolve_func(this, resolve_data);
        }
    }

    void resume() {
        if (!suspended) return;
        suspended = false;
        if (in_coroutine) {
            resume_fifo.push(this);
        } else {
            Promise* next;
            func(this);
            while ((next = resume_fifo.pop())) {
                next->func(next);
            }
        }
    };

    static void resume_promise(Promise*, void* remote)
    {
        ((Promise*)remote)->resume();
    }

    void onResolve(Promise* st) {
        resolve_func = resume_promise;
        resolve_data = (void*)st;
    };

    void suspend(void* l, Promise* p, void* r)
    {
        loc = l;
        p->result = r;
        p->onResolve(this);
        suspended = true;
    }

    void suspend(void* l, Promise* p)
    {
        loc = l;
        p->result = nullptr;
        p->onResolve(this);
        suspended = true;
    }
};

struct _State_test_t : public Promise {
    int param;
    int x;
    Promise* a;
    _State_test_t(coro_inner_func func, int param) : Promise(func), param(param) {}
};


struct _State_test2_t : public Promise {
    void *thisPtr;
    int param;
    int x;
    Promise* a;
    _State_test2_t(coro_inner_func func, void* thisPtr, int param) : Promise(func), thisPtr(thisPtr), param(param) {}
};


template<typename T>
using Async = Promise*;

#define _State__test \
    void _test_inner(Promise*); \
    return (Promise*)(new _State_test_t(_test_inner, param)); \
    } \
    void _test_inner(Promise* _st) { \
    typedef int _result_type; \
    _State_test_t* state = (_State_test_t*)_st; \
    int &x = state->x; \
    Promise* &a = state->a; \
    if (state->loc) goto *(state->loc); \

#define _State__A__test2 \
    struct Caller { static void call(Promise* _st) { \
        ((A*)((_State_test2_t*)_st))->_test2_inner(_st); \
    }}; \
    return (Promise*)(new _State_test2_t(Caller::call, this, param)); \
    } \
    void A::_test2_inner(Promise* _st) { \
    typedef int _result_type; \
    _State_test2_t* state = (_State_test2_t*)_st; \
    int &x = state->x; \
    Promise* &a = state->a; \
    if (state->loc) goto *(state->loc); \

#define _ClassState__A \
    void _test2_inner(Promise* _st);
    void _other_test_inner(Promise* _st);

#define _State(name, ...) _State__ ## name
#define _ClassState(name) _ClassState__ ## name
#define _MethodState(cls, name, ...) _State__ ## cls ## __ ## name
#define _Await2(label, ...) state->suspend(&&label, __VA_ARGS__); return; label:
#define _AWAIT_LABEL_UNIQUE2(line, cnt) _await_label ## line ## _ ## cnt
#define _AWAIT_LABEL_UNIQUE(line, cnt) _AWAIT_LABEL_UNIQUE2(line, cnt)
#define _Await(awaitable, ...) _Await2(_AWAIT_LABEL_UNIQUE(__LINE__, __COUNTER__), awaitable, ##__VA_ARGS__)
#define _Return_args0() do { \
        static_assert(std::is_same<_result_type, void>::value, "Return value expected."); \
        state->resolve(); \
        delete state; \
        return; \
    } while (0);
#define _Return_args1(_value) do { \
        static_assert(!std::is_same<_result_type, void>::value, "Unexpected return value."); \
        if (state->result) { \
            *((_result_type*)state->result) = std::move((_value)); \
        } \
        state->resolve(); \
        delete state; \
        return; \
    } while (0);
#define _Return_r2(name, args, ...) name ## args (__VA_ARGS__)
#define _Return_r1(name, args, ...) _Return_r2(name, args, ##__VA_ARGS__)
#define _NUM_ARGS2(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r, ...) r
#define _NUM_ARGS(...) _NUM_ARGS2(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define _Return(...) _Return_r1(_Return_args, _NUM_ARGS(, ##__VA_ARGS__), ##__VA_ARGS__)

#define STR4(x) #x
#define STR3(x) STR4(x)
#define STR2(x) STR3(x)
#define STR(x) STR2(x)

//const char *tt = STR(
Async<int> test(int param) {
    _State(test, {
        int x;
        Promise* a;
    });
    a = test(12);
    {
        int r = 1;
        printf("%d", r);
    }
    _Await(a, &x);
    _Return(33);
}
//);

class A {

    Async<int> test2(int param);

private:
    _ClassState(A);
};


Async<int> A::test2(int param) {
    _MethodState(A, test2, {
        int x;
        Promise* a;
    });
    a = test(12);
    _Await(a, &x);
    _Return(33);
}


#if 0
void test_inner(Promise* st) { State_test* state = (State_test*)st; if (state->loc) { goto *(state->loc); }
    // a = test2();
    state->a = test2();
    // x = _Await a;
    state->suspend(&&_await_loc0, state->a, &state->x); return; _await_loc0:
    // return 12;
    if (state->result) { *((int*)state->result) = std::move(12); } state->resolve(); delete state; return;
} Promise* test(int param) { return (Promise*)(new State_test(test_inner, param)); }
#endif

#define MACRO2(x) #x
#define MACRO(x) MACRO2(x)
//#pragma message(MACRO(_Return()))

int main() {
 //   printf("%s\n\n", tt);
    //printf("%s\n\n", MACRO(_Return()));
    //printf("%s\n\n", MACRO(_Return(__int__, _____VALUE_____)));
    return 0;
}