/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-31 00:24:33
 */

#pragma once

#define TO_SECONDS(Value) Value / 1000.0f

class Timer
{
public:
    Timer();

    float GetElapsed();
    void Restart();
private:
    float _start;
};
