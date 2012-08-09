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

//               O S C L B A S E _ M A C R O S

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclbase OSCL Base
 *
 * @{
 */


/*! \file oscl_base_macros.h
    \brief This file defines common macros and constants for basic compilation support.
*/

#ifndef OSCL_BASE_MACROS_H_INCLUDED
#define OSCL_BASE_MACROS_H_INCLUDED

// Pick up any platform-specific definitions for the common
// macros.
#include "osclconfig.h"

// Define default values for the common macros
#ifndef OSCL_EXPORT_REF
#define OSCL_EXPORT_REF
#endif

#ifndef OSCL_IMPORT_REF
#define OSCL_IMPORT_REF
#endif

//! The NULL_TERM_CHAR is used to terminate c-style strings.
//static const char NULL_TERM_CHAR = '\0';
#ifndef NULL_TERM_CHAR
#define NULL_TERM_CHAR '\0'
#endif

//! if the NULL macro isn't already defined, then define it as zero.
#ifndef NULL
#define NULL (0)
#endif

#if (OSCL_DISABLE_INLINES)
#define OSCL_INLINE
#define OSCL_COND_EXPORT_REF OSCL_EXPORT_REF
#define OSCL_COND_IMPORT_REF OSCL_IMPORT_REF
#else
#define OSCL_INLINE inline
#define OSCL_COND_EXPORT_REF
#define OSCL_COND_IMPORT_REF
#endif

//this macro may not be defined in all configurations
//so a default is defined here.

//! Type casting macros
/*!
  \param type   Destination type of cast
  \param exp    Expression to cast
*/

#define OSCL_CONST_CAST(type,exp)           ((type)(exp))
#define OSCL_STATIC_CAST(type,exp)          ((type)(exp))
#define OSCL_REINTERPRET_CAST(type,exp)     ((type)(exp))
#define OSCL_DYNAMIC_CAST(type, exp)        ((type)(exp))
#define OSCL_VIRTUAL_BASE(type)               type


/**
 * The following two macros are used to avoid compiler warnings.
 *
 * OSCL_UNUSED_ARG(vbl) is used to "reference" an otherwise unused
 *   parameter or variable, often one which is used only in an
 *   OSCL_ASSERT and thus unreferenced in release mode
 * OSCL_UNUSED_RETURN(val) provides a "return" of a value, in places
 *   which will not actually be executed, such as after an
 *   OSCL_LEAVE or Thread::exit or abort.  The value needs to
 *   be of an appropriate type for the current function, though
 *   zero will usually suffice.  Note that OSCL_UNUSED_RETURN
 *   will not be necessary for 'void' functions, as there is no
 *   requirement for a value-return operation.
 */
#define OSCL_UNUSED_ARG(vbl) (void)(vbl)
#define OSCL_UNUSED_RETURN(value) return value

/* The __TFS__ macro is used to optionally expand to "<>" depending on the
 * compiler.  Some compilers require it to indicate that the friend function
 * is a template function as specified in the standard, but others don't
 * like it so it will handled with a macro expansion that depends on the
 * compiler.
 */
#ifndef __TFS__
#define __TFS__
#endif

#define OSCL_MIN(a,b) ((a) < (b) ? (a) : (b))
#define OSCL_MAX(a,b) ((a) > (b) ? (a) : (b))
#define OSCL_ABS(a) ((a) > (0) ? (a) : -(a))

// the syntax for explicitly calling the destructor varies on some platforms
// below is the default syntax as defined in the C++ standard
#ifndef OSCL_TEMPLATED_DESTRUCTOR_CALL
#define OSCL_TEMPLATED_DESTRUCTOR_CALL(type,simple_type) type :: ~simple_type ()
#endif


/*
 * The OSCL_UNSIGNED_CONST macro is used to optionally add a suffix to the
 * end of integer constants to identify them as unsigned constants.  It is
 * usually only necessary to do that for very large constants that are too
 * big to fit within the range of a signed integer. Some compilers will issue
 * warnings for that.  The default behavior will be to add no suffix.
 */

#ifndef OSCL_UNSIGNED_CONST
#define OSCL_UNSIGNED_CONST(x) x
#endif

/*
 * These macros are used by MTP to avoid byte aligning structures.
 */
#ifndef OSCL_PACKED_VAR
#define OSCL_PACKED_VAR     "error"
#endif


/* The following macros enumerate the possible values of the build flag PV_COMPILER */

#define EPV_ARM_GNUC  1 /* for GNU compiler */
#define EPV_ARM_RVCT  2 /* for RVCT compiler from ARM */
#define EPV_ARM_MSEVC 3 /* for MS Embedded VC compiler */

/* PV_COMPILER value must be set to build command.
   In addition, PV_CPU_ARCH_VERSION must be defined to take on an integer value that specifies the version of target
   ARM architecture. */


/*! @} */

#endif  // OSCL_BASE_MACROS_H_INCLUDED
