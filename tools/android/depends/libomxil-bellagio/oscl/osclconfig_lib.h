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

//     O S C L C O N F I G _ L I B  ( P L A T F O R M   C O N F I G   I N F O )

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig_lib.h
    \brief This file contains configuration information for the ANSI build.

*/

#ifndef OSCLCONFIG_LIB_H_INCLUDED
#define OSCLCONFIG_LIB_H_INCLUDED



//Set this to 1 to indicate this platform has oscllib support
#define OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT    1
#define PV_RUNTIME_LIB_FILENAME_EXTENSION "so"

//Set this to 1 to enable looking for debug versions of libraries.
//Use #ifndef to allow the compiler setting to override this definition
#ifndef OSCL_LIB_READ_DEBUG_LIBS
#if (OSCL_RELEASE_BUILD)
#define OSCL_LIB_READ_DEBUG_LIBS 0
#else
#define OSCL_LIB_READ_DEBUG_LIBS 1
#endif
#endif

// The path recursively from which the config files are picked up
#ifndef PV_DYNAMIC_LOADING_CONFIG_FILE_PATH
#ifdef ANDROID
#define PV_DYNAMIC_LOADING_CONFIG_FILE_PATH "/system/etc"
#else
#define PV_DYNAMIC_LOADING_CONFIG_FILE_PATH "./"
#endif
#endif

// check all osclconfig required macros are defined
#include "osclconfig_lib_check.h"

#endif // OSCLCONFIG_LIB_H_INCLUDED

