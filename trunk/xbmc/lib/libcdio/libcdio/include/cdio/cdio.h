/* -*- c -*-
    $Id: cdio.h,v 1.81 2005/11/07 07:44:00 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>

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

/** \file cdio.h 
 *
 *  \brief The top-level header for libcdio: the CD Input and Control
 *  library. Applications include this for anything regarding libcdio.
 */


#ifndef __CDIO_H__
#define __CDIO_H__

/** Application Interface or Protocol version number. If the public
 *  interface changes, we increase this number.
 */
#define CDIO_API_VERSION 5

#include <cdio/version.h>

#ifdef  HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cdio/types.h>
#include <cdio/sector.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For compatibility. */
#define CdIo CdIo_t
    
  /** This is an opaque structure for the CD object. */
  typedef struct _CdIo CdIo_t; 

  /** This is an opaque structure for the CD-Text object. */
  typedef struct cdtext cdtext_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Drive(r)/Device-related functions. Perhaps we should break out 
   Driver from device?
*/
#include <cdio/device.h>

/* Disc-related functions. */
#include <cdio/disc.h>

/* Sector (frame, or block)-related functions. Uses driver_return_code_t
   from <cdio/device.h> so it should come after that. 
*/
#include <cdio/read.h>

/* CD-Text-related functions. */
#include <cdio/cdtext.h>

/* Track-related functions. */
#include <cdio/track.h>

#endif /* __CDIO_H__ */
