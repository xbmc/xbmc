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

//     O S C L C O N F I G   ( P L A T F O R M   C O N F I G   I N F O )

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig.h
 *  \brief This file contains configuration information for the linux platform
 *
 */

#ifndef OSCLCONFIG_H_INCLUDED
#define OSCLCONFIG_H_INCLUDED

// system includes for dynamic registry
#include <dirent.h>
#include <dlfcn.h>

#define OSCL_HAS_ANDROID_SUPPORT 1
#define OSCL_HAS_ANDROID_FILE_IO_SUPPORT 1

#define OSCL_EXPORT_REF __attribute__ ((visibility("default")))
#define OSCL_IMPORT_REF __attribute__ ((visibility("default")))

// include common include for determining sizes from limits.h
#include "osclconfig_limits_typedefs.h"

//This switch turns off some profiling and debug settings
#ifndef OSCL_RELEASE_BUILD
#ifdef NDEBUG
#define OSCL_RELEASE_BUILD 1
#else
#define OSCL_RELEASE_BUILD 0
#endif
#endif

#ifndef PVLOGGER_INST_LEVEL
#if (OSCL_RELEASE_BUILD)
//Release mode logging - should be kept minimum
#define PVLOGGER_INST_LEVEL 2
#else
//full logging
#define PVLOGGER_INST_LEVEL 5
#endif
#endif

// include common unix definitions
#include "osclconfig_unix_android.h"

// define the suffix for unsigned constants
#define OSCL_UNSIGNED_CONST(x) x##u

// override the common definition for
#undef OSCL_NATIVE_UINT64_TYPE
#define OSCL_NATIVE_UINT64_TYPE    u_int64_t

// include the definitions for the processor
#include "osclconfig_ix86.h"

// the syntax for explicitly calling the destructor varies on some platforms
// below is the default syntax as defined by another ARM project
#define OSCL_TEMPLATED_DESTRUCTOR_CALL(type,simple_type) ~type ()


/* The __TFS__ macro is used to optionally expand to "<>" depending on the
 * compiler.  Some compilers require it to indicate that the friend function
 * is a template function as specified in the standard, but others don't
 * like it so it will handled with a macro expansion that depends on the
 * compiler.
 */
#define __TFS__ <>

#define OSCL_HAS_PRAGMA_PACK 0
#define OSCL_HAS_PACKED_STRUCT 1
#define OSCL_PACKED_VAR(x)  x __attribute__((packed))
#define OSCL_PACKED_STRUCT_BEGIN
#define OSCL_PACKED_STRUCT_END  __attribute__((packed))

//set this to 1 to enable OSCL_ASSERT in release builds.
#define OSCL_ASSERT_ALWAYS 0


// check all osclconfig required macros are defined
#include "osclconfig_check.h"

#endif
