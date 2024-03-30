/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 00:25:20
 */

#include "timer.hpp"

#include <ctime>

Timer::Timer()
{
    _start = clock();
}

float Timer::GetElapsed()
{
    return clock() - _start;
}

void Timer::Restart()
{
    _start = clock();
}
