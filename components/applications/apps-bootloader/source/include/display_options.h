/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2015 Google, Inc
 *
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __DISPLAY_OPTIONS_H
#define __DISPLAY_OPTIONS_H

#include <stdint.h>

/**
 * print_size() - Print a size with a suffix
 *
 * Print sizes as "xxx KiB", "xxx.y KiB", "xxx MiB", "xxx.y MiB",
 * xxx GiB, xxx.y GiB, etc as needed; allow for optional trailing string
 * (like "\n")
 *
 * @size:	Size to print
 * @suffix	String to print after the size
 */
void print_size(uint64_t size, const char *suffix);

#endif
