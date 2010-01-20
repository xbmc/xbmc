/*
    $Id: image_common.h,v 1.10 2005/02/17 07:03:37 rocky Exp $

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

#ifndef __CDIO_IMAGE_COMMON_H__
#define __CDIO_IMAGE_COMMON_H__

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  internal_position_t pos; 
  
  char         *psz_cue_name;
  char         *psz_access_mode; /* Just the name of the driver.
				    We add this for regularity with other
				    real CD drivers which has an access mode.
				  */
  char         *psz_mcn;        /* Media Catalog Number (5.22.3) 
				   exactly 13 bytes */
  track_info_t  tocent[CDIO_CD_MAX_TRACKS+1]; /* entry info for each track 
					         add 1 for leadout. */
  discmode_t    disc_mode;

#ifdef NEED_NERO_STRUCT
  /* Nero Specific stuff. Note: for the image_free to work, this *must*
     be last. */
  bool          is_dao;          /* True if some of disk at once. False
				    if some sort of track at once. */
  uint32_t      mtyp;            /* Value of MTYP (media type?) tag */
  uint8_t       dtyp;            /* Value of DAOX media type tag */

  /* This is a hack because I don't really understnad NERO better. */
  bool            is_cues;

  CdioList_t    *mapping;        /* List of track information */
  uint32_t      size;
#endif
} _img_private_t;

#define free_if_notnull(p_obj) \
  if (NULL != p_obj) { free(p_obj); p_obj=NULL; };

/*!
  We don't need the image any more. Free all memory associated with
  it.
 */
void  _free_image (void *p_user_data);

int _eject_media_image(void *p_user_data);

/*!
  Return the value associated with the key "arg".
*/
const char * _get_arg_image (void *user_data, const char key[]);

/*! 
  Get disc type associated with cd_obj.
*/
discmode_t _get_discmode_image (void *p_user_data);

/*!
  Return the the kind of drive capabilities of device.

 */
void _get_drive_cap_image (const void *user_data,
			   cdio_drive_read_cap_t  *p_read_cap,
			   cdio_drive_write_cap_t *p_write_cap,
			   cdio_drive_misc_cap_t  *p_misc_cap);

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
track_t _get_first_track_num_image(void *p_user_data);

/*! 
  Find out if media has changed since the last call.
  @param p_user_data the CD object to be acted upon.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
  We always return DRIVER_OP_UNSUPPORTED.
 */
int get_media_changed_image(const void *p_user_data);

/*!
  Return the media catalog number (MCN) from the CD or NULL if there
  is none or we don't have the ability to get it.

  Note: string is malloc'd so caller has to free() the returned
  string when done with it.
  */
char * _get_mcn_image(const void *p_user_data);

/*!
  Return the number of tracks. 
*/
track_t _get_num_tracks_image(void *p_user_data);


/*!  
  Return the starting MSF (minutes/secs/frames) for the track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.

*/
bool _get_track_msf_image(void *p_user_data, track_t i_track, msf_t *msf);

/*! Return number of channels in track: 2 or 4; -2 if not
  implemented or -1 for error.
  Not meaningful if track is not an audio track.
*/
int get_track_channels_image(const void *p_user_data, track_t i_track);

/*! Return 1 if copy is permitted on the track, 0 if not, or -1 for error.
  Is this meaningful if not an audio track?
*/
track_flag_t get_track_copy_permit_image(void *p_user_data, track_t i_track);

/*! Return 1 if track has pre-emphasis, 0 if not, or -1 for error.
  Is this meaningful if not an audio track?
  
  pre-emphasis is a non linear frequency response.
*/
track_flag_t get_track_preemphasis_image(const void *p_user_data, 
					 track_t i_track);
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

  @param i_blocks number of blocks to read.

  */
driver_return_code_t 
read_data_sectors_image ( void *p_user_data, void *p_buf, 
			  lsn_t i_lsn,  uint16_t i_blocksize,
			  uint32_t i_blocks );

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" to set the source device in I/O operations 
  is the only valid key.

  0 is returned if no error was found, and nonzero if there as an error.
*/
int _set_arg_image (void *user_data, const char key[], const char value[]);

#endif /* __CDIO_IMAGE_COMMON_H__ */
