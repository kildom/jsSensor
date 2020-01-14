#ifndef _PROMISE_HPP_
#define _PROMISE_HPP_

#include <functional>
#include "variant.hpp"
#include "ref.h"

struct _PromiseInternal;

class Promise
{
public:
    Promise then(const std::function<void()> &func);
    Promise then(const std::function<void(Variant)> &func);
    Promise then(const std::function<Variant()> &func);
    Promise then(const std::function<Variant(Variant)> &func);
    Promise then(const std::function<Promise()> &func);
    Promise then(const std::function<Promise(Variant)> &func);
    void resolve();
    void resolve(Variant result);
    static Promise make();
    static Promise makeResolved();
    static Promise makeResolved(Variant result);
    static Promise all(Promise* promises, size_t count);
    template <size_t size>
    static inline Promise all(Promise (&promises)[size])
    {
        all(promises, size);
    }

private:
    Ref<_PromiseInternal> data;
};

#endif // _PROMISE_HPP_
