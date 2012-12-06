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

//osclconfig: this build configuration file is for win32
#ifndef OSCLCONFIG_TIME_CHECK_H_INCLUDED
#define OSCLCONFIG_TIME_CHECK_H_INCLUDED


/**
OSCL_HAS_UNIX_TIME_FUNCS macro should be set to 1 if
the target platform supports unix time of day functions.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_UNIX_TIME_FUNCS
#error "ERROR: OSCL_HAS_UNIX_TIME_FUNCS has to be defined to either 1 or 0"
#endif

/**
OsclBasicTimeStruct type should be defined to the platform-specific
time of day type.
*/
typedef OsclBasicTimeStruct __Validate__BasicTimeStruct__;

/**
OsclBasicDateTimeStruct type should be defined to the platform-specific
date + time type.
*/
typedef OsclBasicDateTimeStruct __Validate__BasicTimeDateStruct__;

#endif //OSCLCONFIG_TIME_CHECK_H_INCLUDED


