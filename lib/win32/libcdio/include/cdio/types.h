/*
    $Id: types.h,v 1.25 2005/01/04 04:33:36 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002, 2003, 2004 Rocky Bernstein <rocky@panix.com>

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

/** \file types.h 
 *  \brief  Common type definitions used pervasively in libcdio.
 */


#ifndef __CDIO_TYPES_H__
#define __CDIO_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* provide some C99 definitions */

#if defined(HAVE_SYS_TYPES_H) 
#include <sys/types.h>
#endif 

#if defined(HAVE_STDINT_H)
# include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
# include <inttypes.h>
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
    } _cdio_Bool;
  
#  define false   false
#  define true    true
#  define bool _cdio_Bool
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
  //__attribute__((packed))
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
  
  /*!
    \brief MSF (minute/second/frame) structure 

    One CD-ROMs addressing scheme especially used in audio formats
    (Red Book) is an address by minute, sector and frame which
    BCD-encoded in three bytes. An alternative format is an lba_t.
    
    @see lba_t
  */
#if defined(_XBOX) || defined(WIN32) 
  #pragma pack(1)
#else
  PRAGMA_BEGIN_PACKED
#endif
  struct msf_rec {
    uint8_t m, s, f;
  } GNUC_PACKED;
#if defined(_XBOX) || defined(WIN32)
  #pragma pack()
#else
  PRAGMA_END_PACKED
#endif
  
  typedef struct msf_rec msf_t;

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
  
  /*! The type of a Logical Block Address. We allow for an lba to be 
    negative to be consistent with an lba, although I'm not sure this
    this is possible.
      
   */
  typedef int32_t lba_t;
  
  /*! The type of a Logical Sector Number. Note that an lba lsn be negative
    and the MMC3 specs allow for a conversion of a negative lba

    @see msf_t
  */
  typedef int32_t lsn_t;
  
  /*! The type of a track number 0..99. */
  typedef uint8_t track_t;

  /*! The type of a session number 0..99. */
  typedef uint8_t session_t;
  
  /*! 
    Constant for invalid session number
  */
#define CDIO_INVALID_SESSION   0xFF
  
  /*! 
    Constant for invalid LBA. It is 151 less than the most negative
    LBA -45150. This provide slack for the 150-frame offset in
    LBA to LSN 150 conversions
  */
#define CDIO_INVALID_LBA    -45301
  
  /*! 
    Constant for invalid LSN
  */
#define CDIO_INVALID_LSN    CDIO_INVALID_LBA

  /*! 
    Number of ASCII bytes in a media catalog number (MCN).
  */
#define CDIO_MCN_SIZE       13

  /*! 
    Type to hold ASCII bytes in a media catalog number (MCN).
    We include an extra 0 byte so these can be used as C strings.
  */
  typedef char cdio_mcn_t[CDIO_MCN_SIZE+1];
  

  /*! 
    Number of ASCII bytes in International Standard Recording Codes (ISRC)
  */
#define CDIO_ISRC_SIZE       12

  /*! 
    Type to hold ASCII bytes in a media catalog number (MCN).
    We include an extra 0 byte so these can be used as C strings.
  */
  typedef char cdio_isrc_t[CDIO_ISRC_SIZE+1];

  typedef int cdio_fs_anal_t;

  /*! The type of an drive capability bit mask. See below for values*/
  typedef uint32_t cdio_drive_read_cap_t;
  typedef uint32_t cdio_drive_write_cap_t;
  typedef uint32_t cdio_drive_misc_cap_t;
  
  /*!
    \brief Drive types returned by cdio_get_drive_cap()
    
    NOTE: Setting a bit here means the presence of a capability.
  */ 

#define CDIO_DRIVE_CAP_ERROR          0x40000 /**< Error */
#define CDIO_DRIVE_CAP_UNKNOWN        0x80000 /**< Dunno. It can be on if we
					        have only partial information 
                                                or are not completely certain
                                              */

#define CDIO_DRIVE_CAP_MISC_CLOSE_TRAY     0x00001 /**< caddy systems can't 
                                                   close... */
#define CDIO_DRIVE_CAP_MISC_EJECT          0x00002 /**< but can eject.  */
#define CDIO_DRIVE_CAP_MISC_LOCK	   0x00004 /**< disable manual eject */
#define CDIO_DRIVE_CAP_MISC_SELECT_SPEED   0x00008 /**< programmable speed */
#define CDIO_DRIVE_CAP_MISC_SELECT_DISC    0x00010 /**< select disc from 
                                                      juke-box */
#define CDIO_DRIVE_CAP_MISC_MULTI_SESSION  0x00020 /**< read sessions>1 */
#define CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED  0x00080 /**< media changed */
#define CDIO_DRIVE_CAP_MISC_RESET          0x00100 /**< hard reset device */
#define CDIO_DRIVE_CAP_MISC_FILE           0x20000 /**< drive is really a file,
                                                      i.e a CD file image */

  /*! Reading masks.. */
#define CDIO_DRIVE_CAP_READ_AUDIO       0x00001 /**< drive can play CD audio */
#define CDIO_DRIVE_CAP_READ_CD_DA       0x00002 /**< drive can read CD-DA */
#define CDIO_DRIVE_CAP_READ_CD_G        0x00004 /**< drive can read CD+G  */
#define CDIO_DRIVE_CAP_READ_CD_R        0x00008 /**< drive can read CD-R  */
#define CDIO_DRIVE_CAP_READ_CD_RW       0x00010 /**< drive can read CD-RW */
#define CDIO_DRIVE_CAP_READ_DVD_R       0x00020 /**< drive can read DVD-R */
#define CDIO_DRIVE_CAP_READ_DVD_PR      0x00040 /**< drive can read DVD+R */
#define CDIO_DRIVE_CAP_READ_DVD_RAM     0x00080 /**< drive can read DVD-RAM */
#define CDIO_DRIVE_CAP_READ_DVD_ROM     0x00100 /**< drive can read DVD-ROM */
#define CDIO_DRIVE_CAP_READ_DVD_RW      0x00200 /**< drive can read DVD-RW  */
#define CDIO_DRIVE_CAP_READ_DVD_RPW     0x00400 /**< drive can read DVD+RW  */
#define CDIO_DRIVE_CAP_READ_C2_ERRS     0x00800 /**< has C2 error correction */
#define CDIO_DRIVE_CAP_READ_MODE2_FORM1 0x01000 /**< can read mode 2 form 1 */
#define CDIO_DRIVE_CAP_READ_MODE2_FORM2 0x02000 /**< can read mode 2 form 2 */
#define CDIO_DRIVE_CAP_READ_MCN         0x04000 /**< can read MCN      */
#define CDIO_DRIVE_CAP_READ_ISRC        0x08000 /**< can read ISRC     */

  /*! Writing masks.. */
#define CDIO_DRIVE_CAP_WRITE_CD_R       0x00001 /**< drive can write CD-R */
#define CDIO_DRIVE_CAP_WRITE_CD_RW      0x00002 /**< drive can write CD-R */
#define CDIO_DRIVE_CAP_WRITE_DVD_R      0x00004 /**< drive can write DVD-R */
#define CDIO_DRIVE_CAP_WRITE_DVD_PR     0x00008 /**< drive can write DVD+R */
#define CDIO_DRIVE_CAP_WRITE_DVD_RAM    0x00010 /**< drive can write DVD-RAM */
#define CDIO_DRIVE_CAP_WRITE_DVD_RW     0x00020 /**< drive can write DVD-RW */
#define CDIO_DRIVE_CAP_WRITE_DVD_RPW    0x00040 /**< drive can write DVD+RW */
#define CDIO_DRIVE_CAP_WRITE_MT_RAINIER 0x00080 /**< Mount Rainier           */
#define CDIO_DRIVE_CAP_WRITE_BURN_PROOF 0x00100 /**< burn proof */

/**< Masks derived from above... */
#define CDIO_DRIVE_CAP_WRITE_CD (                \
    CDIO_DRIVE_CAP_WRITE_CD_R                    \
    | CDIO_DRIVE_CAP_WRITE_CD_RW                 \
    ) 
/**< Has some sort of CD writer ability */

/**< Masks derived from above... */
#define CDIO_DRIVE_CAP_WRITE_DVD (               \
    | CDIO_DRIVE_CAP_WRITE_DVD_R                 \
    | CDIO_DRIVE_CAP_WRITE_DVD_PR                \
    | CDIO_DRIVE_CAP_WRITE_DVD_RAM               \
    | CDIO_DRIVE_CAP_WRITE_DVD_RW                \
    | CDIO_DRIVE_CAP_WRITE_DVD_RPW               \
    ) 
/**< Has some sort of DVD writer ability */

#define CDIO_DRIVE_CAP_WRITE \
   (CDIO_DRIVE_CAP_WRITE_CD | CDIO_DRIVE_CAP_WRITE_DVD)
/**< Has some sort of DVD or CD writing ability */

  /*! 
    track flags
    Q Sub-channel Control Field (4.2.3.3)
  */
  typedef enum {
    CDIO_TRACK_FLAG_NONE = 		 0x00,	/**< no flags set */
    CDIO_TRACK_FLAG_PRE_EMPHASIS =	 0x01,	/**< audio track recorded with
                                                   pre-emphasis */
    CDIO_TRACK_FLAG_COPY_PERMITTED =	 0x02,	/**< digital copy permitted */
    CDIO_TRACK_FLAG_DATA =		 0x04,	/**< data track */
    CDIO_TRACK_FLAG_FOUR_CHANNEL_AUDIO = 0x08,  /**< 4 audio channels */
  CDIO_TRACK_FLAG_SCMS =		 0x10	/**< SCMS (5.29.2.7) */
} cdio_track_flag;

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
