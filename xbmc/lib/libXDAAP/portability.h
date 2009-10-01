/* portability stuff
 *
 * Copyright (c) 2004 David Hammerton
 * crazney@crazney.net
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#ifndef _PORTABILITY_H
#define _PORTABILITY_H

#if !defined(WIN32)  /* POSIX */

#define SYSTEM_POSIX

#include <sys/types.h>

#if !defined(HAVE_U_INT64_T) && defined(HAVE_UINT64_T)
 typedef uint64_t u_int64_t;
#endif
#if !defined(HAVE_U_INT32_T) && defined(HAVE_UINT32_T)
 typedef uint32_t u_int32_t;
#endif
#if !defined(HAVE_U_INT16_T) && defined(HAVE_UINT16_T)
 typedef uint16_t u_int16_t;
#endif
#if !defined(HAVE_U_INT8_T) && defined(HAVE_UINT8_T)
 typedef uint8_t u_int8_t;
#endif

#elif defined(WIN32)

#define SYSTEM_WIN32

#include <windows.h>
#include <time.h>

#define vsnprintf _vsnprintf

typedef signed __int64 int64_t;
typedef unsigned __int64 u_int64_t;

typedef signed int int32_t;
typedef unsigned int u_int32_t;

typedef signed short int16_t;
typedef unsigned short u_int16_t;

typedef signed char int8_t;
typedef unsigned char u_int8_t;

#else /* WIN32 */

#include <windows.h>

#include <time.h>
typedef INT64 int64_t;
typedef UINT64 u_int64_t;

typedef signed int int32_t;
typedef unsigned int u_int32_t;

typedef signed short int16_t;
typedef unsigned short u_int16_t;

typedef signed char int8_t;
typedef unsigned char u_int8_t;

#endif

#endif /* _PORTABILITY_H */


