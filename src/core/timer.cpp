/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 00:25:20
 */

#include "timer.hpp"

#include <ctime>

Timer::Timer()
{
    QueryPerformanceFrequency(&_frequency);
    QueryPerformanceCounter(&_start);
}

float Timer::GetElapsed()
{
    LARGE_INTEGER end;

    QueryPerformanceCounter(&end);

    return (end.QuadPart - _start.QuadPart) * 1000.0 / _frequency.QuadPart;
}

void Timer::Restart()
{
    QueryPerformanceCounter(&_start);
}
