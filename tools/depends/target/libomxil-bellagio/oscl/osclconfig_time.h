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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//   O S C L C O N F I G _ T I M E   ( T I M E - D E F I N I T I O N S )

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =




#ifndef OSCLCONFIG_TIME_H_INCLUDED
#define OSCLCONFIG_TIME_H_INCLUDED


#ifndef OSCLCONFIG_H_INCLUDED
#include "osclconfig.h"
#endif

// system header files
#include <time.h> // timeval
#include <sys/time.h> // timercmp
#include <unistd.h>



#define OSCL_HAS_UNIX_TIME_FUNCS        1

typedef struct timeval OsclBasicTimeStruct;
typedef tm      OsclBasicDateTimeStruct;

#include "osclconfig_time_check.h"

#endif
