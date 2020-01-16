#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

class Window
{
public:
    Window(int windowSize = 4, bool removeMinMax = true, int minValid = 2);
    ~Window();
    void add(float value);
    void addInvalid();
    bool isValid();
    float get();
};

#endif // _WINDOW_HPP_
