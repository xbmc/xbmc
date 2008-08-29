/* -*- C++ -*-
    $Id: track.hpp,v 1.1 2005/11/10 11:11:16 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

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

/** \file track.hpp
 *  \brief methods relating to getting Compact Discs. This file
 *  should not be #included directly.
 */

/*!  
  Return an opaque CdIo_t pointer for the given track object.
*/
CdIo_t *getCdIo()
{
  return p_cdio;
}

/*! 
  Get CD-Text information for a CdIo_t object.
  
  @return the CD-Text object or NULL if obj is NULL
  or CD-Text information does not exist.
*/
cdtext_t *getCdtext () 
{
  return cdio_get_cdtext (p_cdio, i_track);
}

/*! Return number of channels in track: 2 or 4; -2 if not
  implemented or -1 for error.
  Not meaningful if track is not an audio track.
*/
int getChannels()
{
  return cdio_get_track_channels(p_cdio, i_track);
}

/*! Return copy protection status on a track. Is this meaningful
  if not an audio track?
*/
track_flag_t getCopyPermit() 
{
  return cdio_get_track_copy_permit(p_cdio, i_track);
}

/*!  
  Get the format (audio, mode2, mode1) of track. 
*/
track_format_t getFormat()
{
  return cdio_get_track_format(p_cdio, i_track);
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
bool getGreen() 
{
  return cdio_get_track_green(p_cdio, i_track);
}

/*!  
  Return the ending LSN. CDIO_INVALID_LSN is returned on error.
*/
lsn_t getLastLsn() 
{
  return cdio_get_track_last_lsn(p_cdio, i_track);
}

/*!  
  Get the starting LBA. 

  @return the starting LBA or CDIO_INVALID_LBA on error.
*/
lba_t getLba() 
{
  return cdio_get_track_lba(p_cdio, i_track);
}

/*!  
  @return the starting LSN or CDIO_INVALID_LSN on error.
*/
lsn_t getLsn()
{
  return cdio_get_track_lsn(p_cdio, i_track);
}


/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  i_track in p_cdio. 

  @return true if things worked or false if there is no track entry.
*/
bool getMsf(/*out*/ msf_t &msf)
{
  return cdio_get_track_msf(p_cdio, i_track,/*out*/ &msf);
}

/*!  
  Return the track number of the track object.
*/
track_t getTrackNum()
{
  return i_track;
}

/*! Get linear preemphasis status on an audio track 
  This is not meaningful if not an audio track?
*/
track_flag_t getPreemphasis()
{
  return cdio_get_track_preemphasis(p_cdio, i_track);
}

/*!  
  Get the number of sectors between this track an the next.  This
  includes any pregap sectors before the start of the next track.
  
  @return the number of sectors or 0 if there is an error.
*/
unsigned int getSecCount() 
{
  return cdio_get_track_sec_count(p_cdio, i_track);
}


