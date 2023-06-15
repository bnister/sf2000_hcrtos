/****************************************************************************
 * fs/fs_initialize.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <kernel/module.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <kernel/lib/fdt_api.h>

#include "inode/inode.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fs_initialize
 *
 * Description:
 *   This is called from the OS initialization logic to configure the file
 *   system.
 *
 ****************************************************************************/
extern struct filelist *nxsched_get_files(void);
int fs_initialize(void)
{
  /* Initial inode, file, and VFS data structures */

  inode_initialize();
  files_initlist(nxsched_get_files());
  return 0;
}

const char * __attribute__((weak)) fdt_get_stdio_path(void)
{
	return "/hcrtos/stdio";
}

int stdio_initalize(void)
{
	const char *path = NULL;
	int np;

	np = fdt_get_node_offset_by_path(fdt_get_stdio_path());
	assert(np >= 0);
	assert(fdt_get_property_string_index(np, "serial0", 0, &path) == 0);
	assert(path != NULL);

	np = fdt_node_probe_by_path(path);
	if (np < 0)
		np = fdt_node_probe_by_path("/hcrtos/uart_dummy");
	assert(np >= 0);
	assert(fdt_get_property_string_index(np, "devpath", 0, &path) == 0);

	assert(open(path, O_RDWR) == 0);
	assert(open(path, O_RDWR) == 1);
	assert(open(path, O_RDWR) == 2);

	return 0;
}

module_core(fs, fs_initialize, NULL, 0)
module_arch(stdio, stdio_initalize, NULL, 1)
