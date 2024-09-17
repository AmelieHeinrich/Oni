/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 15:43:59
 */

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <utility>

class Logger
{
public:
    static void Init();
    static void Exit();
    static void Info(const char *fmt, ...);
    static void Warn(const char *fmt, ...);
    static void Error(const char *fmt, ...);

    static void OnUI();
private:
    enum class LogLevel
    {
        Info,
        Warn,
        Error,
        Other
    };

    struct LoggerData
    {
        std::ofstream LogFile;
        std::vector<std::pair<std::string, LogLevel>> LogRecord;
    };
    static LoggerData _Data;
};
