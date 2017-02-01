/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file
 * Core internal defines
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef ENABLE_NLS
#include <libintl.h>
#endif


/* ------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------*/

// Gettext defines 
#ifdef ENABLE_NLS
#define _(String) dgettext(PACKAGE, String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)
#else
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif //HAVE_GETTEXT


// Additional defines for function attributes (under GCC). 
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)) && ! defined(printf)
#define CP_GCC_PRINTF(format_idx, arg_idx) \
	__attribute__((format (printf, format_idx, arg_idx)))
#define CP_GCC_CONST __attribute__((const))
#define CP_GCC_NORETURN __attribute__((noreturn))
#else
#define CP_GCC_PRINTF(format_idx, arg_idx)
#define CP_GCC_CONST
#define CP_GCC_NORETURN
#endif


#endif //DEFINES_H_
