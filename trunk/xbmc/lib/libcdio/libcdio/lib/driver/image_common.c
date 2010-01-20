/*
    $Id: image_common.c,v 1.12 2005/02/17 07:03:37 rocky Exp $

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

/*! Common image routines. 
  
  Because _img_private_t may vary over image formats, the routines are
  included into the image drivers after _img_private_t is defined.  In
  order for the below routines to work, there is a large part of
  _img_private_t that is common among image drivers. For example, see
  image.h
*/

#include "image.h"
#include "image_common.h"
#include "_cdio_stdio.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*!
  Eject media -- there's nothing to do here except free resources.
  We always return DRIVER_OP_UNSUPPORTED.
 */
driver_return_code_t
_eject_media_image(void *p_user_data)
{
  _free_image (p_user_data);
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  We don't need the image any more. Free all memory associated with
  it.
 */
void 
_free_image (void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  track_t i_track;

  if (NULL == p_env) return;

  for (i_track=0; i_track < p_env->gen.i_tracks; i_track++) {
    track_info_t *p_tocent = &(p_env->tocent[i_track]);
    free_if_notnull(p_tocent->filename);
    free_if_notnull(p_tocent->isrc);
    cdtext_destroy(&(p_tocent->cdtext));
    if (p_tocent->data_source) cdio_stdio_destroy(p_tocent->data_source);
  }

  free_if_notnull(p_env->psz_mcn);
  free_if_notnull(p_env->psz_cue_name);
  free_if_notnull(p_env->psz_access_mode);
  cdtext_destroy(&(p_env->gen.cdtext));
  cdio_generic_stdio_free(p_env);
  free(p_env);
}

/*!
  Return the value associated with the key "arg".
*/
const char *
_get_arg_image (void *user_data, const char key[])
{
  _img_private_t *p_env = user_data;

  if (!strcmp (key, "source")) {
    return p_env->gen.source_name;
  } else if (!strcmp (key, "cue")) {
    return p_env->psz_cue_name;
  } else if (!strcmp(key, "access-mode")) {
    return "image";
  } 
  return NULL;
}

/*! 
  Get disc type associated with cd_obj.
*/
discmode_t
_get_discmode_image (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;
  return p_env->disc_mode;
}

/*!
  Return the the kind of drive capabilities of device.

 */
void
_get_drive_cap_image (const void *user_data,
		      cdio_drive_read_cap_t  *p_read_cap,
		      cdio_drive_write_cap_t *p_write_cap,
		      cdio_drive_misc_cap_t  *p_misc_cap)
{

  *p_read_cap  = CDIO_DRIVE_CAP_READ_CD_DA
    | CDIO_DRIVE_CAP_READ_CD_G
    | CDIO_DRIVE_CAP_READ_CD_R
    | CDIO_DRIVE_CAP_READ_CD_RW
    | CDIO_DRIVE_CAP_READ_MODE2_FORM1
    | CDIO_DRIVE_CAP_READ_MODE2_FORM2
    | CDIO_DRIVE_CAP_READ_MCN
    ;

  *p_write_cap = 0;

  /* In the future we may want to simulate
     LOCK, OPEN_TRAY, CLOSE_TRAY, SELECT_SPEED, etc.
  */
  *p_misc_cap  = CDIO_DRIVE_CAP_MISC_FILE;
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
track_t
_get_first_track_num_image(void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  
  return p_env->gen.i_first_track;
}

/*! 
  Find out if media has changed since the last call.
  @param p_user_data the CD object to be acted upon.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
  There is no such thing as changing a media image so we will
  always return 0 - no change.
 */
int
get_media_changed_image(const void *p_user_data)
{
  return 0;
}

/*!
  Return the media catalog number (MCN) from the CD or NULL if there
  is none or we don't have the ability to get it.

  Note: string is malloc'd so caller has to free() the returned
  string when done with it.
  */
char *
_get_mcn_image(const void *p_user_data)
{
  const _img_private_t *p_env = p_user_data;
  
  if (!p_env || !p_env->psz_mcn) return NULL;
  return strdup(p_env->psz_mcn);
}

/*!
  Return the number of tracks. 
*/
track_t
_get_num_tracks_image(void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;

  return p_env->gen.i_tracks;
}

/*!  
  Return the starting MSF (minutes/secs/frames) for the track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.

*/
bool
_get_track_msf_image(void *p_user_data, track_t i_track, msf_t *msf)
{
  const _img_private_t *p_env = p_user_data;

  if (NULL == msf) return false;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = p_env->gen.i_tracks+1;

  if (i_track <= p_env->gen.i_tracks+1 && i_track != 0) {
    *msf = p_env->tocent[i_track-p_env->gen.i_first_track].start_msf;
    return true;
  } else 
    return false;
}

/*! Return number of channels in track: 2 or 4; -2 if not
  implemented or -1 for error.
  Not meaningful if track is not an audio track.
*/
int 
get_track_channels_image(const void *p_user_data, track_t i_track)
{
  const _img_private_t *p_env = p_user_data;
  return ( p_env->tocent[i_track-p_env->gen.i_first_track].flags 
	  & FOUR_CHANNEL_AUDIO ) ? 4 : 2;
}

/*! Return 1 if copy is permitted on the track, 0 if not, or -1 for error.
  Is this meaningful if not an audio track?
*/
track_flag_t 
get_track_copy_permit_image(void *p_user_data, track_t i_track)
{
  const _img_private_t *p_env = p_user_data;
  return ( p_env->tocent[i_track-p_env->gen.i_first_track].flags 
	   & COPY_PERMITTED ) ? CDIO_TRACK_FLAG_TRUE : CDIO_TRACK_FLAG_FALSE;
}

/*! Return 1 if track has pre-emphasis, 0 if not, or -1 for error.
  Is this meaningful if not an audio track?
  
  pre-emphasis is a non linear frequency response.
*/
track_flag_t
get_track_preemphasis_image(const void *p_user_data, track_t i_track)
{
  const _img_private_t *p_env = p_user_data;
  return ( p_env->tocent[i_track-p_env->gen.i_first_track].flags 
	   & PRE_EMPHASIS ) ? CDIO_TRACK_FLAG_TRUE : CDIO_TRACK_FLAG_FALSE;
}

/*!
  Read a data sector
  
  @param p_cdio object to read from

  @param p_buf place to read data into.  The caller should make sure
  this location can store at least ISO_BLOCKSIZE, M2RAW_SECTOR_SIZE,
  or M2F2_SECTOR_SIZE depending on the kind of sector getting read. If
  you don't know whether you have a Mode 1/2, Form 1/ Form 2/Formless
  sector best to reserve space for the maximum, M2RAW_SECTOR_SIZE.

  @param i_lsn sector to read

  @param i_blocksize size of block. Should be either ISO_BLOCKSIZE
  M2RAW_SECTOR_SIZE, or M2F2_SECTOR_SIZE. See comment above under
  p_buf.
  */
driver_return_code_t 
read_data_sectors_image ( void *p_user_data, void *p_buf, 
			  lsn_t i_lsn,  uint16_t i_blocksize,
			  uint32_t i_blocks )
{
  const _img_private_t *p_env = p_user_data;

  if (!p_env || !p_env->gen.cdio) return DRIVER_OP_UNINIT;
  
  {
    CdIo_t *p_cdio                = p_env->gen.cdio;
    track_t i_track               = cdio_get_track(p_cdio, i_lsn);
    track_format_t e_track_format = cdio_get_track_format(p_cdio, i_track);

    switch(e_track_format) {
    case TRACK_FORMAT_PSX:
    case TRACK_FORMAT_AUDIO:
    case TRACK_FORMAT_ERROR:
      return DRIVER_OP_ERROR;
    case TRACK_FORMAT_DATA:
      return cdio_read_mode1_sectors (p_cdio, p_buf, i_lsn, false, i_blocks);
    case TRACK_FORMAT_CDI:
    case TRACK_FORMAT_XA:
      return cdio_read_mode2_sectors (p_cdio, p_buf, i_lsn, false, i_blocks);
    }
  }
  return DRIVER_OP_ERROR;
}


/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" to set the source device in I/O operations 
  is the only valid key.

*/
driver_return_code_t
_set_arg_image (void *p_user_data, const char key[], const char value[])
{
  _img_private_t *p_env = p_user_data;

  if (!strcmp (key, "source"))
    {
      free_if_notnull (p_env->gen.source_name);
      if (!value) return DRIVER_OP_ERROR;
      p_env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "cue"))
    {
      free_if_notnull (p_env->psz_cue_name);
      if (!value) return DRIVER_OP_ERROR;
      p_env->psz_cue_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      free_if_notnull (p_env->psz_access_mode);
      if (!value) return DRIVER_OP_ERROR;
      p_env->psz_access_mode = strdup (value);
    }
  else
    return DRIVER_OP_ERROR;

  return DRIVER_OP_SUCCESS;
}

