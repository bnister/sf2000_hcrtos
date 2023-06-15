#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <generated/br2_autoconf.h>
#include <stdint.h>
#include <stdbool.h>

#define CONSOLE_CMD_MODE_SELF   (0)
#define CONSOLE_CMD_MODE_ALL    (1)

#define CONSOLE_LOG_ERROR       (1<<1)

#define CONSOLE_MAX_HISTORY             (10)
#define CONSOLE_MAX_CMD_BUFFER_LEN      (2048)

#define CONSOLE_OK                  (0)
#define CONSOLE_ERROR               (-1)
#define CONSOLE_QUIT                (-2)
#define CONSOLE_ERROR_ARG           (-3)

#define __console_init __attribute__((__used__)) __attribute__ ((section(".console.cmd")))

typedef int (*console_cmd_callback_t)(int, char **);

struct console_init_cmd {
	const char	*parent;
	const char	*cmd;
	int		(*callback)(int argc, char **argv);
	uint8_t         mode;
	bool            initialized;
	const char	*help;
};

#ifdef CONFIG_LIB_CONSOLE
#define CONSOLE_CMD(_cmd, _parent, _callback, _mode, _help) \
static __console_init struct console_init_cmd console_init_##_cmd = { \
	.parent = _parent, \
	.cmd = #_cmd, \
	.callback = _callback, \
	.mode = _mode, \
	.initialized = false, \
	.help = _help \
};
#else
#define CONSOLE_CMD(...)
#endif

struct console_cmd {
	char               *cmd;
	int                (*callback)(int argc, char **argv);
	uint16_t           unique_len;
	uint16_t            mode;
	char               *help;
	struct console_cmd *children;
	struct console_cmd *parent;
	struct console_cmd *next;
};

#ifdef CONFIG_LIB_CONSOLE
void                 console_init(void);
void                 console_start(void);
void                 console_loop(void);
void                 console_set_hostname(const char *hostname);
int                  console_run_cmd(const char *cmd);
int                  console_run_cmd_from(struct console_cmd *cmds, const char *cmd);
struct console_cmd * console_find_cmd(const char *cmd);
int                  console_unregister_cmd(const char *cmd);
struct console_cmd * console_register_cmd(struct console_cmd *parent,
                             const char *cmd,
                             int (*callback)(int, char **),
                             unsigned int mode,
                             const char *help);
#else
static void console_init(void)
{
}
static void console_start(void)
{
}
static void console_loop(void)
{
}
static void console_set_hostname(const char *hostname)
{
}
static int console_run_cmd(const char *cmd)
{
	return -1;
}
static int console_run_cmd_from(struct console_cmd *cmds, const char *cmd)
{
	return -1;
}
static struct console_cmd *console_find_cmd(const char *cmd)
{
	return NULL;
}
static int console_unregister_cmd(const char *cmd)
{
	return -1;
}
static struct console_cmd *console_register_cmd(struct console_cmd *parent,
					 const char *cmd,
					 int (*callback)(int, char **),
					 unsigned int mode, const char *help)
{
	return NULL;
}
#endif


#ifdef __cplusplus
}
#endif

#endif
