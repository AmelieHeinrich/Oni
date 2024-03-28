/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-28 15:46:33
 */

#include "log.hpp"

#include <exception>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <cstdarg>

Logger::LoggerData Logger::_Data;

void Logger::Init()
{
    _Data.LogFile = std::ofstream("log.txt", std::ios::trunc);
    if (!_Data.LogFile.is_open()) {
        throw std::runtime_error("Failed to create log file!");
    }
}

void Logger::Exit()
{
    if (_Data.LogFile.is_open()) {
        _Data.LogFile.close();
    }
}

void Logger::Info(const char *fmt, ...)
{
    std::stringstream ss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    char buf[4096];
    va_list vl;
    
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);
    ss << std::put_time(&tm, "[%d-%m-%Y %H:%M:%S] ");
    ss << "[INFO] ";
    ss << buf << std::endl;
    std::cout << "\033[32m";
    std::cout << ss.str();
    std::cout << "\033[39m";
    _Data.LogFile << ss.str();
}

void Logger::Warn(const char *fmt, ...)
{
    std::stringstream ss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    char buf[4096];
    va_list vl;
    
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);
    ss << std::put_time(&tm, "[%d-%m-%Y %H:%M:%S] ");
    ss << "[WARN] ";
    ss << buf << std::endl;
    std::cout << "\033[33m";
    std::cout << ss.str();
    std::cout << "\033[39m";
    _Data.LogFile << ss.str();
}

void Logger::Error(const char *fmt, ...)
{
    std::stringstream ss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    char buf[4096];
    va_list vl;
    
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);
    ss << std::put_time(&tm, "[%d-%m-%Y %H:%M:%S] ");
    ss << "[ERROR] ";
    ss << buf << std::endl;
    std::cout << "\033[31m";
    std::cout << ss.str();
    std::cout << "\033[39m";
    _Data.LogFile << ss.str();
}
