/* -*- C++ -*-
    $Id: disc.hpp,v 1.1 2005/11/10 11:11:16 rocky Exp $

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

/** \file disc.hpp
 *  \brief methods relating to getting Compact Disc information. This file
 *  should not be #included directly.
 */

/*! 
  Get disc mode - the kind of CD (CD-DA, CD-ROM mode 1, CD-MIXED, etc.
  that we've got. The notion of "CD" is extended a little to include
  DVD's.
*/
discmode_t getDiscmode () 
{
  return cdio_get_discmode(p_cdio);
}

/*!  
  Get the lsn of the end of the CD
  
  @return the lsn. On error 0 or CDIO_INVALD_LSN.
*/
lsn_t getDiscLastLsn() 
{
  return cdio_get_disc_last_lsn(p_cdio);
}

/*!
  Get the number of the first track. 
  
  @return a track object or NULL;
  on error.
*/
CdioTrack *getFirstTrack() 
{
  track_t i_track = cdio_get_first_track_num(p_cdio);
  return (CDIO_INVALID_TRACK != i_track) 
    ? new CdioTrack(p_cdio, i_track)
    : (CdioTrack *) NULL;
}

/*!
  Get the number of the first track. 
  
  @return the track number or CDIO_INVALID_TRACK 
  on error.
*/
track_t getFirstTrackNum() 
{
  return cdio_get_first_track_num(p_cdio);
}


/*!
  Get the number of the first track. 
  
  @return a track object or NULL;
  on error.
*/
CdioTrack *getLastTrack() 
{
  track_t i_track = cdio_get_last_track_num(p_cdio);
  return (CDIO_INVALID_TRACK != i_track) 
    ? new CdioTrack(p_cdio, i_track)
    : (CdioTrack *) NULL;
}

/*!
  Get the number of the first track. 
  
  @return the track number or CDIO_INVALID_TRACK 
  on error.
*/
track_t getLastTrackNum() 
{
  return cdio_get_last_track_num(p_cdio);
}

/*!  
  Return the Joliet level recognized for p_cdio.
*/
uint8_t getJolietLevel() 
{
  return cdio_get_joliet_level(p_cdio);
}

/*!
  Get the media catalog number (MCN) from the CD.
  
  @return the media catalog number r NULL if there is none or we
  don't have the ability to get it.
  
  Note: string is malloc'd so caller has to free() the returned
  string when done with it.
  
*/
char * getMcn () 
{
  return cdio_get_mcn (p_cdio);
}

/*!
  Get the number of tracks on the CD.
  
  @return the number of tracks, or CDIO_INVALID_TRACK if there is
  an error.
*/
track_t getNumTracks () 
{
  return cdio_get_num_tracks(p_cdio);
}

/*! Find the track which contans lsn.
  CDIO_INVALID_TRACK is returned if the lsn outside of the CD or
  if there was some error. 
  
  If the lsn is before the pregap of the first track 0 is returned.
  Otherwise we return the track that spans the lsn.
*/
CdioTrack *getTrackFromNum(track_t i_track) 
{
  return new CdioTrack(p_cdio, i_track);
}

/*! Find the track which contans lsn.
  CDIO_INVALID_TRACK is returned if the lsn outside of the CD or
  if there was some error. 
  
  If the lsn is before the pregap of the first track 0 is returned.
  Otherwise we return the track that spans the lsn.
*/
CdioTrack *getTrackFromLsn(lsn_t lsn) 
{
  track_t i_track = cdio_get_track(p_cdio, lsn);
  return (CDIO_INVALID_TRACK != i_track) 
    ? new CdioTrack(p_cdio, i_track)
    : (CdioTrack *) NULL;
}


/*! 
  Return true if discmode is some sort of CD.
*/
bool isDiscmodeCdrom (discmode_t discmode) {
  return cdio_is_discmode_cdrom(discmode);
}


/*! 
  Return true if discmode is some sort of DVD.
*/
bool isDiscmodeDvd (discmode_t discmode) 
{
  return cdio_is_discmode_dvd (discmode) ;
}


