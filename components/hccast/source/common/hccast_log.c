#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "hccast_log.h"

static unsigned int log_level = LL_NOTICE;

void hccast_log_level_set(int level)
{
    log_level = level;
}

void hccast_log(enum loglevel level, const char *fmt, ...)
{
    va_list ap;
    char *fs;

    if (level > log_level)
        return;

    fs = malloc(32 + strlen(fmt));

    struct timeval ts;
    struct tm tp_;
    struct tm *tp;

    gettimeofday(&ts, NULL);
    tp = localtime_r(&ts.tv_sec, &tp_);

    strftime(fs, 10, "[%H:%M:%S", tp);
    sprintf(fs + 9, ".%03d][hccast][%d] %s", (int)(ts.tv_usec / 1000), level, fmt);

    va_start(ap, fmt);
    vfprintf(stderr, fs, ap);
    va_end(ap);

    free(fs);
}
