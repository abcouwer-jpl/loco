/***********************************************************************
 * Copyright 2020 by the California Institute of Technology
 * ALL RIGHTS RESERVED. United States Government Sponsorship acknowledged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file        loco_cfs_global_types.h
 * @date        2020-06-09
 * @author      Neil Abcouwer
 * @brief       Definition of global types for testing loco
 *
 * loco_conf_types_pub.h defines public types used by public loco functions.
 * and follows the common guideline, as expressed in MISRA
 * directive 4.6, "typedefs that indicate size and signedness should be used
 * in place of the basic numerical types".
 *
 * loco_types_pub.h includes loco_conf_global_types.h, which must define
 * these types.  For the purposes of working in the NASA core FSW framework
 * (cFS), test/loco_cfs_global_types.h is copied to
 * include/loco/loco_conf_global_types.h.
 * It includes common_types.h from cFS' osal, https://github.com/nasa/osal
 *
 */

#ifndef LOCO_CONF_GLOBAL_TYPES_H
#define LOCO_CONF_GLOBAL_TYPES_H

#include "common_types.h" // defines NULL and sized types

#define LOCO_COMPILE_ASSERT(test, msg) CompileTimeAssert(test, msg)

typedef int16 I16;
typedef int32 I32;
typedef uint8 U8;
typedef uint16 U16;
typedef uint32 U32;

LOCO_COMPILE_ASSERT(sizeof(I16) == 2, I16BadSize);
LOCO_COMPILE_ASSERT(sizeof(I32) == 4, I32BadSize);
LOCO_COMPILE_ASSERT(sizeof(U8)  == 1,  U8BadSize);
LOCO_COMPILE_ASSERT(sizeof(U16) == 2, U16BadSize);
LOCO_COMPILE_ASSERT(sizeof(U32) == 4, U32BadSize);

#endif /* LOCO_CONF_GLOBAL_TYPES_H */
