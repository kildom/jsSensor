#ifndef _REF_H_
#define _REF_H_

#include <cstddef>

template<class T>
static inline void destruct(T* ptr)
{
    ptr->~T();
}

template<class T>
class Ref
{
private:

    static void inc(T* ptr)
    {
        if (!ptr) return;
        ((uintptr_t*)ptr)[-1]++;
    }

    static void dec(T* ptr)
    {
        if (!ptr) return;
        uintptr_t* buffer = &((uintptr_t*)ptr)[-1];
        uintptr_t c = --(*buffer);
        if (c == 0)
        {
            destruct(ptr);
            delete[] buffer;
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
        uintptr_t* buffer = new uintptr_t[sizeof(uintptr_t) + sizeof(T)];
        buffer[0] = 1;
        T* ptr = new(&buffer[1]) T(std::forward<Params>(params)...);
        return Ref(ptr);
    }

};


#endif // _REF_H_
