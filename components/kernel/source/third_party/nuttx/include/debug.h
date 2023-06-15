/****************************************************************************
 * include/debug.h
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

#ifndef __INCLUDE_DEBUG_H
#define __INCLUDE_DEBUG_H

#define PRIdOFF     PRId64
#define PRIiOFF     PRIi64
#define PRIoOFF     PRIo64
#define PRIuOFF     PRIu32
#define PRIxOFF     PRIx64
#define PRIXOFF     PRIX64

#ifndef ELOG_OUTPUT_LVL
#define ELOG_OUTPUT_LVL                          ELOG_LVL_ERROR
#endif
#include <kernel/elog.h>
#define ferr log_e
#define fwarn log_w
#define finfo log_i

#  define i2cdebug     log_d
#  define i2cinfo     log_i
#  define i2cerr     log_e

#define pwminfo		log_i
#define pwmerr		log_e
#define DEBUGPANIC() assert(0)

#endif /* __INCLUDE_DEBUG_H */
