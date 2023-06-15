
/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef _BOOTM_H
#define _BOOTM_H

#include <image.h>

struct cmd_tbl;

#define BOOTM_ERR_RESET		(-1)
#define BOOTM_ERR_OVERLAP		(-2)
#define BOOTM_ERR_UNIMPLEMENTED	(-3)

typedef int boot_os_fn(int flag, int argc, char *const argv[],
			bootm_headers_t *images);

int boot_selected_os(int argc, char *const argv[], int state,
		     bootm_headers_t *images, boot_os_fn *boot_fn);

boot_os_fn *bootm_os_get_boot_func(int os);
extern boot_os_fn do_bootm_linux;

int bootm_find_images(int flag, int argc, char *const argv[], unsigned long start,
		      unsigned long size);

int do_bootm_states(struct cmd_tbl *cmdtp, int flag, int argc,
		    char *const argv[], int states, bootm_headers_t *images,
		    int boot_progress);
int bootm(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);
#endif
