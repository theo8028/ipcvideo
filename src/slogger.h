#ifndef SLOGGER_H
#define SLOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <string>

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_OFF   5

#define LOG_LEVEL     LOG_LEVEL_WARN

inline void sLog(int  level, const char* format, va_list ap)
{
    char str[1024];
    vsnprintf(str, 1024, format, ap);
    if (level >= LOG_LEVEL)
    {
        switch(level){
        case LOG_LEVEL_TRACE :
            printf("[trace]");
            break;
        case LOG_LEVEL_DEBUG:
            printf("[debug]");
            break;
        case LOG_LEVEL_INFO:
            printf("[info ]");
            break;
        case LOG_LEVEL_WARN:
            printf("[warn ]");
            break;
        case LOG_LEVEL_ERROR:
            printf("[error]");
            break;
        }        
        printf(" ipcvideo::");
        printf(str);
    }    
}

inline void slog_trace(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sLog(LOG_LEVEL_TRACE, format, args);
    va_end(args);
}


inline void slog_debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sLog(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

inline void slog_info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sLog(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

inline void slog_warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sLog(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

inline void slog_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sLog(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}


#endif