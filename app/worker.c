
typedef void (*WorkerCallback)(uintptr_t data);


typedef WorkerQueueItem_tag
{
    WorkerCallback callback;
    uintptr_t data;
} WorkerQueueItem;


static QueueHandle_t workerQueue;


void workerSend(WorkerCallback callback, size_t parameters, ...)
{
    BaseType_t ret;
    WorkerQueueItem item;
    item.callback = callback;
    item.data = data;
    ret = xQueueSend(workerQueue, &item, 0);
    PROD_ASSERT(ret == pdTRUE);
}

void workerSendFromISR(bool* yieldRequested, WorkerCallback callback, size_t parameters, ...)
{
    BaseType_t ret;
    BaseType_t woken = pdFALSE;
    WorkerQueueItem item;
    item.callback = callback;
    item.data = data;
    ret = xQueueSendFromISR(workerQueue, &item, &woken);
    PROD_ASSERT(ret == pdTRUE);
    if (woken) *yieldRequested = true;
}

void workerMainLoop()
{
    BaseType_t ret;
    WorkerQueueItem item;
    do {
        ret = xQueueReceive(workerQueue, &item, timerNextTimeoutGet());
        if (ret)
        {
            item->callback(item->data);
        }
        else
        {
            timerExecute();
        }
    } while (true);
}

// TODO: timer

typedef void (*TimerCallback)(uintptr_t data);

void setTimerFromWorker(uintptr_t* data)
{
    TimerCallback callback = data[0];
    uintptr_t data = data[1];
    uint32_t fireTime = data[2];
}

void setTimer(TimerCallback callback, uintptr_t data, uint32_t ticks)
{
    uint32_t fireTime = timerNowGet() + ticks;
    workerSend(setTimerFromWorker, 3, callback, data, fireTime);
}

void setTimerFromISR(bool* yieldRequested, TimerCallback callback, uintptr_t data, uint32_t ticks)
{
    uint32_t fireTime = timerNowGet() + ticks;
    workerSendFromISR(yieldRequested, setTimerFromWorker, 3, callback, data, fireTime);
}
