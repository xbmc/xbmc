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

//         O S C L C O N F I G _ U N I X _ C O M M O N

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig_limits_typedefs.h
 *  \brief This file contains common typedefs based on the ANSI C limits.h header
 *
 *  This header file should work for any ANSI C compiler to determine the
 *  proper native C types to use for OSCL integer types.
 */


#ifndef OSCLCONFIG_UNIX_ANDROID_H_INCLUDED
#define OSCLCONFIG_UNIX_ANDROID_H_INCLUDED


// system header files
#include <stdlib.h> // abort
#include <stdarg.h> // va_list
#include <sys/types.h>
#include <stdio.h>
//#include <wchar.h>
#include <string.h>
#include <unistd.h> //for sleep
#include <pthread.h>
#include <ctype.h> // for tolower and toupper
#ifdef __cplusplus
#include <new> //for placement new
#endif
#include <math.h>

#define OSCL_DISABLE_INLINES                0

#define OSCL_HAS_ANSI_STDLIB_SUPPORT        1
#define OSCL_HAS_ANSI_MATH_SUPPORT          1
#define OSCL_HAS_GLOBAL_VARIABLE_SUPPORT    1
#define OSCL_HAS_ANSI_STRING_SUPPORT        1
#define OSCL_HAS_ANSI_WIDE_STRING_SUPPORT   0
#define OSCL_HAS_ANSI_STDIO_SUPPORT         1

#define OSCL_MEMFRAG_PTR_BEFORE_LEN         1

#define OSCL_HAS_UNIX_SUPPORT               1
#define OSCL_HAS_MSWIN_SUPPORT              0
#define OSCL_HAS_MSWIN_PARTIAL_SUPPORT    0
#define OSCL_HAS_SYMBIAN_SUPPORT            0
#define OSCL_HAS_IPHONE_SUPPORT        0


// 64-bit int
#define OSCL_NATIVE_INT64_TYPE     int64_t
#define OSCL_NATIVE_UINT64_TYPE    uint64_t
#define INT64(x) x##LL
#define UINT64(x) x##ULL
#define INT64_HILO(high,low) ((((high##LL))<<32)|low)
#define UINT64_HILO(high,low) ((((high##ULL))<<32)|low)

// character set.
#define OSCL_HAS_UNICODE_SUPPORT            1
#define OSCL_NATIVE_WCHAR_TYPE wchar_t
#if (OSCL_HAS_UNICODE_SUPPORT)
#define _STRLIT(x) L ## x
#else
#define _STRLIT(x) x
#endif
#define _STRLIT_CHAR(x) x
#define _STRLIT_WCHAR(x) L ## x

// Thread-local storage.  Unix has keyed TLS.
#define OSCL_HAS_TLS_SUPPORT    1
#define OSCL_TLS_IS_KEYED 1
typedef pthread_key_t TOsclTlsKey ;
#define OSCL_TLS_KEY_CREATE_FUNC(key) (pthread_key_create(&key,NULL)==0)
#define OSCL_TLS_KEY_DELETE_FUNC(key) pthread_key_delete(key)
#define OSCL_TLS_STORE_FUNC(key,ptr) (pthread_setspecific(key,(const void*)ptr)==0)
#define OSCL_TLS_GET_FUNC(key) pthread_getspecific(key)

//Basic lock
#define OSCL_HAS_BASIC_LOCK 1
#include <pthread.h>
typedef pthread_mutex_t TOsclBasicLockObject;

#endif // OSCLCONFIG_UNIX_COMMON_H_INCLUDED

