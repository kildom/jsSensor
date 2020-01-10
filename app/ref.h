#ifndef _REF_H_
#define _REF_H_

#include <cstddef>

template<class T>
class Ref
{
private:

    static void inc(T* ptr) {
        if (!ptr) return;
        ((uint32_t*)ptr)[-1]++;
    }

    static void dec(T* ptr) {
        if (!ptr) return;
        uint32_t c = --((uint32_t*)ptr)[-1];
        if (c == 0)
        {
            // TODO: don't delete ptr - it should be both counter and data (as created in make)
            delete ptr;
        }
    }

    Ref(T* data) { ptr = data; }

public:

    T* ptr;

    Ref() { ptr = nullptr; }
    Ref(const Ref& old) { ptr = old.ptr; inc(ptr); }
    Ref(Ref&& old) { ptr = old.ptr; old.ptr = nullptr; }
    Ref(std::nullptr_t) { ptr = nullptr; }
    ~Ref() { dec(ptr); }

    Ref& operator=(const Ref& old) { inc(old.ptr); dec(ptr); ptr = old.ptr; return *this; }
    Ref& operator=(Ref&& old) { dec(ptr); ptr = old.ptr; old.ptr = nullptr; return *this; }
    Ref& operator=(std::nullptr_t) { dec(ptr); ptr = nullptr; return *this; }

    T* operator->() { return ptr; }
    T& operator*() { return *ptr; }

    bool operator==(const Ref& other) const { return ptr == other.ptr; }
    bool operator!=(const Ref& other) const { return ptr != other.ptr; }
    bool operator==(std::nullptr_t) const { return ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr != nullptr; }
    friend bool operator==(std::nullptr_t, const Ref<T>& other) { return nullptr == other.ptr; }
    friend bool operator!=(std::nullptr_t, const Ref<T>& other) { return nullptr != other.ptr; }

    operator bool() { return ptr != nullptr; }

    T* createUnmanaged() { inc(ptr); return ptr; }
    static Ref restoreFromUnmanaged(T* source) { inc(source); return Ref(source); }
    static void deleteUnmanaged(T* source) { dec(source); }
    
    template <typename ...Params>
    static Ref make(Params&&... params)
    {
        struct CntAndData
        {
            // use uintptr_t for counter
            uint32_t counter;
            // TODO: If 'data' is aligned to >32bit empty space is here and it have to be initialized instead of counter.
            //       The initialization must be before initialization of data.
            // Solution: Allocate buffer sizeof(uint32_t) + sizeof(T)
            //           Initialize counter = 1
            //           Call constructor on data using 'placement new': https://stackoverflow.com/questions/519808/call-a-constructor-on-a-already-allocated-memory
            //           In dec() method call destructor directly and free buffer.
            T data;
            CntAndData(Params&&... params) : counter(1), data(std::forward<Params>(params)...) { }
        };
        CntAndData* ptr = new CntAndData(std::forward<Params>(params)...);
        return Ref(&ptr->data);
    }

};


#endif // _REF_H_
