/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 00:24:33
 */

#pragma once

#define TO_SECONDS(Value) Value / 1000.0f

#include <Windows.h>

class Timer
{
public:
    Timer();

    float GetElapsed();
    void Restart();
private:
    LARGE_INTEGER _frequency;
    LARGE_INTEGER _start;
};
