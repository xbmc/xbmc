/*
    $Id: win32.h,v 1.2 2005/01/27 11:08:55 rocky Exp $

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

#include "cdio_private.h"

#pragma pack()

typedef struct {
  lsn_t          start_lsn;
  UCHAR          Control : 4;
  UCHAR          Format;
  cdtext_t       cdtext;	         /* CD-TEXT */
} track_info_t;

typedef enum {
  _AM_NONE,
  _AM_IOCTL,
  _AM_ASPI,
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  access_mode_t access_mode;

  /* Some of the more OS specific things. */
    /* Entry info for each track, add 1 for leadout. */
  track_info_t  tocent[CDIO_CD_MAX_TRACKS+1];

  HANDLE h_device_handle; /* device descriptor */
  long  hASPI;
  short i_sid;
  short i_lun;
  long  (*lpSendCommand)( void* );

  bool b_ioctl_init;
  bool b_aspi_init;

} _img_private_t;

/*! 
  Get disc type associated with cd object.
*/
discmode_t get_discmode_win32ioctl (_img_private_t *p_env);

/*!
   Reads an audio device using the DeviceIoControl method into data
   starting from lsn.  Returns 0 if no error.
*/
int read_audio_sectors_win32ioctl (_img_private_t *obj, void *data, lsn_t lsn, 
				   unsigned int nblocks);
/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int read_mode2_sector_win32ioctl (const _img_private_t *env, void *data, 
				  lsn_t lsn, bool b_form2);

/*!
   Reads a single mode1 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int read_mode1_sector_win32ioctl (const _img_private_t *env, void *data, 
				  lsn_t lsn, bool b_form2);

const char *is_cdrom_win32ioctl (const char drive_letter);

/*!
  Run a SCSI MMC command. 
 
  env	        private CD structure 
  i_timeout_ms  time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  Return 0 if command completed successfully.
 */
int run_scsi_cmd_win32ioctl( void *p_user_data, 
			     unsigned int i_timeout,
			     unsigned int i_cdb, 
			     const scsi_mmc_cdb_t * p_cdb,
			     scsi_mmc_direction_t e_direction, 
			     unsigned int i_buf, /*in/out*/ void *p_buf );

/*!
  Initialize internal structures for CD device.
 */
bool init_win32ioctl (_img_private_t *p_env);

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool read_toc_win32ioctl (_img_private_t *p_env);

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
char *get_mcn_win32ioctl (const _img_private_t *p_env);

/*
  Read cdtext information for a CdIo object .
  
  return true on success, false on error or CD-TEXT information does
  not exist.
*/
bool init_cdtext_win32ioctl (_img_private_t *p_env);

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
void get_drive_cap_aspi (const _img_private_t *p_env,
			 cdio_drive_read_cap_t  *p_read_cap,
			 cdio_drive_write_cap_t *p_write_cap,
			 cdio_drive_misc_cap_t  *p_misc_cap);

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
void get_drive_cap_win32ioctl (const _img_private_t *p_env,
			       cdio_drive_read_cap_t  *p_read_cap,
			       cdio_drive_write_cap_t *p_write_cap,
			       cdio_drive_misc_cap_t  *p_misc_cap);

/*!  
  Get the format (XA, DATA, AUDIO) of a track. 
*/
track_format_t get_track_format_win32ioctl(const _img_private_t *p_env, 
					   track_t i_track); 

void set_cdtext_field_win32(void *user_data, track_t i_track, 
			    track_t i_first_track,
			    cdtext_field_t e_field, const char *psz_value);

