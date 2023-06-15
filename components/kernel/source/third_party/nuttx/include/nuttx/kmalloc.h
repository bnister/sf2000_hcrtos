/****************************************************************************
 * include/nuttx/kmalloc.h
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

#ifndef __INCLUDE_NUTTX_KMALLOC_H
#define __INCLUDE_NUTTX_KMALLOC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C"
{
#endif
/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* If this the kernel phase of a kernel build, and there are only user-space
 * allocators, then the following are defined in userspace.h as macros that
 * call into user-space via a header at the beginning of the user-space blob.
 */

extern void *zalloc(size_t size);
#  define kmm_calloc(n,s)        calloc(n,s);
#  define kmm_malloc(s)          malloc(s)
#  define kmm_zalloc(s)          zalloc(s)
#  define kmm_realloc(p,s)       realloc(p,s)
#  define kmm_memalign(a,s)      memalign(a,s)
#  define kmm_free(p)            free(p)

#  define kumm_calloc(n,s)        calloc(n,s);
#  define kumm_malloc(s)          malloc(s)
#  define kumm_zalloc(s)          zalloc(s)
#  define kumm_realloc(p,s)       realloc(p,s)
#  define kumm_memalign(a,s)      memalign(a,s)
#  define kumm_free(p)            free(p)

#if defined(__cplusplus)
}
#endif

#endif /* __INCLUDE_NUTTX_KMALLOC_H */
