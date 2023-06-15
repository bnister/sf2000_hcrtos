#include <kernel/elog.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
/* Add dummy function, to make prebuilt library link pass. */

ElogErrCode elog_init(void)
{
	return ELOG_NO_ERR;
}

bool elog_port_is_initialized(void)
{
	return false;
}

void elog_start(void)
{
}

void elog_output(uint8_t level, const char *tag, const char *file,
		 const char *func, const long line, const char *format, ...)
{
}

void elog_set_output_enabled(bool enabled)
{
}

bool elog_get_output_enabled(void)
{
	return false;
}

void elog_set_text_color_enabled(bool enabled)
{
}

bool elog_get_text_color_enabled(void)
{
	return false;
}

void elog_set_fmt(uint8_t level, size_t set)
{
}

void elog_set_filter(uint8_t level, const char *tag, const char *keyword)
{
}

void elog_set_filter_lvl(uint8_t level)
{
}

void elog_set_filter_tag(const char *tag)
{
}

void elog_set_filter_kw(const char *keyword)
{
}

void elog_raw(const char *format, ...)
{
}

void elog_output_lock(void) {
}

void elog_output_unlock(void) {
}


void elog_output_lock_enabled(bool enabled)
{
}

void elog_assert_set_hook(void (*hook)(const char *expr, const char *func,
				       size_t line))
{
}

int8_t elog_find_lvl(const char *log)
{
	return 0;
}

void elog_hexdump2(const char *name, uint8_t width, uint8_t *buf, uint16_t size)
{
}

void elog_hexdump(const char *name, uint8_t width, uint8_t *buf, uint16_t size)
{
}
