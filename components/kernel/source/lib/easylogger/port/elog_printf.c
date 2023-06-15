#include <kernel/elog.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>
/* Make all log output just like printf, but will automatically add line end. */

static EasyLogger elog;

void elog_output(uint8_t level, const char *tag, const char *file,
		 const char *func, const long line, const char *format, ...)
{
	va_list args;
	uint8_t tag_level = -1;

	if (!elog.init_ok) {
		return;
	}

	/* check output enabled */
	if (!elog.output_enabled) {
		return;
	}
	/* level filter */
	tag_level = elog_get_filter_tag_lvl(tag);
	if (level > tag_level) {
		return;
	} else if (!strstr(tag, elog.filter.tag)) { /* tag filter */
		return;
	}

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

ElogErrCode elog_init(void)
{
	elog.init_ok = true;
	return ELOG_NO_ERR;
}

bool elog_port_is_initialized(void)
{
	return true;
}

void elog_start(void)
{
	elog_set_output_enabled(true);
}

void elog_set_output_enabled(bool enabled)
{
	elog.output_enabled = enabled;
}

bool elog_get_output_enabled(void)
{
	return elog.output_enabled;
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
	elog_set_filter_lvl(level);
	elog_set_filter_tag(tag);
	elog_set_filter_kw(keyword);
}

void elog_set_filter_lvl(uint8_t level)
{
	elog.filter.level = level;
}

void elog_set_filter_tag(const char *tag)
{
	strncpy(elog.filter.tag, tag, ELOG_FILTER_TAG_MAX_LEN);
}

void elog_set_filter_kw(const char *keyword)
{
	strncpy(elog.filter.keyword, keyword, ELOG_FILTER_KW_MAX_LEN);
}

slist_t *elog_get_tag_lvl_list(void)
{
	return &elog.filter.tag_lvl_list;
}

uint8_t elog_get_filter_tag_lvl(const char *tag)
{
	slist_t *node;
	struct ElogTagLvlFilter *tag_lvl = NULL;
	uint8_t level = elog.filter.level;

	if (!elog.init_ok)
		return level;

	/* lock output */
	elog_output_lock();
	/* find the tag in list */
	for (node = slist_first(elog_get_tag_lvl_list()); node;
	     node = slist_next(node)) {
		tag_lvl = slist_entry(node, struct ElogTagLvlFilter, list);
		if (!strncmp(tag_lvl->tag, tag, ELOG_FILTER_TAG_MAX_LEN)) {
			level = tag_lvl->level;
			break;
		}
	}
	/* unlock output */
	elog_output_unlock();

	return level;
}

int elog_set_filter_tag_lvl(const char *tag, uint8_t level)
{
	slist_t *node;
	struct ElogTagLvlFilter *tag_lvl = NULL;
	int result = 0;

	if (level > ELOG_LVL_ALL)
		return -EINVAL;

	if (!elog.init_ok)
		return result;

	/* lock output */
	elog_output_lock();
	/* find the tag in list */
	for (node = slist_first(elog_get_tag_lvl_list()); node;
	     node = slist_next(node)) {
		tag_lvl = slist_entry(node, struct ElogTagLvlFilter, list);
		if (!strncmp(tag_lvl->tag, tag, ELOG_FILTER_TAG_MAX_LEN)) {
			break;
		} else {
			tag_lvl = NULL;
		}
	}
	/* find OK */
	if (tag_lvl) {
		if (level == ELOG_LVL_ALL) {
			/* remove current tag's level filter when input level is the lowest level */
			slist_remove(elog_get_tag_lvl_list(), &tag_lvl->list);
			free(tag_lvl);
		} else {
			/* update level */
			tag_lvl->level = level;
		}
	} else {
		/* only add the new tag's level filer when level is not LOG_FILTER_LVL_ALL */
		if (level != ELOG_LVL_ALL) {
			/* new a tag's level filter */
			tag_lvl = (struct ElogTagLvlFilter *)malloc(sizeof(struct ElogTagLvlFilter));
			if (tag_lvl) {
				memset(tag_lvl->tag, 0, sizeof(tag_lvl->tag));
				strncpy(tag_lvl->tag, tag, ELOG_FILTER_TAG_MAX_LEN);
				tag_lvl->level = level;
				slist_append(elog_get_tag_lvl_list(), &tag_lvl->list);
			} else {
				result = -ENOMEM;
			}
		}
	}
	/* unlock output */
	elog_output_unlock();

	return result;
}

void elog_raw(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void elog_output_lock(void)
{
}

void elog_output_unlock(void)
{
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

void elog_hexdump2(uint8_t level, const char *name, uint8_t width, uint8_t *buf, uint16_t size)
{
#define __is_print(ch)       ((unsigned int)((ch) - ' ') < 127u - ' ')
	uint16_t i, j;

	if (!elog.init_ok) {
		return;
	}

	if (!elog.output_enabled) {
		return;
	}

	/* level filter */
	if (level > elog.filter.level) {
		return;
	} else if (!strstr(name, elog.filter.tag)) { /* tag filter */
		return;
	}

	for (i = 0; i < size; i += width) {
		/* package header */
		printf("D/HEX %s: %04X-%04X: ", name, i, i + width);

		/* dump hex */
		for (j = 0; j < width; j++) {
			if (i + j < size) {
				printf("%02X ", buf[i + j]);
			} else {
				printf("   ");
			}
			if ((j + 1) % 8 == 0) {
				printf(" ");
			}
		}
		printf("  ");
		/* dump char for hex */
		for (j = 0; j < width; j++) {
			if (i + j < size) {
				printf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
			}
		}
		printf("\n");
	}
}

void elog_hexdump(const char *name, uint8_t width, uint8_t *buf, uint16_t size)
{
	return elog_hexdump2(ELOG_LVL_DEBUG, name, width, buf, size);
}

int logger_init(void)
{
	elog_init();
	elog_set_filter_lvl(CONFIG_APP_LOG_LEVEL);
	elog_start();
	log_d("\n\n\nelog initialized\n\n");
	return 0;
}
module_arch(logger, logger_init, NULL, 2);

CONSOLE_CMD(elog, NULL, NULL, CONSOLE_CMD_MODE_SELF, "elog cmds entry");

static void _print_lvl_info(void)
{
	printf("Assert  : 0\n");
	printf("Error   : 1\n");
	printf("Warning : 2\n");
	printf("Info    : 3\n");
	printf("Debug   : 4\n");
	printf("Verbose : 5\n");
	printf("All     : 5\n");
}

static int elog_tag_lvl(int argc, char *argv[])
{
	int ret = 0;

	if (argc > 2) {
		if ((atoi(argv[2]) <= ELOG_LVL_ALL) && (atoi(argv[2]) >= 0)) {
			elog_set_filter_tag_lvl(argv[1], atoi(argv[2]));
		} else {
			printf("Please input correct level (0-%d).\n",
			       ELOG_LVL_ALL);
			ret = -1;
		}
	} else {
		printf("Please input: %s <tag> <level>.\n", argv[0]);
		_print_lvl_info();
		ret = -1;
	}

	return ret;
}
CONSOLE_CMD(tag_lvl, "elog", elog_tag_lvl, CONSOLE_CMD_MODE_SELF, "Set elog filter level by different tag")

static int elog_lvl(int argc, char *argv[])
{
	int ret = 0;
	if (argc > 1) {
		if ((atoi(argv[1]) <= ELOG_LVL_ALL) && (atoi(argv[1]) >= 0)) {
			elog_set_filter_lvl(atoi(argv[1]));
		} else {
			printf("Please input correct level (0-%d).\n",
			       ELOG_LVL_ALL);
			ret = -1;
		}
	} else {
		printf("Please input: %s <level>.\n", argv[0]);
		_print_lvl_info();
		ret = -1;
	}

	return ret;
}
CONSOLE_CMD(lvl, "elog", elog_lvl, CONSOLE_CMD_MODE_SELF, "Set elog global filter level")

static int elog_tag(int argc, char *argv[])
{
	int ret = 0;

	if (argc > 1) {
		if (strlen(argv[1]) <= ELOG_FILTER_TAG_MAX_LEN) {
			elog_set_filter_tag(argv[1]);
		} else {
			printf("The tag length is too long. Max is %d.\n",
			       ELOG_FILTER_TAG_MAX_LEN);
			ret = -1;
		}
	} else {
		elog_set_filter_tag("");
	}

	return ret;
}
CONSOLE_CMD(tag, "elog", elog_tag, CONSOLE_CMD_MODE_SELF, "Set elog global filter tag")

static int elog_kw(int argc, char *argv[])
{
	int ret = 0;

	if (argc > 1) {
		if (strlen(argv[1]) <= ELOG_FILTER_KW_MAX_LEN) {
			elog_set_filter_kw(argv[1]);
		} else {
			printf("The keyword length is too long. Max is %d.\n", ELOG_FILTER_KW_MAX_LEN);
			ret = -1;
		}
	} else {
		elog_set_filter_kw("");
	}

	return ret;
}
CONSOLE_CMD(kw, "elog", elog_kw, CONSOLE_CMD_MODE_SELF, "Set elog global filter keyword")

static int elog_filter(int argc, char *argv[])
{
	const char *lvl_name[] = { "Assert ", "Error  ", "Warning",
				   "Info   ", "Debug  ", "Verbose" };
	const char *tag = elog.filter.tag, *kw = elog.filter.keyword;
	slist_t *node;
	struct ElogTagLvlFilter *tag_lvl = NULL;

	printf("--------------------------------------\n");
	printf("elog global filter:\n");

	printf("level   : %s\n", lvl_name[elog.filter.level]);

	printf("tag     : %s\n", strlen(tag) == 0 ? "NULL" : tag);
	printf("keyword : %s\n", strlen(kw) == 0 ? "NULL" : kw);

	printf("--------------------------------------\n");
	printf("elog tag's level filter:\n");
	if (slist_isempty(elog_get_tag_lvl_list())) {
		printf("settings not found\n");
	} else {
		/* lock output */
		elog_output_lock();
		/* show the tag level list */
		for (node = slist_first(elog_get_tag_lvl_list()); node;
		     node = slist_next(node)) {
			tag_lvl = slist_entry(node, struct ElogTagLvlFilter, list);
			printf("%-*.s: ", ELOG_FILTER_TAG_MAX_LEN, tag_lvl->tag);
			printf("%s\n", lvl_name[tag_lvl->level]);
		}
		/* unlock output */
		elog_output_unlock();
	}

	return 0;
}
CONSOLE_CMD(filter, "elog", elog_filter, CONSOLE_CMD_MODE_SELF, "Show elog filter settings")
