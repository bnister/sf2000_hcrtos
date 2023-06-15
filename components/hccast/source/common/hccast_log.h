#ifndef _HCCAST_LOG_H_
#define _HCCAST_LOG_H_

enum loglevel {
	LL_FATAL = 0,
	LL_ERROR,
	LL_WARNING,
	LL_NOTICE,
	LL_INFO,
	LL_DEBUG,
	LL_SPEW,
	LL_FLOOD,
};

void hccast_log_level_set(int level);
void hccast_log(enum loglevel level, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

#endif
