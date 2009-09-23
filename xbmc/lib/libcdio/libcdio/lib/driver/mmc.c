/*  Common Multimedia Command (MMC) routines.

    $Id: mmc.c,v 1.37 2008/03/04 10:27:54 rocky Exp $

    Copyright (C) 2004, 2005, 2006, 2007, 2008 Rocky Bernstein <rocky@gnu.org>

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/logging.h>
#include <cdio/mmc.h>
#include <cdio/util.h>
#include "cdio_private.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

/** The below variables are trickery to force enum symbol values to be
    recorded in debug symbol tables. They are used to allow one to refer
    to the enumeration value names in the typedefs above in a debugger
    and debugger expressions
*/
cdio_mmc_feature_t           debug_cdio_mmc_feature;
cdio_mmc_feature_interface_t debug_cdio_mmc_feature_interface;
cdio_mmc_feature_profile_t   debug_cdio_mmc_feature_profile;
cdio_mmc_get_conf_t          debug_cdio_mmc_get_conf;
cdio_mmc_gpcmd_t             debug_cdio_mmc_gpcmd;
cdio_mmc_read_sub_state_t    debug_cdio_mmc_read_sub_state;
cdio_mmc_read_cd_type_t      debug_cdio_mmc_read_cd_type;
cdio_mmc_readtoc_t           debug_cdio_mmc_readtoc;
cdio_mmc_mode_page_t         debug_cdio_mmc_mode_page;

/*************************************************************************
  MMC CdIo Operations which a driver may use. 
  These are not accessible directly.

  Most of these routines just pick out the cdio pointer and call the
  corresponding publically-accessible routine.
*************************************************************************/

/*! The maximum value in milliseconds that we will wait on an MMC
      command.  */
uint32_t mmc_timeout_ms = MMC_TIMEOUT_DEFAULT;

/*! The maximum value in milliseconds that we will wait on an MMC read
  command.  */
uint32_t mmc_read_timeout_ms = MMC_READ_TIMEOUT_DEFAULT;

/*!
  Read Audio Subchannel information
  
  @param p_user_data the CD object to be acted upon.
  
*/
driver_return_code_t
audio_read_subchannel_mmc ( void *p_user_data, cdio_subchannel_t *p_subchannel)
{
  generic_img_private_t *p_env = p_user_data;
  if (!p_env) return DRIVER_OP_UNINIT;
  return mmc_audio_read_subchannel(p_env->cdio, p_subchannel);
}

/*!
  Return a string containing the name of the audio state as returned from
  the Q_SUBCHANNEL.
 */
const char *mmc_audio_state2str( uint8_t i_audio_state )
{
  switch(i_audio_state) {
  case CDIO_MMC_READ_SUB_ST_INVALID:
    return "invalid";
  case CDIO_MMC_READ_SUB_ST_PLAY:
    return "playing";
  case CDIO_MMC_READ_SUB_ST_PAUSED:
    return "paused";
  case CDIO_MMC_READ_SUB_ST_COMPLETED:
    return "completed";
  case CDIO_MMC_READ_SUB_ST_ERROR:
    return "error";
  case CDIO_MMC_READ_SUB_ST_NO_STATUS:
    return "no status";
  default:                     
    return "unknown";
  }
}

/*!
  Get the block size for subsequest read requests, via MMC.
  @return the blocksize if > 0; error if <= 0
 */
int
get_blocksize_mmc (void *p_user_data)
{
  generic_img_private_t *p_env = p_user_data;
  if (!p_env) return DRIVER_OP_UNINIT;
  return mmc_get_blocksize(p_env->cdio);
}

/*!  
  Get the lsn of the end of the CD (via MMC).
  
  @return the lsn. On error return CDIO_INVALID_LSN.
*/
lsn_t
get_disc_last_lsn_mmc (void *p_user_data)
{
  generic_img_private_t *p_env = p_user_data;
  if (!p_env) return CDIO_INVALID_LSN;
  return mmc_get_disc_last_lsn(p_env->cdio);
}

void
get_drive_cap_mmc (const void *p_user_data,
		   /*out*/ cdio_drive_read_cap_t  *p_read_cap,
		   /*out*/ cdio_drive_write_cap_t *p_write_cap,
		   /*out*/ cdio_drive_misc_cap_t  *p_misc_cap)
{
  const generic_img_private_t *p_env = p_user_data;
  mmc_get_drive_cap( p_env->cdio,
                     p_read_cap, p_write_cap, p_misc_cap );
}

/*! Find out if media has changed since the last call.  @param
  p_user_data the environment of the CD object to be acted upon.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
   */
int
get_media_changed_mmc (const void *p_user_data)
{
  const generic_img_private_t *p_env = p_user_data;
  return mmc_get_media_changed( p_env->cdio );
}

char *
get_mcn_mmc (const void *p_user_data)
{
  const generic_img_private_t *p_env = p_user_data;
  return mmc_get_mcn( p_env->cdio );
}

driver_return_code_t
get_tray_status (const void *p_user_data)
{
  const generic_img_private_t *p_env = p_user_data;
  return mmc_get_tray_status( p_env->cdio );
}

/*! Read sectors using SCSI-MMC GPCMD_READ_CD.
   Can read only up to 25 blocks.
*/
driver_return_code_t 
read_data_sectors_mmc ( void *p_user_data, void *p_buf, 
                        lsn_t i_lsn,  uint16_t i_blocksize,
                        uint32_t i_blocks )
{
  const generic_img_private_t *p_env = p_user_data;
  return mmc_read_data_sectors( p_env->cdio, p_buf, i_lsn, i_blocksize,
                                i_blocks );
}

/* Set read blocksize (via MMC) */
driver_return_code_t
set_blocksize_mmc (void *p_user_data, uint16_t i_blocksize)
{
  generic_img_private_t *p_env = p_user_data;
  if (!p_env) return DRIVER_OP_UNINIT;
  return mmc_set_blocksize(p_env->cdio, i_blocksize);
}

/* Set the drive speed Set the drive speed in K bytes per second. (via
   MMC). */
driver_return_code_t
set_speed_mmc (void *p_user_data, int i_speed)
{
  generic_img_private_t *p_env = p_user_data;
  if (!p_env) return DRIVER_OP_UNINIT;
  return mmc_set_speed( p_env->cdio, i_speed );
}

/* Set the drive speed in CD-ROM speed units (via MMC). */
driver_return_code_t
set_drive_speed_mmc (void *p_user_data, int i_Kbs_speed)
{
  generic_img_private_t *p_env = p_user_data;
  if (!p_env) return DRIVER_OP_UNINIT;
  return mmc_set_drive_speed( p_env->cdio, i_Kbs_speed );
}

/** Get the output port volumes and port selections used on AUDIO PLAY
    commands via a MMC MODE SENSE command using the CD Audio Control
    Page.
 */
driver_return_code_t
mmc_audio_get_volume( CdIo_t *p_cdio, /*out*/ mmc_audio_volume_t *p_volume )
{
  uint8_t buf[16];
  int i_rc = mmc_mode_sense(p_cdio, buf, sizeof(buf), CDIO_MMC_AUDIO_CTL_PAGE);
  
  if ( DRIVER_OP_SUCCESS == i_rc ) {
    p_volume->port[0].selection = 0xF & buf[8];
    p_volume->port[0].volume    = buf[9];
    p_volume->port[1].selection = 0xF & buf[10];
    p_volume->port[1].volume    = buf[11];
    p_volume->port[2].selection = 0xF & buf[12];
    p_volume->port[2].volume    = buf[13];
    p_volume->port[3].selection = 0xF & buf[14];
    p_volume->port[3].volume    = buf[15];
    return DRIVER_OP_SUCCESS;
  }
  return i_rc;
}

/*!
  On input a MODE_SENSE command was issued and we have the results
  in p. We interpret this and return a bit mask set according to the 
  capabilities.
 */
void
mmc_get_drive_cap_buf(const uint8_t *p,
                      /*out*/ cdio_drive_read_cap_t  *p_read_cap,
                      /*out*/ cdio_drive_write_cap_t *p_write_cap,
                      /*out*/ cdio_drive_misc_cap_t  *p_misc_cap)
{
  /* Reader */
  if (p[2] & 0x01) *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_R;
  if (p[2] & 0x02) *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_RW;
  if (p[2] & 0x08) *p_read_cap  |= CDIO_DRIVE_CAP_READ_DVD_ROM;
  if (p[4] & 0x01) *p_read_cap  |= CDIO_DRIVE_CAP_READ_AUDIO;
  if (p[4] & 0x10) *p_read_cap  |= CDIO_DRIVE_CAP_READ_MODE2_FORM1;
  if (p[4] & 0x20) *p_read_cap  |= CDIO_DRIVE_CAP_READ_MODE2_FORM2;
  if (p[5] & 0x01) *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_DA;
  if (p[5] & 0x10) *p_read_cap  |= CDIO_DRIVE_CAP_READ_C2_ERRS;
  if (p[5] & 0x20) *p_read_cap  |= CDIO_DRIVE_CAP_READ_ISRC;
  if (p[5] & 0x40) *p_read_cap  |= CDIO_DRIVE_CAP_READ_MCN;
  
  /* Writer */
  if (p[3] & 0x01) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_R;
  if (p[3] & 0x02) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_RW;
  if (p[3] & 0x10) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_R;
  if (p[3] & 0x20) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_RAM;
  if (p[4] & 0x80) *p_misc_cap  |= CDIO_DRIVE_CAP_WRITE_BURN_PROOF;

  /* Misc */
  if (p[4] & 0x40) *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_MULTI_SESSION;
  if (p[6] & 0x01) *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_LOCK;
  if (p[6] & 0x08) *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_EJECT;
  if (p[6] >> 5 != 0) 
    *p_misc_cap |= CDIO_DRIVE_CAP_MISC_CLOSE_TRAY;
}

/*! 
  Get the DVD type associated with cd object.
*/
discmode_t
mmc_get_dvd_struct_physical_private ( void *p_env, 
                                      mmc_run_cmd_fn_t run_mmc_cmd, 
                                      cdio_dvd_struct_t *s)
{
  mmc_cdb_t cdb = {{0, }};
  unsigned char buf[4 + 4 * 20], *base;
  int i_status;
  uint8_t layer_num = s->physical.layer_num;
  
  cdio_dvd_layer_t *layer;
  
  if (!p_env) return DRIVER_OP_UNINIT;
  if (!run_mmc_cmd) return DRIVER_OP_UNSUPPORTED;

  if (layer_num >= CDIO_DVD_MAX_LAYERS)
    return -EINVAL;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_DVD_STRUCTURE);
  cdb.field[6] = layer_num;
  cdb.field[7] = CDIO_DVD_STRUCT_PHYSICAL;
  cdb.field[9] = sizeof(buf) & 0xff;
  
  i_status = run_mmc_cmd(p_env, mmc_timeout_ms, 
			      mmc_get_cmd_len(cdb.field[0]), 
			      &cdb, SCSI_MMC_DATA_READ, 
			      sizeof(buf), &buf);
  if (0 != i_status)
    return CDIO_DISC_MODE_ERROR;
  
  base = &buf[4];
  layer = &s->physical.layer[layer_num];
  
  /*
   * place the data... really ugly, but at least we won't have to
   * worry about endianess in userspace.
   */
  memset(layer, 0, sizeof(*layer));
  layer->book_version = base[0] & 0xf;
  layer->book_type = base[0] >> 4;
  layer->min_rate = base[1] & 0xf;
  layer->disc_size = base[1] >> 4;
  layer->layer_type = base[2] & 0xf;
  layer->track_path = (base[2] >> 4) & 1;
  layer->nlayers = (base[2] >> 5) & 3;
  layer->track_density = base[3] & 0xf;
  layer->linear_density = base[3] >> 4;
  layer->start_sector = base[5] << 16 | base[6] << 8 | base[7];
  layer->end_sector = base[9] << 16 | base[10] << 8 | base[11];
  layer->end_sector_l0 = base[13] << 16 | base[14] << 8 | base[15];
  layer->bca = base[16] >> 7;

  return DRIVER_OP_SUCCESS;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
char *
mmc_get_mcn_private ( void *p_env,
                      const mmc_run_cmd_fn_t run_mmc_cmd
                      )
{
  mmc_cdb_t cdb = {{0, }};
  char buf[28] = { 0, };
  int i_status;

  if ( ! p_env || ! run_mmc_cmd )
    return NULL;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_SUBCHANNEL);
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(buf));

  cdb.field[1] = 0x0;  
  cdb.field[2] = 0x40; 
  cdb.field[3] = CDIO_SUBCHANNEL_MEDIA_CATALOG;

  i_status = run_mmc_cmd(p_env, mmc_timeout_ms, 
			      mmc_get_cmd_len(cdb.field[0]), 
			      &cdb, SCSI_MMC_DATA_READ, 
			      sizeof(buf), buf);
  if(i_status == 0) {
    return strdup(&buf[9]);
  }
  return NULL;
}

/* Run a MODE SENSE command (either the 6- or 10-byte version
   @return DRIVER_OP_SUCCESS if we ran the command ok.
 */
int 
mmc_mode_sense( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                int page)
{
  /* We used to make a choice as to which routine we'd use based
     cdio_have_atapi(). But since that calls this in its determination,
     we had an infinite recursion. So we can't use cdio_have_atapi()
     (until we put in better capability checks.)
   */
  if ( DRIVER_OP_SUCCESS == mmc_mode_sense_6(p_cdio, p_buf, i_size, page) )
    return DRIVER_OP_SUCCESS;
  return mmc_mode_sense_10(p_cdio, p_buf, i_size, page);
}

/*! Run a MODE_SENSE command (6-byte version) 
  and put the results in p_buf 
  @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
int 
mmc_mode_sense_6( CdIo_t *p_cdio, void *p_buf, int i_size, int page)
{
  mmc_cdb_t cdb = {{0, }};

  if ( ! p_cdio ) return DRIVER_OP_UNINIT;
  if ( ! p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  memset (p_buf, 0, i_size);

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_MODE_SENSE_6);

  cdb.field[2] = 0x3F & page;
  cdb.field[4] = i_size;
  
  return p_cdio->op.run_mmc_cmd (p_cdio->env, 
                                 mmc_timeout_ms,
                                 mmc_get_cmd_len(cdb.field[0]), &cdb, 
                                 SCSI_MMC_DATA_READ, i_size, p_buf);
}


/*! Run a MODE_SENSE command (10-byte version) 
  and put the results in p_buf 
  @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
int 
mmc_mode_sense_10( CdIo_t *p_cdio, void *p_buf, int i_size, int page)
{
  mmc_cdb_t cdb = {{0, }};

  if ( ! p_cdio ) return DRIVER_OP_UNINIT;
  if ( ! p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  memset (p_buf, 0, i_size);

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_MODE_SENSE_10);
  CDIO_MMC_SET_READ_LENGTH16(cdb.field, i_size);

  cdb.field[2] = 0x3F & page;
  
  return p_cdio->op.run_mmc_cmd (p_cdio->env, 
                                 mmc_timeout_ms,
                                 mmc_get_cmd_len(cdb.field[0]), &cdb, 
                                 SCSI_MMC_DATA_READ, i_size, p_buf);
}


/*
  Read cdtext information for a CdIo_t object .
  
  return true on success, false on error or CD-Text information does
  not exist.
*/
bool
mmc_init_cdtext_private ( void *p_user_data, 
                          const mmc_run_cmd_fn_t run_mmc_cmd,
                          set_cdtext_field_fn_t set_cdtext_field_fn 
                          )
{

  generic_img_private_t *p_env = p_user_data;
  mmc_cdb_t  cdb = {{0, }};
  unsigned char   wdata[5000] = { 0, };
  int             i_status, i_errno;

  if ( ! p_env || ! run_mmc_cmd || p_env->b_cdtext_error )
    return false;

  /* Operation code */
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC);

  /* Setup to read header, to get length of data */
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, 4);

  cdb.field[1] = CDIO_CDROM_MSF;
  /* Format */
  cdb.field[2] = CDIO_MMC_READTOC_FMT_CDTEXT;

  errno = 0;

  /* We may need to give CD-Text a little more time to complete. */
  /* First off, just try and read the size */
  i_status = run_mmc_cmd (p_env, mmc_read_timeout_ms,
                          mmc_get_cmd_len(cdb.field[0]), 
                          &cdb, SCSI_MMC_DATA_READ, 
                          4, &wdata);

  if (i_status != 0) {
    cdio_info ("CD-Text read failed for header: %s\n", strerror(errno));  
	i_errno = errno;
    p_env->b_cdtext_error = true;
    return false;
  } else {
    /* Now read the CD-Text data */
    int	i_cdtext = CDIO_MMC_GET_LEN16(wdata);

    if (i_cdtext > sizeof(wdata)) i_cdtext = sizeof(wdata);
    
    CDIO_MMC_SET_READ_LENGTH16(cdb.field, i_cdtext);
    i_status = run_mmc_cmd (p_env, mmc_read_timeout_ms,
                            mmc_get_cmd_len(cdb.field[0]), 
                            &cdb, SCSI_MMC_DATA_READ, 
                            i_cdtext, &wdata);
    if (i_status != 0) {
      cdio_info ("CD-Text read for text failed: %s\n", strerror(errno));  
      i_errno = errno;
      p_env->b_cdtext_error = true;
      return false;
    }
    p_env->b_cdtext_init = true;
    return cdtext_data_init(p_env, p_env->i_first_track, wdata, i_cdtext-2,
			    set_cdtext_field_fn);
  }
}

driver_return_code_t
mmc_set_blocksize_private ( void *p_env, 
                            const mmc_run_cmd_fn_t run_mmc_cmd, 
                            uint16_t i_blocksize)
{
  mmc_cdb_t cdb = {{0, }};

  struct
  {
    uint8_t reserved1;
    uint8_t medium;
    uint8_t reserved2;
    uint8_t block_desc_length;
    uint8_t density;
    uint8_t number_of_blocks_hi;
    uint8_t number_of_blocks_med;
    uint8_t number_of_blocks_lo;
    uint8_t reserved3;
    uint8_t block_length_hi;
    uint8_t block_length_med;
    uint8_t block_length_lo;
  } mh;

  if ( ! p_env ) return DRIVER_OP_UNINIT;
  if ( ! run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  memset (&mh, 0, sizeof (mh));
  mh.block_desc_length = 0x08;
  mh.block_length_hi   = (i_blocksize >> 16) & 0xff;
  mh.block_length_med  = (i_blocksize >>  8) & 0xff;
  mh.block_length_lo   = (i_blocksize >>  0) & 0xff;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_MODE_SELECT_6);

  cdb.field[1] = 1 << 4;
  cdb.field[4] = 12;
  
  return run_mmc_cmd (p_env, mmc_timeout_ms,
			      mmc_get_cmd_len(cdb.field[0]), &cdb, 
			      SCSI_MMC_DATA_WRITE, sizeof(mh), &mh);
}

/***********************************************************
  User-accessible Operations.
************************************************************/
/*!  
  Return the number of length in bytes of the Command Descriptor
  buffer (CDB) for a given MMC command. The length will be 
  either 6, 10, or 12. 
*/
uint8_t
mmc_get_cmd_len(uint8_t scsi_cmd) 
{
  static const uint8_t scsi_cdblen[8] = {6, 10, 10, 12, 12, 12, 10, 10};
  return scsi_cdblen[((scsi_cmd >> 5) & 7)];
}

/*!
   Return the size of the CD in logical block address (LBA) units.
   @return the lsn. On error 0 or CDIO_INVALD_LSN.
 */
lsn_t
mmc_get_disc_last_lsn ( const CdIo_t *p_cdio )
{
  mmc_cdb_t cdb = {{0, }};
  uint8_t buf[12] = { 0, };

  lsn_t retval = 0;
  int i_status;

  /* Operation code */
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC);

  cdb.field[1] = 0; /* lba; msf: 0x2 */

  /* Format */
  cdb.field[2] = CDIO_MMC_READTOC_FMT_TOC;

  CDIO_MMC_SET_START_TRACK(cdb.field, CDIO_CDROM_LEADOUT_TRACK);

  CDIO_MMC_SET_READ_LENGTH16(cdb.field, sizeof(buf));
  
  i_status = mmc_run_cmd(p_cdio, mmc_timeout_ms, &cdb, SCSI_MMC_DATA_READ, 
                         sizeof(buf), buf);

  if (i_status) return CDIO_INVALID_LSN;

  {
    int i;
    for (i = 8; i < 12; i++) {
      retval <<= 8;
      retval += buf[i];
    }
  }

  return retval;
}

/*!
  Read Audio Subchannel information
  
  @param p_cdio the CD object to be acted upon.
  
*/
driver_return_code_t
mmc_audio_read_subchannel (CdIo_t *p_cdio,  cdio_subchannel_t *p_subchannel)
{
  mmc_cdb_t cdb;
  driver_return_code_t i_rc;
  cdio_mmc_subchannel_t mmc_subchannel;

  if (!p_cdio) return DRIVER_OP_UNINIT;
  
  memset(&mmc_subchannel, 0, sizeof(mmc_subchannel));
  mmc_subchannel.format = CDIO_CDROM_MSF;
  memset(&cdb, 0, sizeof(mmc_cdb_t));

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_SUBCHANNEL);
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(cdio_mmc_subchannel_t));

  cdb.field[1] = CDIO_CDROM_MSF;
  cdb.field[2] = 0x40; /* subq */
  cdb.field[3] = CDIO_SUBCHANNEL_CURRENT_POSITION;
  cdb.field[6] = 0;    /* track number (only in isrc mode, ignored) */

  i_rc = mmc_run_cmd(p_cdio, mmc_timeout_ms, &cdb, SCSI_MMC_DATA_READ, 
                     sizeof(cdio_mmc_subchannel_t), &mmc_subchannel);
  if (DRIVER_OP_SUCCESS == i_rc) {
    p_subchannel->format       = mmc_subchannel.format;
    p_subchannel->audio_status = mmc_subchannel.audio_status;
    p_subchannel->address      = mmc_subchannel.address;
    p_subchannel->control      = mmc_subchannel.control;
    p_subchannel->track        = mmc_subchannel.track;
    p_subchannel->index        = mmc_subchannel.index;
    p_subchannel->abs_addr.m   = cdio_to_bcd8(mmc_subchannel.abs_addr[1]);
    p_subchannel->abs_addr.s   = cdio_to_bcd8(mmc_subchannel.abs_addr[2]);
    p_subchannel->abs_addr.f   = cdio_to_bcd8(mmc_subchannel.abs_addr[3]);
    p_subchannel->rel_addr.m   = cdio_to_bcd8(mmc_subchannel.rel_addr[1]);
    p_subchannel->rel_addr.s   = cdio_to_bcd8(mmc_subchannel.rel_addr[2]);
    p_subchannel->rel_addr.f   = cdio_to_bcd8(mmc_subchannel.rel_addr[3]);
  }
  return i_rc;
}

/*! 
  Return the discmode as reported by the SCSI-MMC Read (FULL) TOC
  command.

  Information was obtained from Section 5.1.13 (Read TOC/PMA/ATIP)
  pages 56-62 from the MMC draft specification, revision 10a
  at http://www.t10.org/ftp/t10/drafts/mmc/mmc-r10a.pdf See
  especially tables 72, 73 and 75.
 */
discmode_t
mmc_get_discmode( const CdIo_t *p_cdio )

{
  uint8_t buf[14] = { 0, };
  mmc_cdb_t cdb;

  memset(&cdb, 0, sizeof(mmc_cdb_t));

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC);
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(buf));

  cdb.field[1] = CDIO_CDROM_MSF; /* The MMC-5 spec may require this. */
  cdb.field[2] = CDIO_MMC_READTOC_FMT_FULTOC;

  mmc_run_cmd(p_cdio, 2000, &cdb, SCSI_MMC_DATA_READ, sizeof(buf), buf);
  if (buf[7] == 0xA0) {
    if (buf[13] == 0x00) {
      if (buf[5] & 0x04) 
	return CDIO_DISC_MODE_CD_DATA;
      else 
	return CDIO_DISC_MODE_CD_DA;
    }
    else if (buf[13] == 0x10)
      return CDIO_DISC_MODE_CD_I;
    else if (buf[13] == 0x20) 
    return CDIO_DISC_MODE_CD_XA;
  }
  return CDIO_DISC_MODE_NO_INFO;
}

void
mmc_get_drive_cap (CdIo_t *p_cdio,
                   /*out*/ cdio_drive_read_cap_t  *p_read_cap,
                   /*out*/ cdio_drive_write_cap_t *p_write_cap,
                   /*out*/ cdio_drive_misc_cap_t  *p_misc_cap)
{
  /* Largest buffer size we use. */
#define BUF_MAX 2048
  uint8_t buf[BUF_MAX] = { 0, };

  int i_status;
  uint16_t i_data = BUF_MAX;
  int page = CDIO_MMC_ALL_PAGES;

  if ( ! p_cdio )  return;
 retry:

  /* In the first run we run MODE SENSE 10 we are trying to get the
     length of the data features. */
  i_status = mmc_mode_sense_10(p_cdio, buf, 8, CDIO_MMC_ALL_PAGES);

  if (DRIVER_OP_SUCCESS == i_status) {
    uint16_t i_data_try = (uint16_t) CDIO_MMC_GET_LEN16(buf);
    if (i_data_try < BUF_MAX) i_data = i_data_try;
  }

  /* Now try getting all features with length set above, possibly
     truncated or the default length if we couldn't get the proper
     length. */
  i_status = mmc_mode_sense_10(p_cdio, buf, i_data, CDIO_MMC_ALL_PAGES);
  if (0 != i_status && CDIO_MMC_CAPABILITIES_PAGE != page) {
    page =  CDIO_MMC_CAPABILITIES_PAGE; 
    goto retry;
  }

  if (DRIVER_OP_SUCCESS == i_status) {
    uint8_t *p;
    uint8_t *p_max = buf + 256;
    
    *p_read_cap  = 0;
    *p_write_cap = 0;
    *p_misc_cap  = 0;

    /* set to first sense mask, and then walk through the masks */
    p = buf + 8;
    while( (p < &(buf[2+i_data])) && (p < p_max) )       {
      uint8_t which_page;
      
      which_page = p[0] & 0x3F;
      switch( which_page )
	{
	case CDIO_MMC_AUDIO_CTL_PAGE:
	case CDIO_MMC_R_W_ERROR_PAGE:
	case CDIO_MMC_CDR_PARMS_PAGE:
	  /* Don't handle these yet. */
	  break;
	case CDIO_MMC_CAPABILITIES_PAGE:
	  mmc_get_drive_cap_buf(p, p_read_cap, p_write_cap, p_misc_cap);
	  break;
	default: ;
	}
      p += (p[1] + 2);
    }
  } else {
    cdio_info("%s: %s\n", "error in MODE_SELECT", strerror(errno));
    *p_read_cap  = CDIO_DRIVE_CAP_ERROR;
    *p_write_cap = CDIO_DRIVE_CAP_ERROR;
    *p_misc_cap  = CDIO_DRIVE_CAP_ERROR;
  }
  return;
}

/*!
  Get the MMC level supported by the device.
*/
cdio_mmc_level_t
mmc_get_drive_mmc_cap(CdIo_t *p_cdio) 
{
  uint8_t buf[256] = { 0, };
  uint8_t len;
  int rc = mmc_mode_sense(p_cdio, buf, sizeof(buf), 
			  CDIO_MMC_CAPABILITIES_PAGE);

  if (DRIVER_OP_SUCCESS != rc) {
    return CDIO_MMC_LEVEL_NONE;
  }
  
  len = buf[1];
  if (16 > len) {
    return CDIO_MMC_LEVEL_WEIRD;
  } else if (28 <= len) {
    return CDIO_MMC_LEVEL_3;
  } else if (24 <= len) {
    return CDIO_MMC_LEVEL_2;
    printf("MMC 2");
  } else if (20 <= len) {
    return CDIO_MMC_LEVEL_1;
  } else {
    return CDIO_MMC_LEVEL_WEIRD;
  }
}

/*! 
  Get the DVD type associated with cd object.
*/
discmode_t
mmc_get_dvd_struct_physical ( const CdIo_t *p_cdio, cdio_dvd_struct_t *s)
{
  if ( ! p_cdio )  return -2;
  return 
    mmc_get_dvd_struct_physical_private (p_cdio->env, 
                                         p_cdio->op.run_mmc_cmd, 
                                         s);
}

/*! 
  Get the CD-ROM hardware info via a MMC INQUIRY command.
  False is returned if we had an error getting the information.
*/
bool 
mmc_get_hwinfo ( const CdIo_t *p_cdio, 
		      /*out*/ cdio_hwinfo_t *hw_info )
{
  int i_status;                  /* Result of MMC command */
  char buf[36] = { 0, };         /* Place to hold returned data */
  mmc_cdb_t cdb = {{0, }};  /* Command Descriptor Block */
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_INQUIRY);
  cdb.field[4] = sizeof(buf);

  if (! p_cdio || ! hw_info ) return false;
  
  i_status = mmc_run_cmd(p_cdio, mmc_timeout_ms, 
			      &cdb, SCSI_MMC_DATA_READ, 
			      sizeof(buf), &buf);
  if (i_status == 0) {
      
      memcpy(hw_info->psz_vendor, 
	     buf + 8, 
	     sizeof(hw_info->psz_vendor)-1);
      hw_info->psz_vendor[sizeof(hw_info->psz_vendor)-1] = '\0';
      memcpy(hw_info->psz_model,  
	     buf + 8 + CDIO_MMC_HW_VENDOR_LEN, 
	     sizeof(hw_info->psz_model)-1);
      hw_info->psz_model[sizeof(hw_info->psz_model)-1] = '\0';
      memcpy(hw_info->psz_revision, 
	     buf + 8 + CDIO_MMC_HW_VENDOR_LEN + CDIO_MMC_HW_MODEL_LEN,
	     sizeof(hw_info->psz_revision)-1);
      hw_info->psz_revision[sizeof(hw_info->psz_revision)-1] = '\0';
      return true;
    }
  return false;
}

/*! 
  Return results of media status
  @param p_cdio the CD object to be acted upon.
  @return DRIVER_OP_SUCCESS (0) if we got the status.
  return codes are the same as driver_return_code_t
   */
int mmc_get_event_status(const CdIo_t *p_cdio, uint8_t out_buf[2])
{
  mmc_cdb_t cdb = {{0, }};
  uint8_t buf[8] = { 0, };
  int i_status;

  if ( ! p_cdio ) return DRIVER_OP_UNINIT;
  if ( ! p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_GET_EVENT_STATUS);

  /* Setup to read header, to get length of data */
  CDIO_MMC_SET_READ_LENGTH16(cdb.field, sizeof(buf));

  cdb.field[1] = 1;      /* We poll for info */
  cdb.field[4] = 1 << 4; /* We want Media events */

  i_status = p_cdio->op.run_mmc_cmd(p_cdio->env, mmc_timeout_ms, 
                                    mmc_get_cmd_len(cdb.field[0]), 
                                    &cdb, SCSI_MMC_DATA_READ, 
                                    sizeof(buf), buf);
  if(i_status == 0) {
    out_buf[0] = buf[4];
    out_buf[1] = buf[5];
    return DRIVER_OP_SUCCESS;
  }
  return DRIVER_OP_ERROR;
}

/*! 
  Find out if media has changed since the last call.
  @param p_cdio the CD object to be acted upon.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
   */
int mmc_get_media_changed(const CdIo_t *p_cdio)
{
  uint8_t status_buf[2];
  int i_status;

  i_status = mmc_get_event_status(p_cdio, status_buf);
  if (i_status != DRIVER_OP_SUCCESS)
    return i_status;
  return (status_buf[0] & 0x02) ? 1 : 0;
}

char *
mmc_get_mcn ( const CdIo_t *p_cdio )
{
  if ( ! p_cdio )  return NULL;
  return mmc_get_mcn_private (p_cdio->env, p_cdio->op.run_mmc_cmd );
}

/*! 
  Find out if media tray is open or closed.
  @param p_cdio the CD object to be acted upon.
  @return 1 if media is open, 0 if closed. Error
  return codes are the same as driver_return_code_t
   */
int mmc_get_tray_status(const CdIo_t *p_cdio)
{
  uint8_t status_buf[2];
  int i_status;

  i_status = mmc_get_event_status(p_cdio, status_buf);
  if (i_status != DRIVER_OP_SUCCESS)
    return i_status;
  return (status_buf[1] & 0x01) ? 1 : 0;
}

/*!
  Run a MMC command. 
 
  cdio	        CD structure set by cdio_open().
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  buf	        Buffer for data, both sending and receiving
  len	        Size of buffer
  e_direction	direction the transfer is to go
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.
 */
driver_return_code_t
mmc_run_cmd( const CdIo_t *p_cdio, unsigned int i_timeout_ms, 
		  const mmc_cdb_t *p_cdb,
		  cdio_mmc_direction_t e_direction, unsigned int i_buf, 
		  /*in/out*/ void *p_buf )
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (!p_cdio->op.run_mmc_cmd) return DRIVER_OP_UNSUPPORTED;
  return p_cdio->op.run_mmc_cmd(p_cdio->env, i_timeout_ms,
				     mmc_get_cmd_len(p_cdb->field[0]),
				     p_cdb, e_direction, i_buf, p_buf);
}

/* Added by SukkoPera to allow CDB length to be specified manually */
driver_return_code_t
mmc_run_cmd_len( const CdIo_t *p_cdio, unsigned int i_timeout_ms,
                  const mmc_cdb_t *p_cdb, unsigned int i_cdb,
                  cdio_mmc_direction_t e_direction, unsigned int i_buf,
                  /*in/out*/ void *p_buf )
{
  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (!p_cdio->op.run_mmc_cmd) return DRIVER_OP_UNSUPPORTED;
  return p_cdio->op.run_mmc_cmd(p_cdio->env, i_timeout_ms,
                                     i_cdb,
                                     p_cdb, e_direction, i_buf, p_buf);
}


/*! Return the byte size returned on a MMC READ command (e.g. READ_10,
    READ_MSF, ..)
*/
int 
mmc_get_blocksize ( CdIo_t *p_cdio)
{
  int i_status;

  uint8_t buf[255] = { 0, };
  uint8_t *p;

  /* First try using the 6-byte MODE SENSE command. */
  i_status = mmc_mode_sense_6(p_cdio, buf, sizeof(buf), 
                              CDIO_MMC_R_W_ERROR_PAGE);
  
  if (DRIVER_OP_SUCCESS == i_status && buf[3]>=8) {
    p = &buf[4+5];
    return CDIO_MMC_GET_LEN16(p);
  }
  
  /* Next try using the 10-byte MODE SENSE command. */
  i_status = mmc_mode_sense_10(p_cdio, buf, sizeof(buf), 
                               CDIO_MMC_R_W_ERROR_PAGE);
  p = &buf[6];
  if (DRIVER_OP_SUCCESS == i_status && CDIO_MMC_GET_LEN16(p)>=8) {
    return CDIO_MMC_GET_LEN16(p);
  }

#ifdef IS_THIS_CORRECT
  /* Lastly try using the READ CAPACITY command. */
  {
    lba_t    lba = 0;
    uint16_t i_blocksize;

    i_status = mmc_read_capacity(p_cdio, &lba, &i_blocksize);
    if ( DRIVER_OP_SUCCESS == i_status )
      return i_blocksize;
#endif

  return DRIVER_OP_UNSUPPORTED;
}


/*!
 * Load or Unload media using a MMC START STOP command. 
 */
driver_return_code_t
mmc_start_stop_media(const CdIo_t *p_cdio, bool b_eject, bool b_immediate,
                     uint8_t power_condition)
{
  mmc_cdb_t cdb = {{0, }};
  uint8_t buf[1];

  if ( ! p_cdio ) return DRIVER_OP_UNINIT;
  if ( ! p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);

  if (b_immediate) cdb.field[1] |= 1;

  if (power_condition) 
    cdb.field[4] = power_condition << 4;
  else {
    if (b_eject) 
      cdb.field[4] = 2; /* eject */
    else 
      cdb.field[4] = 3; /* close tray for tray-type */
  }
  
  return p_cdio->op.run_mmc_cmd (p_cdio->env, mmc_timeout_ms,
                                 mmc_get_cmd_len(cdb.field[0]), &cdb, 
                                 SCSI_MMC_DATA_WRITE, 0, &buf);
}

/**
 * Close tray using a MMC START STOP command.
 */
driver_return_code_t 
mmc_close_tray( CdIo_t *p_cdio )
{
  if (p_cdio) {
    return mmc_start_stop_media(p_cdio, false, false, 0);
  } else {
    return DRIVER_OP_ERROR;
  }
}

/*!
 Eject using MMC commands. If CD-ROM is "locked" we'll unlock it.
 Command is not "immediate" -- we'll wait for the command to complete.
 For a more general (and lower-level) routine, @see mmc_start_stop_media.
 */
driver_return_code_t
mmc_eject_media( const CdIo_t *p_cdio )
{
  int i_status = 0;
  mmc_cdb_t cdb = {{0, }};
  uint8_t buf[1];

  if ( ! p_cdio ) return DRIVER_OP_UNINIT;
  if ( ! p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_ALLOW_MEDIUM_REMOVAL);

  i_status = p_cdio->op.run_mmc_cmd (p_cdio->env, mmc_timeout_ms,
                                     mmc_get_cmd_len(cdb.field[0]), &cdb, 
                                     SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status) return i_status;

  return mmc_start_stop_media(p_cdio, true, false, 0);
  
}

/*!
 Return a string containing the name of the given feature
 */
const char *mmc_feature2str( int i_feature )
{
  switch(i_feature) {
  case CDIO_MMC_FEATURE_PROFILE_LIST:
    return "Profile List";
  case CDIO_MMC_FEATURE_CORE: 
    return "Core";
  case CDIO_MMC_FEATURE_MORPHING:
    return "Morphing" ;
  case CDIO_MMC_FEATURE_REMOVABLE_MEDIUM:
    return "Removable Medium";
  case CDIO_MMC_FEATURE_WRITE_PROTECT:
    return "Write Protect";
  case CDIO_MMC_FEATURE_RANDOM_READABLE:
    return "Random Readable";
  case CDIO_MMC_FEATURE_MULTI_READ:
    return "Multi-Read";
  case CDIO_MMC_FEATURE_CD_READ:
    return "CD Read";
  case CDIO_MMC_FEATURE_DVD_READ:
    return "DVD Read";
  case CDIO_MMC_FEATURE_RANDOM_WRITABLE:
    return "Random Writable";
  case CDIO_MMC_FEATURE_INCR_WRITE:
    return "Incremental Streaming Writable";
  case CDIO_MMC_FEATURE_SECTOR_ERASE:
    return "Sector Erasable";
  case CDIO_MMC_FEATURE_FORMATABLE:
    return "Formattable";
  case CDIO_MMC_FEATURE_DEFECT_MGMT:
    return "Management Ability of the Logical Unit/media system "
      "to provide an apparently defect-free space.";
  case CDIO_MMC_FEATURE_WRITE_ONCE:
    return "Write Once";
  case CDIO_MMC_FEATURE_RESTRICT_OVERW:
    return "Restricted Overwrite";
  case CDIO_MMC_FEATURE_CD_RW_CAV:
    return "CD-RW CAV Write";
  case CDIO_MMC_FEATURE_MRW:
    return "MRW";
  case CDIO_MMC_FEATURE_ENHANCED_DEFECT:
    return "Enhanced Defect Reporting";
  case CDIO_MMC_FEATURE_DVD_PRW:
    return "DVD+RW";
  case CDIO_MMC_FEATURE_DVD_PR:
    return "DVD+R";
  case CDIO_MMC_FEATURE_RIGID_RES_OVERW:
    return "Rigid Restricted Overwrite";
  case CDIO_MMC_FEATURE_CD_TAO:
    return "CD Track at Once";
  case CDIO_MMC_FEATURE_CD_SAO:
    return "CD Mastering (Session at Once)";
  case CDIO_MMC_FEATURE_DVD_R_RW_WRITE:
    return "DVD-R/RW Write";
  case CDIO_MMC_FEATURE_CD_RW_MEDIA_WRITE:
    return "CD-RW Media Write Support";
  case CDIO_MMC_FEATURE_DVD_PR_2_LAYER:
    return "DVD+R Double Layer";
  case CDIO_MMC_FEATURE_POWER_MGMT:
    return "Initiator- and Device-directed Power Management";
  case CDIO_MMC_FEATURE_CDDA_EXT_PLAY:
    return "CD Audio External Play";
  case CDIO_MMC_FEATURE_MCODE_UPGRADE:
    return "Ability for the device to accept new microcode via the interface";
  case CDIO_MMC_FEATURE_TIME_OUT:
    return "Ability to respond to all commands within a specific time";
  case CDIO_MMC_FEATURE_DVD_CSS:
    return "Ability to perform DVD CSS/CPPM authentication via RPC";
  case CDIO_MMC_FEATURE_RT_STREAMING:
    return "Ability to read and write using Initiator requested performance"
      " parameters";
  case CDIO_MMC_FEATURE_LU_SN:
    return "The Logical Unit Unique Identifier";
  default: 
    {
      static char buf[100];
      if ( 0 != (i_feature & 0xFF00) ) {
        snprintf( buf, sizeof(buf),
                 "Vendor-specific code %x", i_feature );
      } else {
        snprintf( buf, sizeof(buf),
                 "Unknown code %x", i_feature );
      }
      return buf;
    }
  }
}


/*!
 Return a string containing the name of the given feature profile.
 */
const char *mmc_feature_profile2str( int i_feature_profile )
{
  switch(i_feature_profile) {
  case CDIO_MMC_FEATURE_PROF_NON_REMOVABLE:
    return "Non-removable";
  case CDIO_MMC_FEATURE_PROF_REMOVABLE:
    return "disk Re-writable; with removable media";
  case CDIO_MMC_FEATURE_PROF_MO_ERASABLE:
    return "Erasable Magneto-Optical disk with sector erase capability";
  case CDIO_MMC_FEATURE_PROF_MO_WRITE_ONCE:
    return "Write Once Magneto-Optical write once";
  case CDIO_MMC_FEATURE_PROF_AS_MO:
    return "Advance Storage Magneto-Optical";
  case CDIO_MMC_FEATURE_PROF_CD_ROM:
    return "Read only Compact Disc capable";
  case CDIO_MMC_FEATURE_PROF_CD_R:
    return "Write once Compact Disc capable";
  case CDIO_MMC_FEATURE_PROF_CD_RW:
    return "CD-RW Re-writable Compact Disc capable";
  case CDIO_MMC_FEATURE_PROF_DVD_ROM:
    return "Read only DVD";
  case CDIO_MMC_FEATURE_PROF_DVD_R_SEQ:
    return "Re-recordable DVD using Sequential recording";
  case CDIO_MMC_FEATURE_PROF_DVD_RAM:
    return "Re-writable DVD";
  case CDIO_MMC_FEATURE_PROF_DVD_RW_RO:
    return "Re-recordable DVD using Restricted Overwrite";
  case CDIO_MMC_FEATURE_PROF_DVD_RW_SEQ:
    return "Re-recordable DVD using Sequential recording";
  case CDIO_MMC_FEATURE_PROF_DVD_PRW:
    return "DVD+RW - DVD ReWritable";
  case CDIO_MMC_FEATURE_RIGID_RES_OVERW:
    return "Rigid Restricted Overwrite";
  case CDIO_MMC_FEATURE_PROF_DVD_PR:
    return "DVD+R - DVD Recordable";
  case CDIO_MMC_FEATURE_PROF_DDCD_ROM:
    return "Read only DDCD";
  case CDIO_MMC_FEATURE_PROF_DVD_PR2:
    return "DVD+R Double Layer - DVD Recordable Double Layer";
  case CDIO_MMC_FEATURE_PROF_DDCD_R:
    return "DDCD-R Write only DDCD";
  case CDIO_MMC_FEATURE_PROF_DDCD_RW:
    return "Re-Write only DDCD";
  case CDIO_MMC_FEATURE_PROF_NON_CONFORM:
    return "The Logical Unit does not conform to any Profile";
  default: 
    {
      static char buf[100];
      snprintf(buf, sizeof(buf), "Unknown Profile %x", i_feature_profile);
      return buf;
    }
  }
}


/*!
 * See if CD-ROM has feature with value value
 * @return true if we have the feature and false if not.
 */
bool_3way_t
mmc_have_interface( CdIo_t *p_cdio, cdio_mmc_feature_interface_t e_interface )
{
  int i_status;                  /* Result of MMC command */
  uint8_t buf[500] = { 0, };     /* Place to hold returned data */
  mmc_cdb_t cdb = {{0, }};  /* Command Descriptor Buffer */

  if (!p_cdio || !p_cdio->op.run_mmc_cmd) return nope;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_GET_CONFIGURATION);
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(buf));

  cdb.field[1] = CDIO_MMC_GET_CONF_NAMED_FEATURE;
  cdb.field[3] = CDIO_MMC_FEATURE_CORE;

  i_status = mmc_run_cmd(p_cdio, 0, &cdb, SCSI_MMC_DATA_READ, sizeof(buf), 
                         &buf);
  if (DRIVER_OP_SUCCESS == i_status) {
    uint8_t *p;
    uint32_t i_data;
    uint8_t *p_max = buf + 65530;
    
    i_data = (unsigned int) CDIO_MMC_GET_LEN32(buf);
    /* set to first sense feature code, and then walk through the masks */
    p = buf + 8;
    while( (p < &(buf[i_data])) && (p < p_max) ) {
      uint16_t i_feature;
      uint8_t i_feature_additional = p[3];

      i_feature = CDIO_MMC_GET_LEN16(p);
      if (CDIO_MMC_FEATURE_CORE == i_feature) {
        uint8_t *q = p+4;
        uint32_t i_interface_standard = CDIO_MMC_GET_LEN32(q);
        if (e_interface == i_interface_standard) return yep;
      }
      p += i_feature_additional + 4;
    }
    return nope;
  } else 
    return dunno;
}

/* Maximum blocks to retrieve. Would be nice to customize this based on
   drive capabilities.
*/
#define MAX_CD_READ_BLOCKS 16
#define CD_READ_TIMEOUT_MS mmc_timeout_ms * (MAX_CD_READ_BLOCKS/2)

/*! issue a MMC READ_CD command.
*/
driver_return_code_t
mmc_read_cd ( const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
              int read_sector_type, bool b_digital_audio_play,
	      bool b_sync, uint8_t header_codes, bool b_user_data, 
	      bool b_edc_ecc, uint8_t c2_error_information, 
	      uint8_t subchannel_selection, uint16_t i_blocksize, 
	      uint32_t i_blocks )
{
  mmc_cdb_t cdb = {{0, }};

  mmc_run_cmd_fn_t run_mmc_cmd;
  uint8_t i_read_type = 0;
  uint8_t cdb9 = 0;

  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (!p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  run_mmc_cmd = p_cdio->op.run_mmc_cmd;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_CD);

  i_read_type = read_sector_type << 2;
  if (b_digital_audio_play) i_read_type |= 0x2;
  
  CDIO_MMC_SET_READ_TYPE    (cdb.field, i_read_type);
  CDIO_MMC_SET_READ_LENGTH24(cdb.field, i_blocks);

  
  if (b_sync)      cdb9 |= 128;
  if (b_user_data) cdb9 |=  16;
  if (b_edc_ecc)   cdb9 |=   8;
  cdb9 |= (header_codes & 3)         << 5;
  cdb9 |= (c2_error_information & 3) << 1;
  cdb.field[9]  = cdb9;
  cdb.field[10] = (subchannel_selection & 7);
  
  {
    unsigned int j = 0;
    int i_ret = DRIVER_OP_SUCCESS;
    const uint8_t i_cdb = mmc_get_cmd_len(cdb.field[0]);
        
    while (i_blocks > 0) {
      const unsigned i_blocks2 = (i_blocks > MAX_CD_READ_BLOCKS) 
        ? MAX_CD_READ_BLOCKS : i_blocks;
      void *p_buf2 = ((char *)p_buf ) + (j * i_blocksize);
      
      CDIO_MMC_SET_READ_LBA (cdb.field, (i_lsn+j));

      i_ret = run_mmc_cmd (p_cdio->env, CD_READ_TIMEOUT_MS,
                           i_cdb, &cdb, 
                           SCSI_MMC_DATA_READ, 
                           i_blocksize * i_blocks2,
                           p_buf2);

      if (i_ret) return i_ret;

      i_blocks -= i_blocks2;
      j += i_blocks2;
    }
    return i_ret;
  }
}

/*! Read sectors using SCSI-MMC GPCMD_READ_CD.
*/
driver_return_code_t 
mmc_read_data_sectors ( CdIo_t *p_cdio, void *p_buf, 
                        lsn_t i_lsn,  uint16_t i_blocksize,
                        uint32_t i_blocks )
{
  return mmc_read_cd(p_cdio, 
                     p_buf, /* place to store data */
                     i_lsn, /* lsn */
                     0, /* read_sector_type */
                     false, /* digital audio play */
                     false, /* return sync header */
                     0,     /* header codes */
                     true,  /* return user data */
                     false, /* return EDC ECC */
                     false, /* return C2 Error information */
                     0,     /* subchannel selection bits */
                     ISO_BLOCKSIZE, /* blocksize*/ 
                     i_blocks       /* Number of blocks. */);

}

/*! Read sectors using SCSI-MMC GPCMD_READ_CD.
   Can read only up to 25 blocks.
*/
driver_return_code_t
mmc_read_sectors ( const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                   int sector_type, uint32_t i_blocks )
{
  mmc_cdb_t cdb = {{0, }};

  mmc_run_cmd_fn_t run_mmc_cmd;

  if (!p_cdio) return DRIVER_OP_UNINIT;
  if (!p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;

  run_mmc_cmd = p_cdio->op.run_mmc_cmd;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_CD);
  CDIO_MMC_SET_READ_TYPE    (cdb.field, sector_type);
  CDIO_MMC_SET_READ_LBA     (cdb.field, i_lsn);
  CDIO_MMC_SET_READ_LENGTH24(cdb.field, i_blocks);
  CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(cdb.field, 
					   CDIO_MMC_MCSB_ALL_HEADERS);

  return run_mmc_cmd (p_cdio->env, mmc_timeout_ms,
                      mmc_get_cmd_len(cdb.field[0]), &cdb, 
                      SCSI_MMC_DATA_READ, 
                      CDIO_CD_FRAMESIZE_RAW * i_blocks,
                      p_buf);
}

driver_return_code_t
mmc_set_blocksize ( const CdIo_t *p_cdio, uint16_t i_blocksize)
{
  if ( ! p_cdio )  return DRIVER_OP_UNINIT;
  return 
    mmc_set_blocksize_private (p_cdio->env, p_cdio->op.run_mmc_cmd, 
                               i_blocksize);
}


/*!
  Set the drive speed in CD-ROM speed units.
  
  @param p_cdio	   CD structure set by cdio_open().
  @param i_drive_speed   speed in CD-ROM speed units. Note this
                         not Kbytes/sec as would be used in the MMC spec or
	                 in mmc_set_speed(). To convert CD-ROM speed units 
		         to Kbs, multiply the number by 176 (for raw data)
		         and by 150 (for filesystem data). On many CD-ROM 
		         drives, specifying a value too large will result 
		         in using the fastest speed.

  @return the drive speed if greater than 0. -1 if we had an error. is -2
  returned if this is not implemented for the current driver.

   @see cdio_set_speed and mmc_set_speed
*/
driver_return_code_t 
mmc_set_drive_speed( const CdIo_t *p_cdio, int i_drive_speed )
{
  return mmc_set_speed(p_cdio, i_drive_speed * 176);
}

  
/*!
  Set the drive speed in K bytes per second. 
  
  @return the drive speed if greater than 0. -1 if we had an error. is -2
  returned if this is not implemented for the current driver.
*/
int
mmc_set_speed( const CdIo_t *p_cdio, int i_Kbs_speed )

{
  uint8_t buf[14] = { 0, };
  mmc_cdb_t cdb;

  /* If the requested speed is less than 1x 176 kb/s this command
     will return an error - it's part of the ATAPI specs. Therefore, 
     test and stop early. */

  if ( i_Kbs_speed < 176 ) return -1;
  
  memset(&cdb, 0, sizeof(mmc_cdb_t));
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_SET_SPEED);
  CDIO_MMC_SET_LEN16(cdb.field, 2, i_Kbs_speed);
  /* Some drives like the Creative 24x CDRW require one to set a
     nonzero write speed or else one gets an error back.  Some
     specifications have setting the value 0xfffff indicate setting to
     the maximum allowable speed.
  */
  CDIO_MMC_SET_LEN16(cdb.field, 4, 0xffff);
  return mmc_run_cmd(p_cdio, 2000, &cdb, SCSI_MMC_DATA_WRITE, 
                     sizeof(buf), buf);
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
 
 #ifdef WIN32
void* _alloca(size_t size)
{
  return (void*)alloca(size);
}
 #endif
