/*
    $Id$

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002,2003 Rocky Bernstein <rocky@panix.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __CDIO_TYPES_H__
#define __CDIO_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* provide some C99 definitions */

#include "config.h"

#if defined(HAVE_SYS_TYPES_H) 
#include <sys/types.h>
#endif 

#if defined(HAVE_STDINT_H)
# include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
# include "inttypes.h"
#elif defined(AMIGA) || defined(__linux__)
  typedef u_int8_t uint8_t;
  typedef u_int16_t uint16_t;
  typedef u_int32_t uint32_t;
  typedef u_int64_t uint64_t;
#else
  /* warning ISO/IEC 9899:1999 <stdint.h> was missing and even <inttypes.h> */
  /* fixme */
#endif /* HAVE_STDINT_H */
  
  /* default HP/UX macros are broken */
#if defined(__hpux__)
# undef UINT16_C
# undef UINT32_C
# undef UINT64_C
# undef INT64_C
#endif

#if defined (MINGW32)
  typedef int ssize_t;
#endif
  
  /* if it's still not defined, take a good guess... should work for
     most 32bit and 64bit archs */
  
#ifndef UINT16_C
# define UINT16_C(c) c ## U
#endif
  
#ifndef UINT32_C
# if defined (SIZEOF_INT) && SIZEOF_INT == 4
#  define UINT32_C(c) c ## U
# elif defined (SIZEOF_LONG) && SIZEOF_LONG == 4
#  define UINT32_C(c) c ## UL
# else
#  define UINT32_C(c) c ## U
# endif
#endif
  
#ifndef UINT64_C
# if defined (SIZEOF_LONG) && SIZEOF_LONG == 8
#  define UINT64_C(c) c ## UL
# elif defined (SIZEOF_INT) && SIZEOF_INT == 8
#  define UINT64_C(c) c ## U
# else
#  define UINT64_C(c) c ## ULL
# endif
#endif
  
#ifndef INT64_C
# if defined (SIZEOF_LONG) && SIZEOF_LONG == 8
#  define INT64_C(c) c ## L
# elif defined (SIZEOF_INT) && SIZEOF_INT == 8
#  define INT64_C(c) c 
# else
#  define INT64_C(c) c ## LL
# endif
#endif

#if defined(HAVE_STDBOOL_H)
#include <stdbool.h>
#else
  /* ISO/IEC 9899:1999 <stdbool.h> missing -- enabling workaround */
  
# ifndef __cplusplus
  typedef enum
    {
      false = 0,
      true = 1
    } _Bool;
  
#  define false   false
#  define true    true
#  define bool _Bool
# endif
#endif
  
  /* some GCC optimizations -- gcc 2.5+ */
  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define GNUC_PRINTF( format_idx, arg_idx )              \
  __attribute__((format (printf, format_idx, arg_idx)))
#define GNUC_SCANF( format_idx, arg_idx )               \
  __attribute__((format (scanf, format_idx, arg_idx)))
#define GNUC_FORMAT( arg_idx )                  \
  __attribute__((format_arg (arg_idx)))
#define GNUC_NORETURN                           \
  __attribute__((noreturn))
#define GNUC_CONST                              \
  __attribute__((const))
#define GNUC_UNUSED                             \
  __attribute__((unused))
#define GNUC_PACKED                             \
  __attribute__((packed))
#else   /* !__GNUC__ */
#define GNUC_PRINTF( format_idx, arg_idx )
#define GNUC_SCANF( format_idx, arg_idx )
#define GNUC_FORMAT( arg_idx )
#define GNUC_NORETURN
#define GNUC_CONST
#define GNUC_UNUSED
#define GNUC_PACKED
#endif  /* !__GNUC__ */
  
#if defined(__GNUC__)
  /* for GCC we try to use GNUC_PACKED */
# define PRAGMA_BEGIN_PACKED
# define PRAGMA_END_PACKED
#elif defined(HAVE_ISOC99_PRAGMA)
  /* should work with most EDG-frontend based compilers */
# define PRAGMA_BEGIN_PACKED _Pragma("pack(1)")
# define PRAGMA_END_PACKED   _Pragma("pack()")
#else /* neither gcc nor _Pragma() available... */
  /* ...so let's be naive and hope the regression testsuite is run... */
# define PRAGMA_BEGIN_PACKED
# define PRAGMA_END_PACKED
#endif
  
  /*
   * user directed static branch prediction gcc 2.96+
   */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 95)
# define GNUC_LIKELY(x)   __builtin_expect((x),true)
# define GNUC_UNLIKELY(x) __builtin_expect((x),false)
#else 
# define GNUC_LIKELY(x)   (x) 
# define GNUC_UNLIKELY(x) (x)
#endif
  
#ifndef NULL
# define NULL ((void*) 0)
#endif
  
  /* our own offsetof()-like macro */
#define __cd_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
  
  /* In many structures on the disk a sector address is stored as a
     BCD-encoded mmssff in three bytes. */
  PRAGMA_BEGIN_PACKED
  typedef struct {
    uint8_t m, s, f;
  } GNUC_PACKED msf_t;
  PRAGMA_END_PACKED
  
#define msf_t_SIZEOF 3
  
  /* type used for bit-fields in structs (1 <= bits <= 8) */
#if defined(__GNUC__)
  /* this is strict ISO C99 which allows only 'unsigned int', 'signed
     int' and '_Bool' explicitly as bit-field type */
  typedef unsigned int bitfield_t;
#else
  /* other compilers might increase alignment requirements to match the
     'unsigned int' type -- fixme: find out how unalignment accesses can
     be pragma'ed on non-gcc compilers */
  typedef uint8_t bitfield_t;
#endif
  
  /* The type of a Logical Block Address. */
  typedef uint32_t lba_t;
  
  /* The type of an Logical Sector Number. */
  typedef uint32_t lsn_t;
  
  /* The type of an track number. */
  typedef uint8_t track_t;
  
  /*! 
    Constant for invalid track number
  */
#define CDIO_INVALID_TRACK   0xFF
  
  /*! 
    Constant for invalid LBA
  */
#define CDIO_INVALID_LBA   0xFFFFFFFF
  
  /*! 
    Constant for invalid LSN
  */
#define CDIO_INVALID_LSN   0xFFFFFFFF
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_TYPES_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
