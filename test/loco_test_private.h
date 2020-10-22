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
 * @file        loco_test_private.h
 * @date        2020-06-05
 * @author      Neil Abcouwer
 * @brief       Definition private macros for loco functions
 *
 * A user must provide a loco_conf_private.h to define the following macros.
 * This file is copied for unit testing.
 */

#ifndef LOCO_CONF_PRIVATE_H
#define LOCO_CONF_PRIVATE_H

#include <assert.h>
#include <stdio.h>

// private loco functions are preceded by LOCO_PRIVATE
// this can be defined as nothing when compiling unit tests to allow access
// if your infrastructure does something similar, replace it here
#ifndef LOCO_PRIVATE
#define LOCO_PRIVATE static
#endif

/* This library was written with the philosophy that assertions be used to
   check anomalous conditions. Demosaic functions assert if inputs
   indicate there is a logic error.
   See http://spinroot.com/p10/rule5.html.

   This file defines the LOCO_ASSERT macros used in loco.c as simple
   c asserts, for testing. THe google test suite for the repo uses
   "death tests" to test that they are called appropriately.

   It is the intent that user of the loco library will copy an
   appropriate loco_conf.h to include/loco, such that asserts
   are defined appropriately for the application.

   Possible asserts:
        cstd asserts
        ROS_ASSERT
        BOOST_ASSERT
        (test) ? (void)(0)
               : send_asynchronous_safing_alert_to_other_process(), assert(0):

   Asserts could also be disabled, but this is is discouraged.
 */
#define LOCO_ASSERT(test) assert(test)
#define LOCO_ASSERT_1(test, arg1) assert(test)
#define LOCO_ASSERT_2(test, arg1, arg2) assert(test)
#define LOCO_ASSERT_3(test, arg1, arg2, arg3) assert(test)

// if your framework has messaging/logging infrastructure, replace here,
// or define as nothing to disable
#define LOCO_WARN0(id, fmt) \
    printf("WARNING " #id " "fmt"\n")
#define LOCO_WARN2(id, fmt, arg1, arg2) \
    printf("WARNING " #id " "fmt"\n", arg1, arg2)
#define LOCO_WARN4(id, fmt, arg1, arg2, arg3, arg4) \
    printf("WARNING " #id " "fmt"\n", arg1, arg2, arg3, arg4)
#define LOCO_WARN6(id, fmt, arg1, arg2, arg3, arg4, arg5, arg6) \
    printf("WARNING " #id " "fmt"\n", arg1, arg2, arg3, arg4, arg5, arg6)
#define LOCO_WARN7(id, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    printf("WARNING " #id " "fmt"\n", arg1, arg2, arg3, arg4, arg5, arg6, arg7)


#endif /* LOCO_CONF_PRIVATE_H */
