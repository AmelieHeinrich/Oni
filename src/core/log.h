/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 17:43:10
 */

#ifndef LOG_H_
#define LOG_H_

void log_init();
void log_exit();

void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);

#endif
