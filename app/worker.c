#define PROD_ASSERT(cond, ...) // Production assert - always enabled
#define DBG_ASSERT(cond, ...) // Debug assert - enabled only in debug build
#define STATIC_ASSERT(cond, ...) // Static assert - compilation time assert

typedef void (*WorkerCallback)(uintptr_t* data);

#define MAX_ARGS 5

typedef WorkerQueueItem_tag
{
    WorkerCallback callback;
    uintptr_t data;
} WorkerQueueItem;



static QueueHandle_t workerQueue;

#define FIRST_DATA_ITEM ((WorkerCallback)NULL)
#define DATA_ITEM ((WorkerCallback)(void*)(uintptr_t)1)

static void sendCallback(WorkerCallback callback, uintptr_t data)
{
    BaseType_t ret;
    WorkerQueueItem item;
    item.callback = callback;
    item.data = data;
    ret = xQueueSend(workerQueue, &item, 0);
    PROD_ASSERT(ret == pdTRUE, "Work queue overflow");
}


void workerSend(WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    size_t i;
    uintptr_t last = 0;
    DBG_ASSERT(args <= MAX_ARGS)
    va_start(valist, args);
    for (args--)
    {
        sendCallback(args ? NULL : callback, va_arg(valist, uintptr_t));
    }
}

static void sendCallbackFromISR(bool* yieldRequested, WorkerCallback callback, uintptr_t data)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret;
    WorkerQueueItem item;
    item.callback = callback;
    item.data = data;
    ret = xQueueSendFromISR(workerQueue, &item, 0);
    PROD_ASSERT(ret == pdTRUE, "Worker queue overflow");
    if (woken) *yieldRequested = true;
}

void workerSendFromISR(bool* yieldRequested, WorkerCallback callback, size_t args, ...)
{
    va_list valist;
    size_t i;
    uintptr_t last = 0;
    DBG_ASSERT(args <= MAX_ARGS)
    va_start(valist, args);
    for (args--)
    {
        sendCallbackFromISR(yieldRequested, args ? NULL : callback, va_arg(valist, uintptr_t));
    }
}

void workerMainLoop()
{
    static uintptr_t argsBuffer[MAX_ARGS];
    size_t argsBufferSize = 0;
    BaseType_t ret;
    WorkerQueueItem item;
    do {
        timeout = timerGetNextTimeout();
        ret = pdFALSE;
        if (timeout != 0)
        {
            ret = xQueueReceive(workerQueue, &item, timeout);
        }
        if (ret)
        {
            DBG_ASSERT(argsBufferSize < MAX_ARGS);
            argsBuffer[argsBufferSize++] = item->data;
            if (item->callback)
            {
                item->callback(argsBuffer);
                argsBufferSize = 0;
            }
        }
        else
        {
            timerExecute();
        }
    } while (true);
}

// TODO: timer

#define TIMER_FLAG_ABSOLUTE 0x01000000
#define TIMER_FLAG_REPEAT 0x02000000
#define TIMER_INT_FLAG_RUNNING 0x04000000

struct Timer_tag;

typedef void (*TimerCallback)(struct Timer_tag* timer);

typedef struct Timer_tag
{
    uint32_t fireTime;
    struct Timer_tag* next;
    uint32_t flags;
    TimerCallback callback;
    uintptr_t userData;
} Timer;

static Timer* timersList = NULL;

void initTimer(Timer* timer, TimerCallback callback, uintptr_t userData);


static void startTimerFW(uintptr_t* args)
{
    startTimer((Timer*)args[0], args[1], args[2]);
}

static void addItem(Timer* timer)
{
    Timer** curPtr = &timersList;
    Timer* cur = timersList;
    while (cur && (int32_t)(cur->fireTime - timer->fireTime) <= 0)
    {
        curPtr = &cur->next;
        cur = cur->next;
    }
    *curPtr = timer;
    timer->next = cur;
}

static void removeItem(Timer* timer)
{
    Timer** curPtr = &timersList;
    Timer* cur = timersList;
    while (cur)
    {
        if (cur == timer)
        {
            *curPtr = timer->next;
            return;
        }
        curPtr = &cur->next;
        cur = cur->next;
    }
}

// e.g. interval startTimer(&someTimer, 1000, TIMER_FLAG_REPEAT | 200) - timer that starts in 1 sec and repeats every 200ms

void startTimer(Timer* timer, uint32_t time, uint32_t flags)
{
    time = calcAbsoluteTime(time, flags);
    if (!inWorkerThread())
    {
        workerSend(startTimerFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
        return;
    }
    if (timer->flags & TIMER_INT_FLAG_RUNNING)
    {
        removeItem(timer);
    }
    timer->flags = flags | TIMER_INT_FLAG_RUNNING;
    timer->fireTime = time;
    addItem(timer);
}

void stopTimer(Timer* timer)
{
    if (!inWorkerThread())
    {
        workerSend(stopTimerFW, 1, (uintptr_t)timer);
        return;
    }
    removeItem(timer);
    timer->flags &= ~TIMER_INT_FLAG_RUNNING;
}

void updateTimer(Timer* timer, TimerCallback callback, uintptr_t data);

void startTimerFromISR(bool* yieldRequested, Timer* timer, uint32_t time, uint32_t flags)
{
    time = calcAbsoluteTime(time, flags);
    workerSendFromISR(yieldRequested, startTimerFW, 3, (uintptr_t)timer, time, flags | TIMER_FLAG_ABSOLUTE);
}

void stopTimerFromISR(bool* yieldRequested, Timer* timer);
void updateTimerFromISR(bool* yieldRequested, Timer* timer);

// TODO: time critical timers that runs on HW timer, they are used in ISR, callback is executed in timer ISR context
//       (if needed)


uint32_t timerGetNextTimeout()
{
    if (timersList == NULL)
    {
        return INFINITE; // TODO: fix return value
    }
    int32_t t = timersList->fireTime - getTime();
    if (t <= 0) return 0;
    return (uint32_t)t;
}

void timerExecute()
{
    uint32_t t = timerGetNextTimeout();
    if (t != 0) return;
    Timer* timer = timersList;
    timersList = timer->next;
    if (timer->flags & REPEAT)
    {
        timer->fireTime += timer->flags & 0x00FFFFFF;
        addItem(timer);
    }
    else
    {
        timer->flags &= ~RUNNING;
    }
    timer->callback(timer);
}
