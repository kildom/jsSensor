#ifndef _FIFO_HPP_
#define _FIFO_HPP_


template<class T, size_t size>
class Fifo
{
public:
    Fifo() : start(0), len(0) { }
    void push(const T& x)
    {
        DBG_ASSERT(len < size, "FIFO overflow");
        array[(start + len) % size] = x;
        len++;
    }
    T& pop()
    {
        DBG_ASSERT(len > 0, "FIFO empty");
        size_t current = start;
        len--;
        start = (start + 1) % size;
        return array[current];
    }
    T& peek()
    {
        DBG_ASSERT(len > 0, "FIFO empty");
        return array[start];
    }
    size_t length()
    {
        return len;
    }
private:
    size_t start;
    size_t len;
    T array[size];
};


#endif // _FIFO_HPP_
