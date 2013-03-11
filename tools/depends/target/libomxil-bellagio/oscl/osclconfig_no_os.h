/* ------------------------------------------------------------------
 * Copyright (C) 1998-2010 PacketVideo
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

#ifndef OSCLCONFIG_NO_OS_H_INCLUDED
#define OSCLCONFIG_NO_OS_H_INCLUDED

/*! \addtogroup osclconfig OSCL config
 *
 * @{
 */

//a file to turn off ALL os-specific switches.

//osclconfig
#define OSCL_HAS_UNIX_SUPPORT               0
#define OSCL_HAS_MSWIN_SUPPORT              0
#define OSCL_HAS_MSWIN_PARTIAL_SUPPORT      0
#define OSCL_HAS_SYMBIAN_SUPPORT            0
#define OSCL_HAS_SAVAJE_SUPPORT             0
#define OSCL_HAS_PV_C_OS_SUPPORT            0
#define OSCL_HAS_ANDROID_SUPPORT            0
#define OSCL_HAS_IPHONE_SUPPORT             0

//osclconfig_error
#define OSCL_HAS_SYMBIAN_ERRORTRAP 0

//osclconfig_memory
#define OSCL_HAS_SYMBIAN_MEMORY_FUNCS 0
#define OSCL_HAS_PV_C_OS_API_MEMORY_FUNCS 0

//osclconfig_time
#define OSCL_HAS_PV_C_OS_TIME_FUNCS 0
#define OSCL_HAS_UNIX_TIME_FUNCS    0

//osclconfig_util
#define OSCL_HAS_SYMBIAN_TIMERS 0
#define OSCL_HAS_SYMBIAN_MATH   0

//osclconfig_proc
#define OSCL_HAS_SYMBIAN_SCHEDULER 0
#define OSCL_HAS_SEM_TIMEDWAIT_SUPPORT 0
#define OSCL_HAS_PTHREAD_SUPPORT 0

//osclconfig_io
#define OSCL_HAS_SYMBIAN_COMPATIBLE_IO_FUNCTION 0
#define OSCL_HAS_SAVAJE_IO_SUPPORT 0
#define OSCL_HAS_SYMBIAN_SOCKET_SERVER 0
#define OSCL_HAS_SYMBIAN_DNS_SERVER 0
#define OSCL_HAS_BERKELEY_SOCKETS 0


/*! @} */

#endif // OSCLCONFIG_CHECK_H_INCLUDED


