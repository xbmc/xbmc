/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
#ifndef OSCLCONFIG_UTIL_H_INCLUDED
#define OSCLCONFIG_UTIL_H_INCLUDED

#ifndef OSCLCONFIG_H_INCLUDED
#include "osclconfig.h"
#endif

#include <stdio.h> //sprintf
#include <time.h>     // OSCL clock
#include <sys/time.h> // timeval

#define OSCL_CLOCK_HAS_DRIFT_CORRECTION 0
#define OSCL_HAS_SYMBIAN_TIMERS 0
#define OSCL_HAS_SYMBIAN_MATH   0

#define OSCL_RAND_MAX           RAND_MAX

//Define system sleep call for the tick count test here.
#include <unistd.h>
#define SLEEP_ONE_SEC sleep(1)


#include "osclconfig_util_check.h"

#endif

