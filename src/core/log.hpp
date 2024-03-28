/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 15:43:59
 */

#pragma once

#include <iostream>
#include <fstream>

class Logger
{
public:
    static void Init();
    static void Exit();
    static void Info(const char *fmt, ...);
    static void Warn(const char *fmt, ...);
    static void Error(const char *fmt, ...);

private:
    struct LoggerData
    {
        std::ofstream LogFile;
    };
    static LoggerData _Data;
};
