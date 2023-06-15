#include <stdio.h>
#include <stdarg.h>
#include "nsh.h"
#include "nsh_console.h"

const char g_fmtsyntax[]         = "nsh: %s: syntax error\n";
const char g_fmtarginvalid[]     = "nsh: %s: argument invalid\n";
const char g_fmttoomanyargs[]    = "nsh: %s: too many arguments\n";
const char g_fmtcmdoutofmemory[] = "nsh: %s: out of memory\n";
const char g_fmtcmdfailed[]      = "nsh: %s: %s failed: %d\n";
const char g_fmtnosuch[]         = "nsh: %s: no such %s: %s\n";
const char g_fmtargrange[]       = "nsh: %s: value out of range\n";
const char g_fmtargrequired[]    = "nsh: %s: missing required argument(s)\n";
const char g_fmtsignalrecvd[]    = "nsh: %s: Interrupted by signal\n";

static ssize_t nsh_consolewrite(FAR struct nsh_vtbl_s *vtbl,
                                FAR const void *buffer, size_t nbytes)
{
  ssize_t ret;

  /* Write the data to the output stream */

  ret = fwrite(buffer, 1, nbytes, stdout);

  /* Flush the data to the output stream */

  fflush(stdout);

  return ret;
}

static int nsh_consoleoutput(FAR struct nsh_vtbl_s *vtbl,
                             FAR const char *fmt, ...)
{
  va_list ap;
  int ret;

  /* The stream is open in a lazy fashion.  This is done because the file
   * descriptor may be opened on a different task than the stream.  The
   * actual open will then occur with the first output from the new task.
   */

  va_start(ap, fmt);
  ret = vfprintf(stdout, fmt, ap);
  va_end(ap);

  return ret;
}

static int nsh_erroroutput(FAR struct nsh_vtbl_s *vtbl,
                           FAR const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vfprintf(stdout, fmt, ap);
  va_end(ap);

  return ret;
}

struct nsh_vtbl_s g_vtbl = {
      .write       = nsh_consolewrite,
      .output      = nsh_consoleoutput,
      .error       = nsh_erroroutput,
};

static int nsh(int argc, char **argv)
{
	return CONSOLE_OK;
}
#ifndef CONFIG_NSH_CMD_MODE_ALL
NSH_CONSOLE_CMD(nsh, NULL, nsh, CONSOLE_CMD_MODE_SELF, "Enter NSH cmds")
#endif
