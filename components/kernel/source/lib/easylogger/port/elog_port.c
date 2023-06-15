/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */

//#define USE_PTHREAD_API 1

#include <kernel/elog.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <generated/br2_autoconf.h>
#include <kernel/module.h>
#include <errno.h>
#ifdef USE_PTHREAD_API

#if defined(BR2_PACKAGE_PTHREAD)
#include <pthread.h>
#endif

static pthread_mutex_t output_lock;
#else

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

QueueHandle_t output_lock;
#endif /* USE_PTHREAD_API */

static bool _elog_initialized = false;
/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;
#ifdef UST_PTHREAD_API
    pthread_mutex_init(&output_lock, NULL);
#else
    output_lock = xSemaphoreCreateMutex();
#endif

    _elog_initialized = true;
    return result;
}

/**
 *  Check if elog already initialized.
 */
bool elog_port_is_initialized(void)
{
	return _elog_initialized;
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    printf("%.*s", (int)size, log);
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
#ifdef UST_PTHREAD_API
    pthread_mutex_lock(&output_lock);
#else
    if (!uxInterruptNesting && !vTaskIsInCritical())
        xSemaphoreTake(output_lock, portMAX_DELAY);
#endif
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
#ifdef UST_PTHREAD_API
    pthread_mutex_unlock(&output_lock);
#else
    if (!uxInterruptNesting && !vTaskIsInCritical())
        xSemaphoreGive(output_lock);
#endif
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {

    static char cur_system_time[25] = { 0 };
    struct tm *p;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    p = localtime(&tv.tv_sec);
    if (p == NULL) {
        return "";
    }
    snprintf(cur_system_time, 24, "%02d-%02d %02d:%02d:%02d.%03ld", p->tm_mon + 1, p->tm_mday,
            p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec / 1000);

    return cur_system_time;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    static char cur_process_info[10] = { 0 };
    snprintf(cur_process_info, 10, "p:%02d", getpid());
    return cur_process_info;    
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    static char cur_thread_info[16] = { 0 };
    unsigned int tid = 0;

#ifdef USE_PTHREAD_API
    tid = (unsigned int) pthread_self();
#else
    TaskStatus_t tstatus;
    vTaskGetInfo( xTaskGetCurrentTaskHandle(), &tstatus, pdFALSE , eInvalid);
    tid = tstatus.xTaskNumber;
#endif

    snprintf(cur_thread_info, 16, "t:%02u,%d",tid, errno);
    return cur_thread_info;
}

int logger_init(void)
{
#ifdef CONFIG_EASYLOGGER_NORMAL
	ElogErrCode ret = ELOG_NO_ERR;
	size_t fmt_m = ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME |
		       ELOG_FMT_T_INFO | ELOG_FMT_FUNC | ELOG_FMT_LINE;

	ret = elog_init();
	if (ret != ELOG_NO_ERR)
		return -1;

	/* set logger log format */
	elog_set_fmt(ELOG_LVL_ASSERT, fmt_m);
	elog_set_fmt(ELOG_LVL_ERROR, fmt_m);
	elog_set_fmt(ELOG_LVL_WARN, fmt_m);
	elog_set_fmt(ELOG_LVL_INFO, fmt_m);
	elog_set_fmt(ELOG_LVL_DEBUG, fmt_m);
	elog_set_fmt(ELOG_LVL_VERBOSE, fmt_m);
#ifdef ELOG_COLOR_ENABLE
	elog_set_text_color_enabled(true);
#endif
	elog_set_filter_lvl(CONFIG_APP_LOG_LEVEL);
	elog_start();
	log_d("\n\n\nelog initialized\n\n");
#endif
	return 0;
}
module_arch(logger, logger_init, NULL, 2)
