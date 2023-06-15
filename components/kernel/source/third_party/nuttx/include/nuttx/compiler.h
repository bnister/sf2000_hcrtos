/****************************************************************************
 * include/nuttx/compiler.h
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

#ifndef __INCLUDE_NUTTX_COMPILER_H
#define __INCLUDE_NUTTX_COMPILER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GCC-specific definitions *************************************************/

/* At present, only the following Zilog compilers are recognized */

/* Pre-processor */

/* Intrinsics */

/* No I-space access qualifiers */

/* C++ support */

/* Attributes
 *
 * The Zilog compiler does not support weak symbols
 */

/* The Zilog compiler does not support the noreturn, packed, naked
 * attributes.
 */

/* REVISIT: */

/* The Zilog compiler does not support the reentrant attribute */

/* Addressing.
 *
 * Z16F ZNEO:  Far is 24-bits; near is 16-bits of address.
 *             The supported model is (1) all code on ROM, and (2) all data
 *             and stacks in external (far) RAM.
 * Z8Encore!:  Far is 16-bits; near is 8-bits of address.
 *             The supported model is (1) all code on ROM, and (2) all data
 *             and stacks in internal (far) RAM.
 * Z8Acclaim:  In Z80 mode, all pointers are 16-bits.  In ADL mode, all
 *             pointers are 24 bits.
 */

/* ISO C11 supports anonymous (unnamed) structures and unions.  Zilog does
 * not support C11
 */

/* Older Zilog compilers support both types double and long long, but the
 * size is 32-bits (same as long and single precision) so it is safer to say
 * that they are not supported.  Later versions are more ANSII compliant and
 * simply do not support long long or double.
 */

/* Indicate that a local variable is not used */

/* ICCARM-specific definitions **********************************************/

/* Indicate that a local variable is not used */

/* For operatots __sfb() and __sfe() */

/* C++ support */

/* ISO C11 supports anonymous (unnamed) structures and unions.  Does
 * ICCARM?
 */

/* Unknown compiler *********************************************************/

#  define FAR
#  define NEAR
#  define DSEG
#  define CODE
#  define OK (0)
#  define ERROR (-1)
#  define TRUE 1

#define DEBUGASSERT assert

#  define begin_packed_struct
#  define end_packed_struct __attribute__ ((packed))

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_COMPILER_H */
