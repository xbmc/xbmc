/*
    $Id: cdio_private.h,v 1.11 2005/01/27 03:10:06 rocky Exp $

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

/* Internal routines for CD I/O drivers. */


#ifndef __CDIO_PRIVATE_H__
#define __CDIO_PRIVATE_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include "scsi_mmc_private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* Opaque type */
  typedef struct _CdioDataSource CdioDataSource_t;

#ifdef __cplusplus
}

#endif /* __cplusplus */

#include "generic.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


  typedef struct {
    
    /*!
      Eject media in CD drive. If successful, as a side effect we 
      also free obj. Return 0 if success and 1 for failure.
    */
    int (*eject_media) (void *p_env);
    
    /*!
      Release and free resources associated with cd. 
    */
    void (*free) (void *p_env);
    
    /*!
      Return the value associated with the key "arg".
    */
    const char * (*get_arg) (void *p_env, const char key[]);
    
    /*!
      Get the block size for subsequest read requests, via a SCSI MMC 
      MODE_SENSE 6 command.
    */
    int (*get_blocksize) (void *p_env);
    
    /*! 
      Get cdtext information for a CdIo object.
    
      @param obj the CD object that may contain CD-TEXT information.
      @return the CD-TEXT object or NULL if obj is NULL
      or CD-TEXT information does not exist.
    
      If i_track is 0 or CDIO_CDROM_LEADOUT_TRACK the track returned
      is the information assocated with the CD. 
    */
    cdtext_t * (*get_cdtext) (void *p_env, track_t i_track);
    
    /*!
      Return an array of device names. if CdIo is NULL (we haven't
      initialized a specific device driver), then find a suitable device 
      driver.
      
      NULL is returned if we couldn't return a list of devices.
    */
    char ** (*get_devices) (void);
    
    /*!
      Return a string containing the default CD device if none is specified.
    */
    char * (*get_default_device)(void);
    
    /*!
      Return the size of the CD in logical block address (LBA) units.
      @return the lsn. On error 0 or CDIO_INVALD_LSN.
    */
    lsn_t (*get_disc_last_lsn) (void *p_env);

    /*! 
      Get disc mode associated with cd_obj.
    */
    discmode_t (*get_discmode) (void *p_env);

    /*!
      Return the what kind of device we've got.
      
      See cd_types.h for a list of bitmasks for the drive type;
    */
    void (*get_drive_cap) (const void *p_env,
			   cdio_drive_read_cap_t  *p_read_cap,
			   cdio_drive_write_cap_t *p_write_cap,
			   cdio_drive_misc_cap_t  *p_misc_cap);
    /*!
      Return the number of of the first track. 
      CDIO_INVALID_TRACK is returned on error.
    */
    track_t (*get_first_track_num) (void *p_env);
    
    /*! 
      Get the CD-ROM hardware info via a SCSI MMC INQUIRY command.
      False is returned if we had an error getting the information.
    */
    bool (*get_hwinfo) ( const CdIo_t *p_cdio, 
			 /* out*/ cdio_hwinfo_t *p_hw_info );

    /*!  
      Return the media catalog number MCN from the CD or NULL if
      there is none or we don't have the ability to get it.
    */
    char * (*get_mcn) (const void *p_env);

    /*! 
      Return the number of tracks in the current medium.
      CDIO_INVALID_TRACK is returned on error.
    */
    track_t (*get_num_tracks) (void *p_env);
    
    /*! Return number of channels in track: 2 or 4; -2 if not
      implemented or -1 for error.
      Not meaningful if track is not an audio track.
    */
    int (*get_track_channels) (const void *p_env, track_t i_track);
  
    /*! Return 0 if track is copy protected, 1 if not, or -1 for error
      or -2 if not implimented (yet). Is this meaningful if not an
      audio track?
    */
    track_flag_t (*get_track_copy_permit) (void *p_env, track_t i_track);
  
    /*!  
      Return the starting LBA for track number
      i_track in p_env.  Tracks numbers start at 1.
      The "leadout" track is specified either by
      using track_num LEADOUT_TRACK or the total tracks+1.
      CDIO_INVALID_LBA is returned on error.
    */
    lba_t (*get_track_lba) (void *p_env, track_t i_track);
    
    /*!  
      Get format of track. 
    */
    track_format_t (*get_track_format) (void *p_env, track_t i_track);
    
    /*!
      Set the drive speed. -1 is returned if we had an error.
      -2 is returned if this is not implemented for the current driver.
    */
    int (*get_speed) (void *p_env);
    
    /*!
      Return true if we have XA data (green, mode2 form1) or
      XA data (green, mode2 form2). That is track begins:
      sync - header - subheader
      12     4      -  8
      
      FIXME: there's gotta be a better design for this and get_track_format?
    */
    bool (*get_track_green) (void *p_env, track_t i_track);
    
    /*!  
      Return the starting MSF (minutes/secs/frames) for track number
      i_track in p_env.  Tracks numbers start at 1.
      The "leadout" track is specified either by
      using i_track LEADOUT_TRACK or the total tracks+1.
      False is returned on error.
    */
    bool (*get_track_msf) (void *p_env, track_t i_track, msf_t *p_msf);
    
    /*! Return 1 if track has pre-emphasis, 0 if not, or -1 for error
      or -2 if not implimented (yet). Is this meaningful if not an
      audio track?
    */
    track_flag_t (*get_track_preemphasis) (const void  *p_env, 
					   track_t i_track);
  
    /*!
      lseek - reposition read/write file offset
      Returns (off_t) -1 on error. 
      Similar to libc's lseek()
    */
    off_t (*lseek) (void *p_env, off_t offset, int whence);
    
    /*!
      Reads into buf the next size bytes.
      Returns -1 on error. 
      Similar to libc's read()
    */
    ssize_t (*read) (void *p_env, void *p_buf, size_t size);
    
    /*!
      Reads a single mode2 sector from cd device into buf starting
      from lsn. Returns 0 if no error. 
    */
    int (*read_audio_sectors) (void *p_env, void *p_buf, lsn_t lsn,
			       unsigned int i_blocks);
    
    /*!
      Reads a single mode2 sector from cd device into buf starting
      from lsn. Returns 0 if no error. 
    */
    int (*read_mode2_sector) (void *p_env, void *p_buf, lsn_t lsn, 
			      bool b_mode2_form2);
    
    /*!
      Reads i_blocks of mode2 sectors from cd device into data starting
      from lsn.
      Returns 0 if no error. 
    */
    int (*read_mode2_sectors) (void *p_env, void *p_buf, lsn_t lsn, 
			       bool b_mode2_form2, unsigned int i_blocks);
    
    /*!
      Reads a single mode1 sector from cd device into buf starting
      from lsn. Returns 0 if no error. 
    */
    int (*read_mode1_sector) (void *p_env, void *p_buf, lsn_t lsn, 
			      bool mode1_form2);
    
    /*!
      Reads i_blocks of mode1 sectors from cd device into data starting
      from lsn.
      Returns 0 if no error. 
    */
    int (*read_mode1_sectors) (void *p_env, void *p_buf, lsn_t lsn, 
			       bool mode1_form2, unsigned int i_blocks);
    
    bool (*read_toc) ( void *p_env ) ;

    /*!
      Run a SCSI MMC command. 
      
      cdio	        CD structure set by cdio_open().
      i_timeout_ms      time in milliseconds we will wait for the command
                        to complete. 
      cdb_len           number of bytes in cdb (6, 10, or 12).
      cdb	        CDB bytes. All values that are needed should be set on 
                        input. 
      b_return_data	TRUE if the command expects data to be returned in 
                        the buffer
      len	        Size of buffer
      buf	        Buffer for data, both sending and receiving
      
      Returns 0 if command completed successfully.
    */
    scsi_mmc_run_cmd_fn_t run_scsi_mmc_cmd;

    /*!
      Set the arg "key" with "value" in the source device.
    */
    int (*set_arg) (void *p_env, const char key[], const char value[]);
    
    /*!
      Set the blocksize for subsequent reads. 
      
      @return 0 if everything went okay, -1 if we had an error. is -2
      returned if this is not implemented for the current driver.
    */
    int (*set_blocksize) ( void *p_env, int i_blocksize );

    /*!
      Set the drive speed. 
      
      @return 0 if everything went okay, -1 if we had an error. is -2
      returned if this is not implemented for the current driver.
    */
    int (*set_speed) ( void *p_env, int i_speed );

  } cdio_funcs_t;


  /*! Implementation of CdIo type */
  struct _CdIo {
    driver_id_t driver_id; /**< Particular driver opened. */
    cdio_funcs_t op;       /**< driver-specific routines handling
			        implementation*/
    void *env;             /**< environment. Passed to routine above. */
  };

  /* This is used in drivers that must keep their own internal 
     position pointer for doing seeks. Stream-based drivers (like bincue,
     nrg, toc, network) would use this. 
   */
  typedef struct 
  {
    off_t   buff_offset;      /* buffer offset in disk-image seeks. */
    track_t index;            /* Current track index in tocent. */
    lba_t   lba;              /* Current LBA */
  } internal_position_t;
  
  CdIo_t * cdio_new (generic_img_private_t *p_env, cdio_funcs_t *p_funcs);

  /* The below structure describes a specific CD Input driver  */
  typedef struct 
  {
    driver_id_t  id;
    unsigned int flags;
    const char  *name;
    const char  *describe;
    bool (*have_driver) (void); 
    CdIo *(*driver_open) (const char *psz_source_name); 
    CdIo *(*driver_open_am) (const char *psz_source_name, 
			     const char *psz_access_mode); 
    char *(*get_default_device) (void); 
    bool (*is_device) (const char *psz_source_name);
    char **(*get_devices) (void);
  } CdIo_driver_t;

  /* The below array gives of the drivers that are currently available for 
     on a particular host. */
  extern CdIo_driver_t CdIo_driver[CDIO_MAX_DRIVER];

  /* The last valid entry of Cdio_driver. -1 means uninitialzed. -2 
     means some sort of error.
   */
  extern int CdIo_last_driver; 

  /* The below array gives all drivers that can possibly appear.
     on a particular host. */
  extern CdIo_driver_t CdIo_all_drivers[CDIO_MAX_DRIVER+1];

  /*! 
    Add/allocate a drive to the end of drives. 
    Use cdio_free_device_list() to free this device_list.
  */
  void cdio_add_device_list(char **device_list[], const char *drive, 
			    unsigned int *i_drives);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_PRIVATE_H__ */
