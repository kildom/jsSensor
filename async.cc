

#include "async.hh"


static Promise* const PromiseSuspendedValue = (Promise*)1;

static Promise* activeFirst = nullptr;
static Promise* activeLast = nullptr;


Awaitable::Awaitable() : 
    result(nullptr),
    callbackType(NONE)
{
    LOG_DBG("");
}


bool Awaitable::onResolve(Promise* p, void* result)
{
    return onResolve(resumeOnResolve, (void*)p, result);
}


bool Awaitable::onResolve(CoroutineResolveFuncT resolveCallback, void* data, void* result)
{
    this->result = result;
    if (callbackType == IMMEDIATE) {
        LOG_DBG("Immediate called");
        callbackType = NONE;
        callback(this, callbackData);
        return false;
    } else {
        LOG_DBG("Waiting for resolve");
        callbackType = RESOLVE;
        callback = resolveCallback;
        callbackData = data;
        return true;
    }
}


bool Awaitable::onResolve(Promise* p)
{
    return onResolve(resumeOnResolve, (void*)p, nullptr);
}


bool Awaitable::onResolve(CoroutineResolveFuncT resolveCallback, void* data)
{
    return onResolve(resolveCallback, data, nullptr);
}


void Awaitable::resumeOnResolve(Awaitable* awaitable, void* data)
{
    LOG_DBG("Resolve caused coroutine resuming");
    ((Promise*)data)->resume();
}


void Awaitable::resolve()
{
    if (callbackType == RESOLVE)
    {
        LOG_DBG("Calling resolve callback");
        callbackType = NONE;
        callback(this, callbackData);
    }
    else
    {
        LOG_DBG("Resolve ignored - no callback");
    }
}

bool Awaitable::postpone(CoroutineResolveFuncT callback, void* data)
{
    if (callbackType != RESOLVE)
    {
        LOG_DBG("Postpone resolving");
        callbackType = IMMEDIATE;
        this->callback = callback;
        callbackData = data;
        return true;
    }
    else
    {
        LOG_DBG("Not possible to postpone - coroutine is waiting");
        return false;
    }
}


Promise::Promise(CoroutineBodyFuncT body) : next(PromiseSuspendedValue), resumePoint(nullptr), body(body)
{
    LOG_DBG("");
    resume();
}

void Promise::resume()
{
    if (next != PromiseSuspendedValue) {
        LOG_DBG("Coroutine already resumed");
        return;
    }

    if (activeLast) {
        activeLast->next = this;
    } else {
        activeFirst = this;
    }
    
    next = nullptr;
    activeLast = this;

    LOG_DBG("Coroutine added to resume list");
}

bool Promise::suspend(void* resumePoint, Awaitable* awaitable)
{
    return suspend(resumePoint, awaitable, nullptr);
}

bool Promise::suspend(void* resumePoint, Awaitable* awaitable, void* result)
{
    LOG_DBG("Suspending...");
    bool suspended = awaitable->onResolve(this, result);
    if (suspended)
    {
        LOG_DBG("Coroutine suspended");
        this->next = PromiseSuspendedValue;
        this->resumePoint = resumePoint;
    } else {
        LOG_DBG("Awaitable was ready - coroutine is not suspended");
    }
    return suspended;
}

bool Promise::yield(void* resumePoint)
{
    if (!activeFirst) {
        return false;
    }

    this->next = PromiseSuspendedValue;
    this->resumePoint = resumePoint;
    this->resume();

    return true;
}


bool Promise::execute()
{
    if (!activeFirst) {
        LOG_DBG("No coroutine to execute");
        return false;
    }
    auto p = activeFirst;
    activeFirst = p->next;
    if (!activeFirst) {
        activeLast = nullptr;
    }
    LOG_DBG("Executing coroutine");
    p->body(p);
    if (activeFirst != nullptr)
    {
        LOG_DBG("Next coroutine is waiting for execution");
        return true;
    }
    else
    {
        LOG_DBG("No more coroutines to execute");
        return false;
    }
}

