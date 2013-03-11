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
#ifndef OSCLCONFIG_UTIL_CHECK_H_INCLUDED
#define OSCLCONFIG_UTIL_CHECK_H_INCLUDED

/**
OSCL_HAS_SYMBIAN_TIMERS macro should be set to 1 if
the target platform supports Symbian timers (RTimer).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SYMBIAN_TIMERS
#error "ERROR: OSCL_HAS_SYMBIAN_TIMERS has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_SYMBIAN_MATH macro should be set to 1 if
the target platform supports Symbian <e32math.h> features.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SYMBIAN_MATH
#error "ERROR: OSCL_HAS_SYMBIAN_MATH has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_ANSI_MATH_SUPPORT macro should be set to 1 if
the target platform supports the ANSI C math functions (math.h)
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_ANSI_MATH_SUPPORT
#error "ERROR: OSCL_HAS_ANSI_MATH_SUPPORT has to be defined to either 1 or 0"
#endif

/**
OSCL_CLOCK_HAS_DRIFT_CORRECTION macro should be set to 1 if the target platform
has drift correction Otherwise it should be set to 0.
*/
#ifndef OSCL_CLOCK_HAS_DRIFT_CORRECTION
#error "ERROR: OSCL_CLOCK_HAS_DRIFT_CORRECTION has to be defined to either 1 or 0"
#endif

#endif // OSCLCONFIG_UTIL_CHECK_H_INCLUDED


