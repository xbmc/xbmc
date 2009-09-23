/*
  $Id: paranoia.h,v 1.14 2007/09/28 12:09:38 rocky Exp $

  Copyright (C) 2004, 2005, 2006, 2007 Rocky Bernstein <rocky@gnu.org>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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

/** \file paranoia.h 
 *
 *  \brief The top-level header for libcdda_paranoia: a device- and OS-
 *  independent library for reading CD-DA with error tolerance and
 *  repair. Applications include this for paranoia access.
 */

#ifndef _CDIO_PARANOIA_H_
#define _CDIO_PARANOIA_H_

#include <cdio/cdda.h>

/*! Paranoia likes to work with 16-bit numbers rather than
    (possibly byte-swapped) bytes. So there are this many 
    16-bit numbers block (frame, or sector) read.
*/
#define CD_FRAMEWORDS (CDIO_CD_FRAMESIZE_RAW/2)

/**
  Flags used in paranoia_modeset. 

  The enumeration type one probably wouldn't really use in a program.
  It is here instead of defines to give symbolic names that can be
  helpful in debuggers where wants just to say refer to
  PARANOIA_MODE_DISABLE and get the correct value.
*/

typedef enum  {
  PARANOIA_MODE_DISABLE   =  0x00, /**< No fixups */
  PARANOIA_MODE_VERIFY    =  0x01, /**< Verify data integrety in overlap area*/
  PARANOIA_MODE_FRAGMENT  =  0x02, /**< unsupported */
  PARANOIA_MODE_OVERLAP   =  0x04, /**< Perform overlapped reads */
  PARANOIA_MODE_SCRATCH   =  0x08, /**< unsupported */
  PARANOIA_MODE_REPAIR    =  0x10, /**< unsupported */
  PARANOIA_MODE_NEVERSKIP =  0x20, /**< Do not skip failed reads (retry 
				      maxretries) */
  PARANOIA_MODE_FULL      =  0xff, /**< Maximum paranoia - all of the above 
				        (except disable) */
} paranoia_mode_t;
  

/**
   Flags set in a callback.

  The enumeration type one probably wouldn't really use in a program.
  It is here instead of defines to give symbolic names that can be
  helpful in debuggers where wants just to say refer to
  PARANOIA_CB_READ and get the correct value.
*/
typedef enum  {
  PARANOIA_CB_READ,           /**< Read off adjust ??? */
  PARANOIA_CB_VERIFY,         /**< Verifying jitter */
  PARANOIA_CB_FIXUP_EDGE,     /**< Fixed edge jitter */
  PARANOIA_CB_FIXUP_ATOM,     /**< Fixed atom jitter */
  PARANOIA_CB_SCRATCH,        /**< Unsupported */
  PARANOIA_CB_REPAIR,         /**< Unsupported */
  PARANOIA_CB_SKIP,           /**< Skip exhausted retry */
  PARANOIA_CB_DRIFT,          /**< Skip exhausted retry */
  PARANOIA_CB_BACKOFF,        /**< Unsupported */
  PARANOIA_CB_OVERLAP,        /**< Dynamic overlap adjust */
  PARANOIA_CB_FIXUP_DROPPED,  /**< Fixed dropped bytes */
  PARANOIA_CB_FIXUP_DUPED,    /**< Fixed duplicate bytes */
  PARANOIA_CB_READERR         /**< Hard read error */
} paranoia_cb_mode_t;

  extern const char *paranoia_cb_mode2str[];
  
#ifdef __cplusplus
extern "C" {
#endif

  /*! 
    Get and initialize a new cdrom_paranoia object from cdrom_drive.
    Run this before calling any of the other paranoia routines below.

    @return new cdrom_paranoia object Call paranoia_free() when you are
    done with it
   */
  extern cdrom_paranoia_t *cdio_paranoia_init(cdrom_drive_t *d);
  
  /*!
    Free any resources associated with p.

    @param p       paranoia object to for which resources are to be freed.

    @see paranoia_init.
   */
  extern void cdio_paranoia_free(cdrom_paranoia_t *p);

  /*! 
    Set the kind of repair you want to on for reading. 
    The modes are listed above

    @param p             paranoia type
    @param mode_flags    paranoia mode flags built from values in 
                         paranoia_mode_t, e.g. 
		         PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP
   */
  extern void cdio_paranoia_modeset(cdrom_paranoia_t *p, int mode_flags);

  /*!
    reposition reading offset. 

    @param p       paranoia type
    @param seek    byte offset to seek to
    @param whence  like corresponding parameter in libc's lseek, e.g. 
                   SEEK_SET or SEEK_END.
  */
  extern lsn_t cdio_paranoia_seek(cdrom_paranoia_t *p, off_t seek, int whence);

  /*!
    Reads the next sector of audio data and returns a pointer to a full
    sector of verified samples. 
    
    @param p paranoia object.

    @param callback callback routine which gets called with the status
    on each read.

    @return the audio data read, CDIO_CD_FRAMESIZE_RAW (2352)
    bytes. This data is not to be freed by the caller. It will persist
    only until the next call to paranoia_read() for this p.
  */
  extern int16_t *cdio_paranoia_read(cdrom_paranoia_t *p,
				     void(*callback)(long int, 
						     paranoia_cb_mode_t));

  /*! The same as cdio_paranoia_read but the number of retries is set.
    @param p paranoia object.

    @param callback callback routine which gets called with the status
    on each read.

    @param max_retries number of times to try re-reading a block before
    failing.

    @return the block of CDIO_FRAMEIZE_RAW bytes (or
    CDIO_FRAMESIZE_RAW / 2 16-bit integers). Unless byte-swapping has
    been turned off the 16-bit integers Endian independent order.

    @see cdio_paranoia_read.
    
  */
  extern int16_t *cdio_paranoia_read_limited(cdrom_paranoia_t *p,
					     void(*callback)(long int, 
							   paranoia_cb_mode_t),
					     int max_retries);


/*! a temporary hack */
  extern void cdio_paranoia_overlapset(cdrom_paranoia_t *p,long overlap);

  extern void cdio_paranoia_set_range(cdrom_paranoia_t *p, long int start, 
				      long int end);

#ifndef DO_NOT_WANT_PARANOIA_COMPATIBILITY
/** For compatibility with good ol' paranoia */
#define paranoia_init         cdio_paranoia_init
#define paranoia_free         cdio_paranoia_free
#define paranoia_modeset      cdio_paranoia_modeset
#define paranoia_seek         cdio_paranoia_seek
#define paranoia_read         cdio_paranoia_read
#define paranoia_read_limited cdio_paranoia_read_limited
#define paranoia_overlapset   cdio_paranoia_overlapset
#define paranoia_set_range    cdio_paranoia_set_range
#endif /*DO_NOT_WANT_PARANOIA_COMPATIBILITY*/

#ifdef __cplusplus
}
#endif

/** The below variables are trickery to force the above enum symbol
    values to be recorded in debug symbol tables. They are used to
    allow one to refer to the enumeration value names in the typedefs
    above in a debugger and debugger expressions
*/

extern paranoia_mode_t    debug_paranoia_mode;
extern paranoia_cb_mode_t debug_paranoia_cb_mode;

#endif /*_CDIO_PARANOIA_H_*/
