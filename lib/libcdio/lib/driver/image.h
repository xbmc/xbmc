/*
    $Id: image.h,v 1.6 2005/01/22 23:57:10 rocky Exp $

    Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>

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

/*! 
  Header for image drivers. In contrast to image_common.h which contains
  routines, this header like most C headers does not depend on anything
  defined before it is included.
*/

#ifndef __CDIO_IMAGE_H__
#define __CDIO_IMAGE_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/types.h>
#include <cdio/cdtext.h>
#include "cdio_private.h"
#include <cdio/sector.h>

/*! 
  The universal format for information about a track for CD image readers
  It may be that some fields can be derived from other fields.
  Over time this structure may get cleaned up. Possibly this can be
  expanded/reused for real CD formats.
*/

typedef struct {
  track_t        track_num;     /**< Probably is index+1 */
  msf_t          start_msf;
  lba_t          start_lba;
  int            start_index;
  lba_t          pregap;	/**< pre-gap with zero audio data */
  int            sec_count;     /**< Number of sectors in this track. Does not
				     include pregap */
  int            num_indices;
  flag_t         flags;         /**< "[NO] COPY", "4CH", "[NO] PREMPAHSIS" */
  char          *isrc;		/**< IRSC Code (5.22.4) exactly 12 bytes */
  char           *filename;
  CdioDataSource_t *data_source;
  off_t          offset;        /**< byte offset into data_start of track
				     beginning. In cdrdao for example, one
				     filename may cover many tracks and
				     each track would then have a different
				     offset.
				*/
  track_format_t track_format;
  bool           track_green;
  cdtext_t       cdtext;	/**< CD-TEXT */

  trackmode_t    mode;
  uint16_t       datasize;      /**< How much is in the portion we return 
				     back? */
  long int       datastart;     /**<  Offset from begining of fraem 
				      that data starts */
  uint16_t       endsize;       /**< How much stuff at the end to skip over. 
				     This stuff may have error correction 
				     (EDC, or ECC).*/
  uint16_t       blocksize;     /**< total block size = start + size + end */
} track_info_t;


#endif /* __CDIO_IMAGE_H__ */
