#if 0

void Dht22::calcWindow(bool v, uintptr_t h, uintptr_t t)
{
    TASK_HIGH;
    int16_t hh;
    int16_t tt;
    if (v)
    {
        hh = h;
        tt = t;
        if (tt & 0x8000)
        {
            tt = -(tt & 0x7FFF);
        }
    }
    else
    {
        hh = -0x7FFF;
        tt = -0x7FFF;
    }
    if (windowIndex < 0)
    {
        window[2] = hh;
        window[3] = tt;
        window[4] = hh;
        window[5] = tt;
        windowIndex = 0;
    }
    window[windowIndex] = hh;
    window[windowIndex + 1] = tt;
    windowIndex += 2;
    if (windowIndex >= windowSize)
    {
        windowIndex = 0;
    }
    valid = calcWindowItem(window, humidity);
    valid = valid && calcWindowItem(window + 1, temperature);
}

bool Dht22::calcWindowItem(int16_t* buffer, float &value)
{
    int16_t min = 0x7FFF;
    int16_t max = -0x7FFF;
    int validCount = 0;
    for (int i = 0; i < windowSize; i += 2)
    {
        int16_t x = buffer[i];
        if (x <= -0x7000) continue;
        if (x < min) min = x;
        if (x > max) max = x;
        validCount++;
    }
    if (validCount < 3)
    {
        return false;
    }
    int32_t sum = 0;
    for (int i = 0; i < windowSize; i += 2)
    {
        int16_t x = buffer[i];
        if (x <= -0x7000) continue;
        if (x == min) { min = 0x7FFF; }
        else if (x == max) { max = 0x7FFF; }
        else { sum += x; }
    }
    return (float)sum / (float)(validCount - 2);
}

#endif
