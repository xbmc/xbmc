/* -*- c -*-
    $Id: disc.h,v 1.4 2005/01/24 00:06:31 rocky Exp $

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

/** \file disc.h 
 *  \brief  The top-level header for disc-related libcdio calls.
 */
#ifndef __CDIO_DISC_H__
#define __CDIO_DISC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  extern const char *discmode2str[];
  
  /*! 
    Get disc mode - the kind of CD (CD-DA, CD-ROM mode 1, CD-MIXED, etc.
    that we've got. The notion of "CD" is extended a little to include
    DVD's.
  */
  discmode_t cdio_get_discmode (CdIo_t *p_cdio);

  /*!  
    Get the lsn of the end of the CD

    @return the lsn. On error 0 or CDIO_INVALD_LSN.
  */
  lsn_t cdio_get_disc_last_lsn(const CdIo_t *p_cdio);
  

  /*!  
    Return the Joliet level recognized for p_cdio.
  */
  uint8_t cdio_get_joliet_level(const CdIo_t *p_cdio);

  /*!
    Get the media catalog number (MCN) from the CD.

    @return the media catalog number r NULL if there is none or we
    don't have the ability to get it.

    Note: string is malloc'd so caller has to free() the returned
    string when done with it.

  */
  char * cdio_get_mcn (const CdIo_t *p_cdio);

  /*!
    Get the number of tracks on the CD.

    @return the number of tracks, or CDIO_INVALID_TRACK if there is
    an error.
  */
  track_t cdio_get_num_tracks (const CdIo_t *p_cdio);
  
  /*! cdio_stat_size is deprecated. @see cdio_get_disc_last_lsn  */
#define cdio_stat_size cdio_get_disc_last_lsn
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_DISC_H__ */

