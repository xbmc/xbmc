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

//             O S C L C O N F I G _ M E M O R Y

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =




#ifndef OSCLCONFIG_MEMORY_H_INCLUDED
#define OSCLCONFIG_MEMORY_H_INCLUDED


#ifndef OSCLCONFIG_H_INCLUDED
#include "osclconfig.h"
#endif

#ifndef OSCLCONFIG_ANSI_MEMORY_H_INCLUDED
#include "osclconfig_ansi_memory.h"
#endif

/* OSCL_HAS_GLOBAL_NEW_DELETE - Enables or disables the definition of overloaded
 * global memory operators in oscl_mem.h
 *
 * Release Mode: OSCL_HAS_GLOBAL_NEW_DELETE 0
 * Debug Mode: OSCL_HAS_GLOBAL_NEW_DELETE 1
 */


#if (OSCL_RELEASE_BUILD)
#define OSCL_BYPASS_MEMMGT 1
#define OSCL_HAS_GLOBAL_NEW_DELETE 0
#else
#define OSCL_BYPASS_MEMMGT 1  //Temporarily disabling
#define OSCL_HAS_GLOBAL_NEW_DELETE 1
#endif

/* PVMEM_INST_LEVEL - Memory leak instrumentation level enables the compilation
 * of detailed memory leak info (filename + line number).
 * PVMEM_INST_LEVEL 0: Release mode.
 * PVMEM_INST_LEVEL 1: Debug mode.
 */

#if(OSCL_RELEASE_BUILD)
#define PVMEM_INST_LEVEL 0
#else
#define PVMEM_INST_LEVEL 1
#endif

#if(OSCL_HAS_GLOBAL_NEW_DELETE)
//Detect if <new> or <new.h> is included anyplace to avoid a compile error.
#if defined(_INC_NEW)
#error Duplicate New Definition!
#endif //_INC_NEW
#if defined(_NEW_)
#error Duplicate New Definition!
#endif //_NEW_
#endif //OSCL_HAS_GLOBAL_NEW_DELETE

#ifdef __cplusplus
#include <new> //for placement new
#endif //__cplusplus

//OSCL_HAS_HEAP_BASE_SUPPORT - Enables or disables overloaded memory operators in HeapBase class
#define OSCL_HAS_HEAP_BASE_SUPPORT 1

#define OSCL_HAS_SYMBIAN_MEMORY_FUNCS 0


#include "osclconfig_memory_check.h"


#endif
