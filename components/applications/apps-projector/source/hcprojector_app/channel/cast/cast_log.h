#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>
#include <stdarg.h>

#define LOG_PREFIX_INFO     "[INFO]: "
#define LOG_PREFIX_DEBUG    "[\033[1;34mDEBUG\033[0m] "
#define LOG_PREFIX_WARN     "[\033[1;33mWARN\033[0m] "
#define LOG_PREFIX_ERR      "[\033[1;31mERR\033[0m] "

#define LOG_LEVEL_INFO  0x01
#define LOG_LEVEL_DEBUG 0x02
#define LOG_LEVEL_WARN  0x04
#define LOG_LEVEL_ERR   0x08

static volatile int log_level = LOG_LEVEL_INFO | LOG_LEVEL_WARN | LOG_LEVEL_ERR;

#define LOG_INFO(modules, format, arg...) \
    do                                       \
    {                                        \
        if (log_level & LOG_LEVEL_INFO)  \
            printf("["#modules"]" LOG_PREFIX_INFO format"\n", ##arg);           \
    } while (0)

#define LOG_DEBUG(modules, format, arg...) \
    do                                       \
    {                                        \
        if (log_level & LOG_LEVEL_DEBUG)  \
            printf("["#modules"]"LOG_PREFIX_DEBUG"[%s %d]: "format"\n", __FUNCTION__, __LINE__, ##arg);           \
    } while (0)

#define LOG_WARN(modules, format, arg...) \
    do                                       \
    {                                        \
        if (log_level & LOG_LEVEL_WARN)  \
            printf("["#modules"]"LOG_PREFIX_WARN"[%s %d]: "format"\n", __FUNCTION__, __LINE__, ##arg);           \
    } while (0)

#define LOG_ERR(modules, format, arg...) \
    do                                       \
    {                                        \
        if (log_level & LOG_LEVEL_ERR)  \
            printf("["#modules"]"LOG_PREFIX_ERR"[%s %d]: "format"\n", __FUNCTION__, __LINE__, ##arg);           \
    } while (0)

#define log(modules, level, format, arg...) LOG_##level(modules, format, ##arg)


#endif