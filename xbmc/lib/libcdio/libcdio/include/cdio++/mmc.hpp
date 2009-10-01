/*
    $Id: mmc.hpp,v 1.2 2006/01/18 21:31:37 rocky Exp $

    Copyright (C) 2005, 2006 Rocky Bernstein <rocky@panix.com>

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

/** \file mmc.hpp
 *  \brief methods relating to  MMC (Multimedia Commands). This file
 *  should not be #included directly.
 */

/*!
  Read Audio Subchannel information
  
  @param p_cdio the CD object to be acted upon.
  @param p_subchannel place for returned subchannel information

  A DriverOpException is raised on error.
*/
void
mmcAudioReadSubchannel (/*out*/ cdio_subchannel_t *p_subchannel) 
{
  driver_return_code_t drc = mmc_audio_read_subchannel (p_cdio, p_subchannel);
  possible_throw_device_exception(drc);
}

/*!
  Eject using MMC commands. If CD-ROM is "locked" we'll unlock it.
  Command is not "immediate" -- we'll wait for the command to complete.
  For a more general (and lower-level) routine, @see mmc_start_stop_media.

  A DriverOpException is raised on error.
*/
void mmcEjectMedia() 
{
  driver_return_code_t drc = mmc_eject_media( p_cdio );
  possible_throw_device_exception(drc);
}
  
/*!  
  Get the lsn of the end of the CD
  
  @return the lsn. On error return CDIO_INVALID_LSN.
*/
lsn_t mmcGetDiscLastLsn() 
{
  return mmc_get_disc_last_lsn( p_cdio );
}

/*! 
  Return the discmode as reported by the MMC Read (FULL) TOC
  command.
  
  Information was obtained from Section 5.1.13 (Read TOC/PMA/ATIP)
  pages 56-62 from the MMC draft specification, revision 10a
  at http://www.t10.org/ftp/t10/drafts/mmc/mmc-r10a.pdf See
  especially tables 72, 73 and 75.
*/
discmode_t mmcGetDiscmode() 
{
  return mmc_get_discmode( p_cdio );
}

/*!
  Get drive capabilities for a device.
  @return the drive capabilities.
*/
void mmcGetDriveCap ( /*out*/ cdio_drive_read_cap_t  *p_read_cap,
                      /*out*/ cdio_drive_write_cap_t *p_write_cap,
                      /*out*/ cdio_drive_misc_cap_t  *p_misc_cap) 
{
  mmc_get_drive_cap ( p_cdio, p_read_cap, p_write_cap, p_misc_cap);
}

/*!
  Get the MMC level supported by the device.
*/
cdio_mmc_level_t mmcGetDriveMmcCap() 
{
  return mmc_get_drive_mmc_cap(p_cdio);
}

/*! 
  Get the DVD type associated with cd object.
  
  @return the DVD discmode.
*/
discmode_t mmcGetDvdStructPhysical (cdio_dvd_struct_t *s) 
{
  return mmc_get_dvd_struct_physical (p_cdio, s);
}

/*! 
  Get the CD-ROM hardware info via an MMC INQUIRY command.
  
  @return true if we were able to get hardware info, false if we had
  an error.
*/
bool mmcGetHwinfo ( /* out*/ cdio_hwinfo_t *p_hw_info ) 
{
  return mmc_get_hwinfo ( p_cdio, p_hw_info );
}

/*! 
  Find out if media has changed since the last call.
  @param p_cdio the CD object to be acted upon.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
*/
int mmcGetMediaChanged() 
{
  return mmc_get_media_changed(p_cdio);
}

/*!
  Get the media catalog number (MCN) from the CD via MMC.
  
  @return the media catalog number r NULL if there is none or we
  don't have the ability to get it.
  
  Note: string is malloc'd so caller has to free() the returned
  string when done with it.
  
*/
char * mmcGetMcn () 
{
  return mmc_get_mcn ( p_cdio );
}

/** Get the output port volumes and port selections used on AUDIO PLAY
    commands via a MMC MODE SENSE command using the CD Audio Control
    Page.

    A DriverOpException is raised on error.
*/
void mmcAudioGetVolume (mmc_audio_volume_t *p_volume) 
{
  driver_return_code_t drc = mmc_audio_get_volume (p_cdio, p_volume);
  possible_throw_device_exception(drc);
}

/*!
  Report if CD-ROM has a praticular kind of interface (ATAPI, SCSCI, ...)
  Is it possible for an interface to have serveral? If not this 
  routine could probably return the single mmc_feature_interface_t.
  @return true if we have the interface and false if not.
*/
bool_3way_t mmcHaveInterface( cdio_mmc_feature_interface_t e_interface ) 
{
  return mmc_have_interface( p_cdio, e_interface );
}

/*! Run a MODE_SENSE command (6- or 10-byte version) 
  and put the results in p_buf 
  @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
int mmcModeSense( /*out*/ void *p_buf, int i_size, int page) 
{
  return mmc_mode_sense( p_cdio, /*out*/ p_buf, i_size, page);
}

/*! Run a MODE_SENSE command (10-byte version) 
  and put the results in p_buf 
  @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
int mmcModeSense10( /*out*/ void *p_buf, int i_size, int page) 
{
  return mmc_mode_sense_10( p_cdio, /*out*/ p_buf, i_size, page);
}

/*! Run a MODE_SENSE command (6-byte version) 
  and put the results in p_buf 
  @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
int mmcModeSense6( /*out*/ void *p_buf, int i_size, int page) 
{
  return mmc_mode_sense_6( p_cdio, /*out*/ p_buf, i_size, page);
}

/*! Issue a MMC READ_CD command.
  
@param p_cdio  object to read from 

@param p_buf   Place to store data. The caller should ensure that 
               p_buf can hold at least i_blocksize * i_blocks  bytes.

@param i_lsn   sector to read 
  
@param expected_sector_type restricts reading to a specific CD
  sector type.  Only 3 bits with values 1-5 are used:
    0 all sector types
    1 CD-DA sectors only 
    2 Mode 1 sectors only
    3 Mode 2 formless sectors only. Note in contrast to all other
      values an MMC CD-ROM is not required to support this mode.
    4 Mode 2 Form 1 sectors only
    5 Mode 2 Form 2 sectors only

@param b_digital_audio_play Control error concealment when the
  data being read is CD-DA.  If the data being read is not CD-DA,
  this parameter is ignored.  If the data being read is CD-DA and
  DAP is false zero, then the user data returned should not be
  modified by flaw obscuring mechanisms such as audio data mute and
  interpolate.  If the data being read is CD-DA and DAP is true,
  then the user data returned should be modified by flaw obscuring
  mechanisms such as audio data mute and interpolate.  
  
  b_sync_header return the sync header (which will probably have
  the same value as CDIO_SECTOR_SYNC_HEADER of size
  CDIO_CD_SYNC_SIZE).
  
  @param header_codes Header Codes refer to the sector header and
  the sub-header that is present in mode 2 formed sectors: 
  
   0 No header information is returned.  
   1 The 4-byte sector header of data sectors is be returned, 
   2 The 8-byte sector sub-header of mode 2 formed sectors is
     returned.  
   3 Both sector header and sub-header (12 bytes) is returned.  
   The Header preceeds the rest of the bytes (e.g. user-data bytes) 
   that might get returned.
   
   @param b_user_data  Return user data if true. 
   
   For CD-DA, the User Data is CDIO_CD_FRAMESIZE_RAW bytes.

   For Mode 1, The User Data is ISO_BLOCKSIZE bytes beginning at
   offset CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE.
   
   For Mode 2 formless, The User Data is M2RAW_SECTOR_SIZE bytes
   beginning at offset CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE.
   
   For data Mode 2, form 1, User Data is ISO_BLOCKSIZE bytes beginning at
   offset CDIO_CD_XA_SYNC_HEADER.
   
   For data Mode 2, form 2, User Data is 2 324 bytes beginning at
   offset CDIO_CD_XA_SYNC_HEADER.
   
   @param b_sync 

   @param b_edc_ecc true if we return EDC/ECC error detection/correction bits.
   
   The presence and size of EDC redundancy or ECC parity is defined
   according to sector type: 
   
   CD-DA sectors have neither EDC redundancy nor ECC parity.  
   
   Data Mode 1 sectors have 288 bytes of EDC redundancy, Pad, and
   ECC parity beginning at offset 2064.
   
   Data Mode 2 formless sectors have neither EDC redundancy nor ECC
   parity
   
   Data Mode 2 form 1 sectors have 280 bytes of EDC redundancy and
   ECC parity beginning at offset 2072
   
   Data Mode 2 form 2 sectors optionally have 4 bytes of EDC
   redundancy beginning at offset 2348.
   
   
   @param c2_error_information If true associate a bit with each
   sector for C2 error The resulting bit field is ordered exactly as
   the main channel bytes.  Each 8-bit boundary defines a byte of
   flag bits.
   
   @param subchannel_selection subchannel-selection bits
   
     0  No Sub-channel data shall be returned.  (0 bytes)
     1  RAW P-W Sub-channel data shall be returned.  (96 byte)
     2  Formatted Q sub-channel data shall be transferred (16 bytes)
     3  Reserved     
     4  Corrected and de-interleaved R-W sub-channel (96 bytes)
     5-7  Reserved

   @param i_blocksize size of the a block expected to be returned
     
   @param i_blocks number of blocks expected to be returned.

   A DriverOpException is raised on error.     
  */
void 
mmcReadCd ( void *p_buf, lsn_t i_lsn, int expected_sector_type, 
            bool b_digital_audio_play, bool b_sync, uint8_t header_codes, 
            bool b_user_data, bool b_edc_ecc, uint8_t c2_error_information, 
            uint8_t subchannel_selection, uint16_t i_blocksize, 
            uint32_t i_blocks ) 
{
  driver_return_code_t drc = 
    mmc_read_cd ( p_cdio, p_buf, i_lsn, expected_sector_type, 
                  b_digital_audio_play, b_sync, header_codes, 
                  b_user_data, b_edc_ecc, c2_error_information, 
                  subchannel_selection, i_blocksize, i_blocks );
  possible_throw_device_exception(drc);
}

/*! Read just the user data part of some sort of data sector (via 
    mmc_read_cd). 
    
    @param p_cdio object to read from

    @param p_buf place to read data into.  The caller should make sure
                 this location can store at least CDIO_CD_FRAMESIZE,
                 M2RAW_SECTOR_SIZE, or M2F2_SECTOR_SIZE depending on
                 the kind of sector getting read. If you don't know
                 whether you have a Mode 1/2, Form 1/ Form 2/Formless
                 sector best to reserve space for the maximum,
                 M2RAW_SECTOR_SIZE.

    @param i_lsn sector to read
    @param i_blocksize size of each block
    @param i_blocks number of blocks to read

  */
void mmcReadDataSectors ( void *p_buf, lsn_t i_lsn, uint16_t i_blocksize,
                          uint32_t i_blocks=1) 
{
  driver_return_code_t drc = mmc_read_data_sectors ( p_cdio, p_buf, i_lsn, 
                                                     i_blocksize, i_blocks );
  possible_throw_device_exception(drc);
}


/*! Read MMC read mode2 sectors

    A DriverOpException is raised on error.
 */
void mmcReadSectors ( void *p_buf, lsn_t i_lsn,  int read_sector_type, 
                      uint32_t i_blocks=1) 
{
  driver_return_code_t drc = mmc_read_sectors ( p_cdio, p_buf, i_lsn, 
                                                read_sector_type, i_blocks);
  possible_throw_device_exception(drc);
}

/*!
    Run an MMC command. 
    
    @param p_cdio	 CD structure set by cdio_open().
    @param i_timeout_ms  time in milliseconds we will wait for the command
                         to complete. 
    @param p_cdb	 CDB bytes. All values that are needed should be set 
                         on input. We'll figure out what the right CDB length 
                         should be.
    @param e_direction   direction the transfer is to go.
    @param i_buf	 Size of buffer
    @param p_buf	 Buffer for data, both sending and receiving.

    @return 0 if command completed successfully.
  */
int mmcRunCmd( unsigned int i_timeout_ms, const mmc_cdb_t *p_cdb,
               cdio_mmc_direction_t e_direction, unsigned int i_buf, 
               /*in/out*/ void *p_buf )
{
  return mmc_run_cmd( p_cdio, i_timeout_ms, p_cdb, e_direction, i_buf, p_buf );
}

/*!
  Set the block size for subsequent read requests, via MMC.

  @param i_blocksize size to set for subsequent requests

  A DriverOpException is raised on error.
*/
void mmcSetBlocksize ( uint16_t i_blocksize) 
{
  driver_return_code_t drc = mmc_set_blocksize ( p_cdio, i_blocksize);
  possible_throw_device_exception(drc);
}


/*!
  Set the drive speed via MMC. 

  @param i_speed speed to set drive to.

  A DriverOpException is raised on error.
*/
void mmcSetSpeed( int i_speed )
{
  driver_return_code_t drc = mmc_set_speed( p_cdio, i_speed );
  possible_throw_device_exception(drc);
}

/*!
  Load or Unload media using a MMC START STOP command. 
  
  @param p_cdio  the CD object to be acted upon.
  @param b_eject eject if true and close tray if false
  @param b_immediate wait or don't wait for operation to complete
  @param power_condition Set CD-ROM to idle/standby/sleep. If nonzero
  eject/load is ignored, so set to 0 if you want to eject or load.
  
  @see mmc_eject_media or mmc_close_tray

  A DriverOpException is raised on error.
*/
void mmcStartStopMedia(bool b_eject, bool b_immediate, 
                       uint8_t power_condition) 
{
  driver_return_code_t drc = 
    mmc_start_stop_media(p_cdio, b_eject, b_immediate, power_condition);
 possible_throw_device_exception(drc);
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
