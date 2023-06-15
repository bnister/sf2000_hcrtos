#ifndef _ADAPT_FCNTL_H
#define _ADAPT_FCNTL_H

#ifdef __cplusplus
extern "C" {
#endif
#include_next <fcntl.h>

#define _FNOINHERIT	0x40000
#define O_CLOEXEC	_FNOINHERIT

#define FFCNTL      (_FNONBLOCK | _FNDELAY | _FAPPEND | _FSYNC | _FASYNC)

#define F_GETPATH   15 /* Get the path of the file descriptor(BSD/macOS) */

#ifdef __cplusplus
}
#endif

#endif
