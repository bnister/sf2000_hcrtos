#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

#define CONSOLE_CMD_MODE_SELF   (0)
#define CONSOLE_CMD_MODE_ALL    (1)

#define CONSOLE_LOG_ERROR       (1<<1)

#define CONSOLE_MAX_HISTORY             (10)
#define CONSOLE_MAX_CMD_BUFFER_LEN      (1024)

#define CONSOLE_OK                  (0)
#define CONSOLE_ERROR               (-1)
#define CONSOLE_QUIT                (-2)
#define CONSOLE_ERROR_ARG           (-3)

struct console_init_cmd {
	const char	*parent;
	const char	*cmd;
	int		(*callback)(int argc, char **argv);
	uint8_t         mode;
	bool            initialized;
	const char	*help;
};

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

void                 console_init(const char *hostname);
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

#ifdef __cplusplus
}
#endif

#endif
