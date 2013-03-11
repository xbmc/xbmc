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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//     O S C L C O N F I G_ L I M I T S _ T Y P E D E F S

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig_limits_typedefs.h
 *  \brief This file contains common typedefs based on the ANSI C limits.h header
 *
 *  This header file should work for any ANSI C compiler to determine the
 *  proper native C types to use for OSCL integer types.
 */


#ifndef OSCLCONFIG_LIMITS_TYPEDEFS_H_INCLUDED
#define OSCLCONFIG_LIMITS_TYPEDEFS_H_INCLUDED


#include <limits.h>

// determine if char is signed or unsigned
#if ( CHAR_MIN == 0 )
#define OSCL_CHAR_IS_UNSIGNED 1
#define OSCL_CHAR_IS_SIGNED   0
#elif ( CHAR_MIN == SCHAR_MIN )
#define OSCL_CHAR_IS_UNSIGNED 0
#define OSCL_CHAR_IS_SIGNED   1
#else
#error "Cannot determine if char is signed or unsigned"
#endif


#if ( (CHAR_MAX == 255) || (CHAR_MAX == 127) )
typedef signed char int8;
typedef unsigned char uint8;
#else
#error "Cannot determine an 8-bit interger type"
#endif


#if ( SHRT_MAX == 32767 )
typedef short int16;
typedef unsigned short uint16;

#elif ( INT_MAX == 32767 )
typedef int int16;
typedef unsigned int uint16;

#else
#error "Cannot determine 16-bit integer type"
#endif



#if ( INT_MAX == 2147483647 )
typedef int int32;
typedef unsigned int uint32;

#elif ( LONG_MAX == 2147483647 )
typedef long int32;
typedef unsigned long uint32;

#else
#error "Cannot determine 32-bit integer type"
#endif



#endif
