/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 17:44:08
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>

#include "log.h"
#include "file_system.h"

typedef struct log_data_t {
    file_t log_file;
} log_data_t;

log_data_t data;

void log_init()
{
    file_open(&data.log_file, "oni.log", FileOpen_Write | FileOpen_Create);
}

void log_exit()
{
    file_close(&data.log_file);
}

void log_info(const char *fmt, ...)
{
    time_t raw_time;
    struct tm *time_info;

    time(&raw_time);
    time_info = localtime(&raw_time);

    char msg_buf[4096];
    char arg_buf[4096];
    va_list vl;
    
    va_start(vl, fmt);
    vsnprintf(arg_buf, sizeof(arg_buf), fmt, vl);
    va_end(vl);

    char *time_str = asctime(time_info);
    time_str[strlen(time_str) - 1] = '\0';
    sprintf(msg_buf, "[%s] [INFO] %s\n", time_str, arg_buf);

    printf("\033[32m");
    printf("%s", msg_buf);
    printf("\033[39m");

    file_write_utf8(&data.log_file, msg_buf, strlen(msg_buf));
}

void log_warn(const char *fmt, ...)
{
    time_t raw_time;
    struct tm *time_info;

    time(&raw_time);
    time_info = localtime(&raw_time);

    char msg_buf[4096];
    char arg_buf[4096];
    va_list vl;
    
    va_start(vl, fmt);
    vsnprintf(arg_buf, sizeof(arg_buf), fmt, vl);
    va_end(vl);

    char *time_str = asctime(time_info);
    time_str[strlen(time_str) - 1] = '\0';
    sprintf(msg_buf, "[%s] [WARN] %s\n", time_str, arg_buf);

    printf("\033[33m");
    printf("%s", msg_buf);
    printf("\033[39m");

    file_write_utf8(&data.log_file, msg_buf, strlen(msg_buf));
}

void log_error(const char *fmt, ...)
{
    time_t raw_time;
    struct tm *time_info;

    time(&raw_time);
    time_info = localtime(&raw_time);

    char msg_buf[4096];
    char arg_buf[4096];
    va_list vl;
    
    va_start(vl, fmt);
    vsnprintf(arg_buf, sizeof(arg_buf), fmt, vl);
    va_end(vl);

    char *time_str = asctime(time_info);
    time_str[strlen(time_str) - 1] = '\0';
    sprintf(msg_buf, "[%s] [ERROR] %s\n", time_str, arg_buf);

    printf("\033[31m");
    printf("%s", msg_buf);
    printf("\033[39m");

    file_write_utf8(&data.log_file, msg_buf, strlen(msg_buf));
}
