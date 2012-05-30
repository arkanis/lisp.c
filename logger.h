#ifndef _LOGGER_H
#define _LOGGER_H

#include "output_stream.h"

#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

void log_setup(int level, output_stream_t *os);
void log_printf(int level, const char *file, int line, const char *func, const char *format, ...);

#define info(...) log_printf(LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define warn(...) log_printf(LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define error(...) log_printf(LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif