/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2011 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mutex.h>
#include <errno.h>
#include <yaffs_guts.h>
#include <yaffs_trace.h>

static DEFINE_MUTEX(__yaffs_mutex);

unsigned int yaffs_trace_mask = 0;

void yaffsfs_SetError(int err)
{
	errno = -err;
}

int yaffsfs_GetLastError(void)
{
	return errno;
}

void yaffsfs_Lock(void)
{
	mutex_lock(&__yaffs_mutex);
}

void yaffsfs_Unlock(void)
{
	mutex_unlock(&__yaffs_mutex);
}

u32 yaffsfs_CurrentTime(void)
{
	return 0;
}

void *yaffsfs_malloc(size_t size)
{
	return malloc(size);
}

void yaffsfs_free(void *ptr)
{
	free(ptr);
}

int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
	return 0;
}

void yaffs_bug_fn(const char *file_name, int line_no)
{
}
