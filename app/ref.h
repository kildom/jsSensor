#ifndef _REF_H_
#define _REF_H_

template<class T>
class Ref
{
private:
    T* ptr;

    void inc() {
        ((uint32_t*)ptr)[-1]++;
    }

    void dec() {
        ((uint32_t*)ptr)[-1]--;
    }

    void decAndCheck() {
        uint32_t c = --((uint32_t*)ptr)[-1];
        if (c == 0)
        {
            delete ptr;
        }
    }

    Ref(T* data) { ptr = data; }

public:
    Ref() { ptr = nullptr; }
    Ref(const Ref<T>& old) { ptr = old.ptr; if (ptr) inc(); }
    Ref(Ref<T>&& old) { ptr = old.ptr; old.ptr = nullptr; }
    ~Ref() { decAndCheck(); }
    
    template <typename ...Params>
    static Ref<T> make(Params&&... params)
    {
        struct CntAndData
        {
            uint32_t counter;
            T data;
            CntAndData(Params&&... params) : counter(1), data(std::forward<Params>(params)...) { }
        };
        CntAndData* ptr = new CntAndData(std::forward<Params>(params)...);
        return Ref(&ptr->data);
    }

};

#endif // _REF_H_
