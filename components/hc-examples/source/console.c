#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "console.h"

#define console_error(fmt, ...)                                                \
	do {                                                                   \
		if (g_debug_control & CONSOLE_LOG_ERROR) {                     \
			printf(fmt, ##__VA_ARGS__);                            \
			printf("\r\n");                                        \
		}                                                              \
	} while (0)

#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif

#define free_z(p)                                                              \
	do {                                                                   \
		if (p) {                                                       \
			free(p);                                               \
			(p) = 0;                                               \
		}                                                              \
	} while (0)

#ifndef CTRL
#define CTRL(c) (c - '@')
#endif

struct console_def {
	struct console_cmd *cmds_root;
	struct console_cmd *cmds_current;
	char *history[CONSOLE_MAX_HISTORY];
	char showprompt;
	char *promptchar;
	char *hostname;
	char *promptstring;

	/* temporary buffer for console_cmd_name() to prevent leak */
	char *cmdname;
};

static struct console_def *g_console = NULL;
static unsigned int g_debug_control = CONSOLE_LOG_ERROR;

static int nprintf(const char *string, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		putchar(*(string + i));
	}

	return n;
}

static int show_prompt(struct console_def *console)
{
	int len = 0;

	if (console->hostname)
		len += nprintf(console->hostname, strlen(console->hostname));

	if (console->promptstring)
		len += nprintf(console->promptstring,
			       strlen(console->promptstring));

	return len + nprintf(console->promptchar, strlen(console->promptchar));
}

static void console_clear_line(char *cmd, int l, int cursor)
{
	int i;

	if (cursor < l) {
		for (i = 0; i < (l - cursor); i++)
			nprintf(" ", 1);
	}

	for (i = 0; i < l; i++)
		cmd[i] = '\b';
	nprintf(cmd, i);

	for (i = 0; i < l; i++)
		cmd[i] = ' ';
	nprintf(cmd, i);

	for (i = 0; i < l; i++)
		cmd[i] = '\b';
	nprintf(cmd, i);

	memset(cmd, 0, i);
}

static void console_set_promptstring(struct console_def *console,
				     char *promptstring)
{
	free_z(console->promptstring);
	if (promptstring)
		console->promptstring = strdup(promptstring);
}

static void console_set_promptchar(struct console_def *console,
				   char *promptchar)
{
	if (NULL == promptchar)
		return;
	free_z(console->promptchar);
	console->promptchar = strdup(promptchar);
}

static int console_set_promptstr(struct console_def *console, char *str)
{
	if (str && *str) {
		char string[64];
		snprintf(string, sizeof(string), "(%s)", str);
		console_set_promptstring(console, string);
	} else {
		console_set_promptstring(console, NULL);
	}

	return 0;
}

static int console_set_cmds_current(struct console_def *console,
				    struct console_cmd *cmd)
{
	console->cmds_current = cmd;

	return 0;
}

static char *console_cmd_name(struct console_def *console,
			      struct console_cmd *cmd)
{
	char *name = console->cmdname;
	char *o;

	if (name)
		free(name);

	if (!(name = malloc(1)))
		return NULL;

	memset(name, 0, 1);

	while (cmd && cmd != console->cmds_current->parent) {
		o = name;
		name = NULL;
		asprintf(&name, "%s%s%s", cmd->cmd, *o ? " " : "", o);
		cmd = cmd->parent;
		free(o);
	}
	console->cmdname = name;
	return name;
}

static char *console_cmd_name2(struct console_def *console,
			       struct console_cmd *cmd)
{
	char *name = console->cmdname;
	char *o;

	if (name)
		free(name);

	if (!(name = malloc(1)))
		return NULL;

	memset(name, 0, 1);

	while (cmd) {
		o = name;
		name = NULL;
		asprintf(&name, "%s%s%s", cmd->cmd, *o ? "/" : "", o);
		cmd = cmd->parent;
		free(o);
	}
	console->cmdname = name;
	return name;
}

static int console_show_help(struct console_def *console, struct console_cmd *c,
			     unsigned int mode)
{
	struct console_cmd *p;

	for (p = c; p; p = p->next) {
		if (p->mode == mode) {
			console_error("  %-20s %s",
				      console_cmd_name(console, p),
				      p->help ?: "");
		}
		//		if (p->children != NULL)
		//			console_show_help(console, p->children, mode);
	}

	return CONSOLE_OK;
}

static int console_help(UNUSED(int argc), UNUSED(char *argv[]))
{
	struct console_def *console = g_console;

	console_error("\r\nCommands available:");
	console_show_help(console, console->cmds_root, CONSOLE_CMD_MODE_ALL);
	console_show_help(console, console->cmds_current,
			  CONSOLE_CMD_MODE_SELF);

	return CONSOLE_OK;
}

static int console_exit(UNUSED(int argc), UNUSED(char *argv[]))
{
	int ret = CONSOLE_QUIT; /* Set to CONSOLE_QUIT if want to quit console */

	struct console_def *console = g_console;

	if (console->cmds_current->parent == NULL) {
		return ret;
	} else if (console->cmds_current->parent->parent == NULL) {
		console_set_cmds_current(console, console->cmds_root);
		console_set_promptstr(console, NULL);
		return CONSOLE_OK;
	} else {
		console_set_cmds_current(
			console,
			console->cmds_current->parent->parent->children);
		console_set_promptstr(
			console,
			console_cmd_name2(console,
					  console->cmds_current->parent));
		return CONSOLE_OK;
	}
}

static int console_history(UNUSED(int argc), UNUSED(char *argv[]))
{
	int i;
	struct console_def *console = g_console;

	for (i = 0; i < CONSOLE_MAX_HISTORY; i++) {
		if (console->history[i])
			console_error("%3d. %s", i, console->history[i]);
	}

	return CONSOLE_OK;
}

static int dummy_callback(UNUSED(int argc), UNUSED(char *argv[]))
{
	return CONSOLE_OK;
}

static struct console_cmd *
console_register_cmd2(struct console_def *console, struct console_cmd *parent,
		      const char *cmd, int (*callback)(int, char **),
		      unsigned int mode, const char *help)
{
	struct console_cmd *c, *p;

	if (!cmd)
		return NULL;

	if (!(c = malloc(sizeof(struct console_cmd))))
		return NULL;

	memset(c, 0, sizeof(struct console_cmd));

	if (callback)
		c->callback = callback;
	else
		c->callback = dummy_callback;

	c->next = NULL;
	c->children = NULL;
	c->mode = mode;
	if (!(c->cmd = strdup(cmd))) {
		free(c);
		return NULL;
	}

	c->parent = parent;

	if (help) {
		if (!(c->help = strdup(help))) {
			free(c->cmd);
			free(c);
			return NULL;
		}
	}

	if (parent) {
		if (parent->children == NULL) {
			parent->children = c;
		} else {
			for (p = parent->children; p && p->next; p = p->next)
				;
			if (p)
				p->next = c;
		}
	} else {
		if (console->cmds_root == NULL) {
			console->cmds_root = c;
			console->cmds_current = c;
		} else {
			for (p = console->cmds_root; p && p->next; p = p->next)
				;
			if (p)
				p->next = c;
		}
	}

	return c;
}

static int console_build_shortest(struct console_def *console,
				  struct console_cmd *cmds)
{
	struct console_cmd *c, *p;
	char *cp, *pp;
	int len;

	for (c = cmds; c; c = c->next) {
		c->unique_len = 1;
		for (p = cmds; p; p = p->next) {
			if (c == p)
				continue;

			cp = c->cmd;
			pp = p->cmd;
			len = 1;

			while (*cp && *pp && *cp++ == *pp++)
				len++;

			if (len > c->unique_len)
				c->unique_len = len;
		}

		if (c->children)
			console_build_shortest(console, c->children);
	}

	return CONSOLE_OK;
}

static struct console_cmd *
_console_register_cmd(struct console_def *console, struct console_cmd *parent,
		      const char *cmd, int (*callback)(int, char **),
		      unsigned int mode, const char *help)
{
	struct console_cmd *c;

	c = console_register_cmd2(console, parent, cmd, callback, mode, help);

	console_build_shortest(console, console->cmds_root);

	return c;
}

static int console_add_history(struct console_def *console, char *cmd)
{
	int i;

	for (i = 0; i < CONSOLE_MAX_HISTORY; i++) {
		if (!console->history[i]) {
			if (i == 0 || strcasecmp(console->history[i - 1], cmd))
				if (!(console->history[i] = strdup(cmd)))
					return CONSOLE_ERROR;
			return CONSOLE_OK;
		}
	}

	if (CONSOLE_MAX_HISTORY > 0 && 
		strcasecmp(console->history[CONSOLE_MAX_HISTORY - 1], cmd)) {
		free(console->history[0]);
		for (i = 0; i < CONSOLE_MAX_HISTORY - 1; i++)
			console->history[i] = console->history[i + 1];

		if (!(console->history[CONSOLE_MAX_HISTORY - 1] = strdup(cmd)))
			return CONSOLE_ERROR;
	}

	return CONSOLE_OK;
}

static void console_free_history(struct console_def *console)
{
	int i;
	for (i = 0; i < CONSOLE_MAX_HISTORY; i++) {
		if (console->history[i])
			free_z(console->history[i]);
	}
}

static int console_parse_line(const char *line, char *words[], int max_words)
{
	int nwords = 0;
	char *p = (char *)line;
	char *word_start = 0;
	int inquote = 0;

	while (*p) {
		if (!isspace((int)*p)) {
			word_start = p;
			break;
		}
		p++;
	}

	while (nwords < max_words - 1) {
		if (!*p || *p == inquote ||
		    (word_start && !inquote &&
		     (isspace((int)*p) || *p == '|'))) {
			if (word_start) {
				int len = p - word_start;

				memcpy(words[nwords] = malloc(len + 1),
				       word_start, len);
				words[nwords++][len] = 0;
			}

			if (!*p)
				break;

			if (inquote)
				p++; /* skip over trailing quote */

			inquote = 0;
			word_start = 0;
		} else if (*p == '"' || *p == '\'') {
			inquote = *p++;
			word_start = p;
		} else {
			if (!word_start) {
				if (*p == '|') {
					if (!(words[nwords++] = strdup("|")))
						return 0;
				} else if (!isspace((int)*p))
					word_start = p;
			}

			p++;
		}
	}

	return nwords;
}

static int console_get_completions(struct console_def *console, char *cmd,
				   char **completions, int max_completions)
{
	struct console_cmd *c;
	struct console_cmd *n;
	int num_words = 0, i, k = 0;
	int free_num_words = 0;
	char *words[128] = { 0 };

	if (!cmd)
		return 0;
	while (isspace((int)*cmd))
		cmd++;

	num_words = console_parse_line(cmd, words,
				       sizeof(words) / sizeof(words[0]));

	free_num_words = num_words;

	if (!cmd[0] || cmd[strlen(cmd) - 1] == ' ')
		num_words++;

	if (!num_words)
		return 0;

	for (c = console->cmds_current, i = 0;
	     c && i < num_words && k < max_completions; c = n) {
		n = c->next;

		if (words[i] && strncasecmp(c->cmd, words[i], strlen(words[i])))
			continue;

		if (i < num_words - 1) {
			if (strlen(words[i]) < c->unique_len)
				continue;

			n = c->children;
			i++;
			continue;
		}

		completions[k++] = c->cmd;
	}

	if (console->cmds_current != console->cmds_root) {
		for (c = console->cmds_root, i = 0;
		     c && i < num_words && k < max_completions; c = n) {
			n = c->next;

			if (c->mode != CONSOLE_CMD_MODE_ALL)
				continue;

			if (words[i] &&
			    strncasecmp(c->cmd, words[i], strlen(words[i])))
				continue;

			if (i < num_words - 1) {
				if (strlen(words[i]) < c->unique_len)
					continue;

				n = c->children;
				i++;
				continue;
			}

			completions[k++] = c->cmd;
		}
	}

	for (i = 0; i < free_num_words; i++)
		free(words[i]);

	return k;
}

static int console_find_cmd2(struct console_def *console,
			     struct console_cmd *cmds, unsigned int mode,
			     int num_words, char *words[], int start_word,
			     int show_result)
{
	struct console_cmd *c, *again = NULL;
	int c_words = num_words;

	/* Deal with ? for help */
	if (!words[start_word])
		return CONSOLE_ERROR;

	if (words[start_word][strlen(words[start_word]) - 1] == '?') {
		int l = strlen(words[start_word]) - 1;

		for (c = cmds; c; c = c->next) {
			if (strncasecmp(c->cmd, words[start_word], l) == 0 &&
			    (c->callback || c->children) && (c->mode == mode)) {
				console_error("  %-20s %s", c->cmd,
					      c->help ?: "");
			}
		}

		return CONSOLE_OK;
	}

	for (c = cmds; c; c = c->next) {
		int rc = CONSOLE_OK;

		if (c->mode != mode)
			continue;

		if (strncasecmp(c->cmd, words[start_word], c->unique_len))
			continue;

		if (strncasecmp(c->cmd, words[start_word],
				strlen(words[start_word])))
			continue;

	AGAIN:
		/* Found a word! */
		if (!c->children) {
			/* Last word */
			if (!c->callback) {
				console_error("No callback for \"%s\"",
					      console_cmd_name(console, c));
				return CONSOLE_ERROR;
			}
		} else {
			if (start_word == c_words - 1) {
				if (c->callback) {
					goto CORRECT_CHECKS;
				}

				console_error("Incomplete command");
				return CONSOLE_ERROR;
			}
			rc = console_find_cmd2(console, c->children, mode,
					       num_words, words, start_word + 1,
					       show_result);
			if (rc == CONSOLE_ERROR_ARG) {
				if (c->callback) {
					rc = CONSOLE_OK;
					goto CORRECT_CHECKS;
				} else {
					if (show_result) {
						console_error(
							"Invalid %s \"%s\"",
							"command",
							words[start_word]);
					}
				}
			}
			return rc;
		}

		if (!c->callback) {
			console_error("Internal server error processing \"%s\"",
				      console_cmd_name(console, c));
			return CONSOLE_ERROR;
		}

	CORRECT_CHECKS:

		if (rc == CONSOLE_OK) {
			if (c->children != NULL) {
				console_set_cmds_current(console, c->children);
				console_set_promptstr(
					console, console_cmd_name2(console, c));
			}
			//rc = c->callback(c_words - start_word - 1,
			//words + start_word + 1);
			rc = c->callback(c_words - start_word,
					 words + start_word);
		}

		return rc;
	}

	/* drop out of config submode if we have matched cmd on MODE_CONFIG */
	if (again) {
		c = again;
		console_set_promptstr(console, NULL);
		goto AGAIN;
	}

	if (start_word == 0) {
		if (show_result) {
			console_error("Invalid %s \"%s\"", "command",
				      words[start_word]);
		}
	}

	return CONSOLE_ERROR_ARG;
}

static struct console_cmd *console_find_cmd_ext(struct console_cmd *parent,
						const char *cmd)
{
	struct console_cmd *p, *n = NULL;

	for (p = parent; p; p = p->next) {
		if (strcmp(p->cmd, cmd) == 0) {
			return p;
		}
		if (p->children != NULL)
			n = console_find_cmd_ext(p->children, cmd);
	}

	return n;
}

static int _console_run_cmd(struct console_def *console, const char *cmd)
{
	int r;
	unsigned int num_words, i;
	char *words[128] = { 0 };

	if (!cmd)
		return CONSOLE_ERROR;
	while (isspace((int)*cmd))
		cmd++;

	if (!*cmd)
		return CONSOLE_OK;

	num_words = console_parse_line(cmd, words,
				       sizeof(words) / sizeof(words[0]));

	if (num_words) {
		r = console_find_cmd2(console, console->cmds_current,
				      CONSOLE_CMD_MODE_SELF, num_words, words,
				      0, 0);

		if (r != CONSOLE_OK) {
			r = console_find_cmd2(console, console->cmds_root,
					      CONSOLE_CMD_MODE_ALL, num_words,
					      words, 0, 1);
		}
	} else {
		r = CONSOLE_ERROR;
	}

	for (i = 0; i < num_words; i++)
		free(words[i]);

	if (r == CONSOLE_QUIT)
		return r;

	return r;
}

int console_run_cmd_from(struct console_cmd *cmds, const char *cmd)
{
	int r;
	unsigned int num_words, i;
	char *words[128] = { 0 };

	if (!cmd)
		return CONSOLE_ERROR;
	while (isspace((int)*cmd))
		cmd++;

	if (!*cmd)
		return CONSOLE_OK;

	num_words = console_parse_line(cmd, words,
				       sizeof(words) / sizeof(words[0]));

	if (num_words) {
		r = console_find_cmd2(g_console, cmds,
				      CONSOLE_CMD_MODE_SELF, num_words, words,
				      0, 0);
	} else {
		r = CONSOLE_ERROR;
	}

	for (i = 0; i < num_words; i++)
		free(words[i]);

	if (r == CONSOLE_QUIT)
		return r;

	return r;
}

static int console_delete_cmd(struct console_def *console,
			      struct console_cmd *cmd)
{
	struct console_cmd *p, *c, *start, *t;

	for (c = cmd->children; c;) {
		p = c->next;
		console_delete_cmd(console, c);
		c = p;
	}

	if (cmd->parent == NULL) {
		start = console->cmds_root;
	} else {
		start = cmd->parent->children;
	}

	for (t = start; t; t = t->next) {
		if (t == cmd && t == start) {
			if (start == console->cmds_root) {
				console->cmds_root = t->next;
			} else {
				cmd->parent->children = t->next;
			}

			if (console->cmds_current == t) {
				if (t->parent && t->parent->parent) {
					console->cmds_current =
						t->parent->parent->children;
					console_set_promptstr(
						console,
						console_cmd_name2(
							console,
							console->cmds_current
								->parent));
				} else {
					console->cmds_current =
						console->cmds_root;
					console_set_promptstr(console, NULL);
				}
			}
		}

		if (t->next == cmd)
			t->next = cmd->next;
	}

	free(cmd->cmd);
	if (cmd->help)
		free(cmd->help);
	free(cmd);

	return 0;
}

static int _console_loop(struct console_def *console)
{
	unsigned char c;
	int l, oldl = 0, is_telnet_option = 0, skip = 0, esc = 0;
	int cursor = 0, insertmode = 1;
	char *cmd = NULL, *oldcmd = 0;

	console_build_shortest(console, console->cmds_root);

	console_free_history(console);

	if ((cmd = malloc(CONSOLE_MAX_CMD_BUFFER_LEN)) == NULL)
		return CONSOLE_ERROR;

	memset(cmd, 0, CONSOLE_MAX_CMD_BUFFER_LEN);
	while (1) {
		signed int in_history = 0;
		int lastchar = 0;

		console->showprompt = 1;

		if (oldcmd) {
			l = cursor = oldl;
			oldcmd[l] = 0;
			console->showprompt = 1;
			oldcmd = NULL;
			oldl = 0;
		} else {
			memset(cmd, 0, CONSOLE_MAX_CMD_BUFFER_LEN);
			l = 0;
			cursor = 0;
		}

		while (1) {
			if (console->showprompt) {
				//nprintf("\r\n", 2);

				show_prompt(console);
				nprintf(cmd, l);
				if (cursor < l) {
					int n = l - cursor;
					while (n--)
						nprintf("\b", 1);
				}

				console->showprompt = 0;
			}

			c = getchar();

			if (c == 0)
				continue;

			if (skip) {
				skip--;
				continue;
			}

			if (c == 255 && !is_telnet_option) {
				is_telnet_option++;
				continue;
			}

			if (is_telnet_option) {
				if (c >= 251 && c <= 254) {
					is_telnet_option = c;
					continue;
				}

				if (c != 255) {
					is_telnet_option = 0;
					continue;
				}

				is_telnet_option = 0;
			}

			/* handle ANSI arrows */
			if (esc) {
				if (esc == '[') {
					/* remap to readline control codes */
					switch (c) {
					case 'A': /* Up */
						c = CTRL('P');
						break;

					case 'B': /* Down */
						c = CTRL('N');
						break;

					case 'C': /* Right */
						c = CTRL('F');
						break;

					case 'D': /* Left */
						c = CTRL('B');
						break;

					default:
						c = 0;
					}

					esc = 0;
				} else {
					esc = (c == '[') ? c : 0;
					continue;
				}
			}

			if (c == 0)
				continue;

			if (c == '\r' || c == '\n') {
				nprintf("\r\n", 2);
				break;
			}

			if (c == 27) {
				esc = 1;
				continue;
			}

			if (c == CTRL('C')) {
				nprintf("\r\n", 2);
				strncpy(cmd, "\r\n",
					CONSOLE_MAX_CMD_BUFFER_LEN);
				lastchar = c;
				break;
			}

			/* back word, backspace/delete */
			if (c == CTRL('W') || c == CTRL('H') || c == 0x7f) {
				int back = 0;

				if (c == CTRL('W')) { /* word */
					int nc = cursor;

					if (l == 0 || cursor == 0)
						continue;

					while (nc && cmd[nc - 1] == ' ') {
						nc--;
						back++;
					}

					while (nc && cmd[nc - 1] != ' ') {
						nc--;
						back++;
					}
				} else { /* char */
					if (l == 0 || cursor == 0) {
						nprintf("\a", 1);
						continue;
					}

					back = 1;
				}

				if (back) {
					while (back--) {
						if (l == cursor) {
							cmd[--cursor] = 0;
							nprintf("\b \b", 3);
						} else {
							int i;
							cursor--;
							for (i = cursor; i <= l;
							     i++)
								cmd[i] = cmd[i +
									     1];

							nprintf("\b", 1);
							nprintf(cmd + cursor,
								strlen(cmd +
								       cursor));
							nprintf(" ", 1);

							for (i = 0;
							     i <=
							     (int)strlen(
								     cmd +
								     cursor);
							     i++)
								nprintf("\b",
									1);
						}
						l--;
					}

					continue;
				}
			}

			/* redraw */
			if (c == CTRL('L')) {
				int i;
				int cursorback = l - cursor;

				nprintf("\r\n", 2);
				show_prompt(console);
				nprintf(cmd, l);

				for (i = 0; i < cursorback; i++)
					nprintf("\b", 1);

				continue;
			}

			/* clear line */
			if (c == CTRL('U')) {
				console_clear_line(cmd, l, cursor);
				l = cursor = 0;
				continue;
			}

			/* kill to EOL */
			if (c == CTRL('K')) {
				int c;

				if (cursor == l)
					continue;

				for (c = cursor; c < l; c++)
					nprintf(" ", 1);

				for (c = cursor; c < l; c++)
					nprintf("\b", 1);

				memset(cmd + cursor, 0, l - cursor);
				l = cursor;
				continue;
			}

			/* EOT */
			if (c == CTRL('D')) {
				if (l)
					continue;

				strcpy(cmd, "quit");
				l = cursor = strlen(cmd);
				nprintf("quit\r\n", l + 2);
				break;
			}

			/* disable */
			if (c == CTRL('Z')) {
				console_clear_line(cmd, l, cursor);
				console_set_promptstr(console, NULL);
				console->showprompt = 1;
				continue;
			}

			/* TAB completion */
			if (c == CTRL('I')) {
				char *completions[128];
				int num_completions = 0;

				if (cursor != l)
					continue;

				num_completions = console_get_completions(
					console, cmd, completions, 128);
				if (num_completions == 0) {
					nprintf("\a", 1);
				} else if (num_completions == 1) {
					/* Single completion */
					for (; l > 0; l--, cursor--) {
						if (cmd[l - 1] == ' ' ||
						    cmd[l - 1] == '|')
							break;
						nprintf("\b", 1);
					}
					strcpy((cmd + l), completions[0]);
					l += strlen(completions[0]);
					cmd[l++] = ' ';
					cursor = l;
					nprintf(completions[0],
						strlen(completions[0]));
					nprintf(" ", 1);
				} else if (lastchar == CTRL('I')) {
					/* double tab */
					int i;
					nprintf("\r\n", 2);
					for (i = 0; i < num_completions; i++) {
						nprintf(completions[i],
							strlen(completions[i]));
						if (i % 4 == 3)
							nprintf("\r\n", 2);
						else
							nprintf("	 ", 1);
					}
					if (i % 4 != 3)
						nprintf("\r\n", 2);
					console->showprompt = 1;
				} else {
					/* More than one completion */
					lastchar = c;
					nprintf("\a", 1);
				}
				continue;
			}

			/* history */
			if (c == CTRL('P') || c == CTRL('N')) {
				int history_found = 0;
				if (c == CTRL('P')) { /* Up */
					in_history--;
					if (in_history < 0) {
						for (in_history =
							     CONSOLE_MAX_HISTORY -
							     1;
						     in_history >= 0;
						     in_history--) {
							if (console->history
								    [in_history]) {
								history_found =
									1;
								break;
							}
						}
					} else {
						if (console->history[in_history])
							history_found = 1;
					}
				} else { /* Down */
					in_history++;
					if (in_history >= CONSOLE_MAX_HISTORY ||
					    !console->history[in_history]) {
						int i = 0;
						for (i = 0;
						     i < CONSOLE_MAX_HISTORY;
						     i++) {
							if (console->history[i]) {
								in_history = i;
								history_found =
									1;
								break;
							}
						}
					} else {
						if (console->history[in_history])
							history_found = 1;
					}
				}
				if (history_found &&
				    console->history[in_history]) {
					/* Show history item */
					console_clear_line(cmd, l, cursor);
					strncpy(cmd,
						console->history[in_history],
						CONSOLE_MAX_CMD_BUFFER_LEN - 1);
					l = cursor = strlen(cmd);
					nprintf(cmd, l);
				}

				continue;
			}

			/* left/right cursor motion */
			if (c == CTRL('B') || c == CTRL('F')) {
				if (c == CTRL('B')) { /* Left */
					if (cursor) {
						nprintf("\b", 1);
						cursor--;
					}
				} else { /* Right */
					if (cursor < l) {
						nprintf(&cmd[cursor], 1);
						cursor++;
					}
				}

				continue;
			}

			/* start of line */
			if (c == CTRL('A')) {
				if (cursor) {
					nprintf("\r", 1);
					show_prompt(console);

					cursor = 0;
				}

				continue;
			}

			/* end of line */
			if (c == CTRL('E')) {
				if (cursor < l) {
					nprintf(&cmd[cursor], l - cursor);
					cursor = l;
				}

				continue;
			}

			/* normal character typed */
			if (cursor == l) {
				/* append to end of line */
				cmd[cursor] = c;
				if (l < CONSOLE_MAX_CMD_BUFFER_LEN - 1) {
					l++;
					cursor++;
				} else {
					nprintf("\a", 1);
					continue;
				}
			} else {
				/* Middle of text */
				if (insertmode) {
					int i;
					/* Move everything one character to the right */
					if (l >= CONSOLE_MAX_CMD_BUFFER_LEN - 2)
						l--;
					for (i = l; i >= cursor; i--)
						cmd[i + 1] = cmd[i];
					/* Write what we've just added */
					cmd[cursor] = c;

					nprintf(&cmd[cursor], l - cursor + 1);
					for (i = 0; i < (l - cursor + 1); i++)
						nprintf("\b", 1);
					l++;
				} else {
					cmd[cursor] = c;
				}
				cursor++;
			}

			nprintf((const char *)&c, 1);

			oldcmd = 0;
			oldl = 0;
			lastchar = c;
		}

		if (l < 0)
			break;
		//if (!strcasecmp(cmd, "quit")) break;

		if (l == 0)
			continue;
		if (lastchar != CTRL('C') && cmd[l - 1] != '?' &&
		    strcasecmp(cmd, "history") != 0)
			console_add_history(console, cmd);

		if (_console_run_cmd(console, cmd) == CONSOLE_QUIT)
			break;
	}

	console_free_history(console);
	free_z(cmd);

	return CONSOLE_OK;
}

void console_set_hostname(const char *hostname)
{
	struct console_def *console = g_console;

	free_z(console->hostname);
	if (hostname && *hostname)
		console->hostname = strdup(hostname);
}

struct console_cmd *console_register_cmd(struct console_cmd *parent,
					 const char *cmd,
					 int (*callback)(int, char **),
					 unsigned int mode, const char *help)
{
	return _console_register_cmd(g_console, parent, cmd, callback, mode,
				     help);
}

int console_unregister_cmd(const char *cmd)
{
	struct console_def *console = g_console;
	struct console_cmd *c;

	if (!cmd)
		return -1;
	if (!console->cmds_root)
		return CONSOLE_OK;

	c = console_find_cmd(cmd);
	if (c == NULL)
		return -1;

	console_delete_cmd(console, c);

	return CONSOLE_OK;
}

int console_run_cmd(const char *cmd)
{
	return _console_run_cmd(g_console, cmd);
}

struct console_cmd *console_find_cmd(const char *cmd)
{
	return console_find_cmd_ext(g_console->cmds_root, cmd);
}


void console_init(const char *hostname)
{
	if (!(g_console = malloc(sizeof(struct console_def))))
		return;

	memset(g_console, 0, sizeof(struct console_def));

	console_set_promptchar(g_console, "# ");
	console_set_hostname(hostname);
	console_set_promptstr(g_console, NULL);

	console_register_cmd(NULL, "help", console_help, CONSOLE_CMD_MODE_ALL,
			     "Show available cmds");

	console_register_cmd(NULL, "exit", console_exit, CONSOLE_CMD_MODE_ALL,
			     "Exit from current cmd set");

	console_register_cmd(NULL, "history", console_history,
			     CONSOLE_CMD_MODE_ALL, "Show history cmds");

	return;
}

void console_start(void)
{
	_console_loop(g_console);

	return;
}

