/*
    $Id: freebsd_cam.c,v 1.11 2006/03/03 09:50:30 flameeyes Exp $

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

/* This file contains FreeBSD-specific code and implements low-level
   control of the CD drive via SCSI emulation.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: freebsd_cam.c,v 1.11 2006/03/03 09:50:30 flameeyes Exp $";

#ifdef HAVE_FREEBSD_CDROM

#include "freebsd.h"
#include <cdio/mmc.h>

/* Default value in seconds we will wait for a command to 
   complete. */
#define DEFAULT_TIMEOUT_MSECS 10000

/*!
  Run a SCSI MMC command. 
 
  p_user_data   internal CD structure.
  i_timeout_ms  time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  i_cdb	        Size of p_cdb
  p_cdb	        CDB bytes. 
  e_direction	direction the transfer is to go.
  i_buf	        Size of buffer
  p_buf	        Buffer for data, both sending and receiving

  Return 0 if no error.
 */
int
run_mmc_cmd_freebsd_cam( const void *p_user_data, unsigned int i_timeout_ms,
			 unsigned int i_cdb, const mmc_cdb_t *p_cdb, 
			 cdio_mmc_direction_t e_direction, 
			 unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  int   i_status;
  int direction = CAM_DEV_QFRZDIS;
  union ccb ccb;

  if (!p_env || !p_env->cam) return -2;
    
  memset(&ccb, 0, sizeof(ccb));

  ccb.ccb_h.path_id    = p_env->cam->path_id;
  ccb.ccb_h.target_id  = p_env->cam->target_id;
  ccb.ccb_h.target_lun = p_env->cam->target_lun;
  ccb.ccb_h.timeout    = i_timeout_ms;

  if (!i_buf)
    direction |= CAM_DIR_NONE;
  else
    direction |= (e_direction == SCSI_MMC_DATA_READ)?CAM_DIR_IN : CAM_DIR_OUT;

 
   memcpy(ccb.csio.cdb_io.cdb_bytes, p_cdb->field, i_cdb);
   ccb.csio.cdb_len =
     mmc_get_cmd_len(ccb.csio.cdb_io.cdb_bytes[0]);
   
  cam_fill_csio (&(ccb.csio), 1, NULL, 
		 direction | CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, p_buf, i_buf, 
 		 sizeof(ccb.csio.sense_data), ccb.csio.cdb_len, 30*1000);

  if (cam_send_ccb(p_env->cam, &ccb) < 0)
    {
      cdio_warn ("transport failed: %s", strerror(errno));
      return -1;
    }
  if ((ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
    {
      return 0;
    }
  errno = EIO;
  i_status = ERRCODE(((unsigned char *)&ccb.csio.sense_data));
  if (i_status == 0)
    i_status = -1;
  else
    CREAM_ON_ERRNO(((unsigned char *)&ccb.csio.sense_data));
  cdio_warn ("transport failed: %d", i_status);
  return i_status;
}

bool
init_freebsd_cam (_img_private_t *p_env)
{
  char pass[100];
  
  p_env->cam=NULL;
  memset (&p_env->ccb, 0, sizeof(p_env->ccb));
  p_env->ccb.ccb_h.func_code = XPT_GDEVLIST;

  if (-1 == p_env->gen.fd) 
    p_env->gen.fd = open (p_env->device, O_RDONLY, 0);

  if (p_env->gen.fd < 0)
    {
      cdio_warn ("open (%s): %s", p_env->device, strerror (errno));
      return false;
    }

  if (ioctl (p_env->gen.fd, CAMGETPASSTHRU, &p_env->ccb) < 0)
    {
      cdio_warn ("open: %s", strerror (errno));
      return false;
    }
  sprintf (pass,"/dev/%.15s%u",
	   p_env->ccb.cgdl.periph_name,
	   p_env->ccb.cgdl.unit_number);
  p_env->cam = cam_open_pass (pass,O_RDWR,NULL);
  if (!p_env->cam) return false;
  
  p_env->gen.init   = true;
  p_env->b_cam_init = true;
  return true;
}

void
free_freebsd_cam (void *user_data)
{
  _img_private_t *p_env = user_data;

  if (NULL == p_env) return;

  if (p_env->gen.fd > 0)
    close (p_env->gen.fd);
  p_env->gen.fd = -1;

  if(p_env->cam)
    cam_close_device(p_env->cam);

  free (p_env);
}

driver_return_code_t
read_mode2_sector_freebsd_cam (_img_private_t *p_env, void *data, lsn_t lsn, 
			       bool b_form2)
{
  if ( b_form2 )
    return read_mode2_sectors_freebsd_cam(p_env, data, lsn, 1);
  else {
    /* Need to pick out the data portion from a mode2 form2 frame */
    char buf[M2RAW_SECTOR_SIZE] = { 0, };
    int retval = read_mode2_sectors_freebsd_cam(p_env, buf, lsn, 1);
    if ( retval ) return retval;
    memcpy (((char *)data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
    return DRIVER_OP_SUCCESS;
  }
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
int
read_mode2_sectors_freebsd_cam (_img_private_t *p_env, void *p_buf, 
				lsn_t lsn, unsigned int nblocks)
{
  mmc_cdb_t cdb = {{0, }};

  bool b_read_10 = false;

  CDIO_MMC_SET_READ_LBA(cdb.field, lsn);
  
  if (b_read_10) {
    int retval;
    
    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_10);
    CDIO_MMC_SET_READ_LENGTH16(cdb.field, nblocks);
    if ((retval = mmc_set_blocksize (p_env->gen.cdio, M2RAW_SECTOR_SIZE)))
      return retval;
    
    if ((retval = run_mmc_cmd_freebsd_cam (p_env, 0, 
					   mmc_get_cmd_len(cdb.field[0]),
					   &cdb, 
					   SCSI_MMC_DATA_READ,
					   M2RAW_SECTOR_SIZE * nblocks, 
					   p_buf)))
      {
	mmc_set_blocksize (p_env->gen.cdio, CDIO_CD_FRAMESIZE);
	return retval;
      }
    
    return mmc_set_blocksize (p_env->gen.cdio, CDIO_CD_FRAMESIZE);
  } else {
    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_CD);
    CDIO_MMC_SET_READ_LENGTH24(cdb.field, nblocks);
    cdb.field[1] = 0; /* sector size mode2 */
    cdb.field[9] = 0x58; /* 2336 mode2 */
    return run_mmc_cmd_freebsd_cam (p_env, 0, 
				    mmc_get_cmd_len(cdb.field[0]), 
				    &cdb, 
				    SCSI_MMC_DATA_READ,
				    M2RAW_SECTOR_SIZE * nblocks, p_buf);
    
  }
}

/*!
  Eject media in CD-ROM drive. Return DRIVER_OP_SUCCESS if successful, 
  DRIVER_OP_ERROR on error.
 */
driver_return_code_t
eject_media_freebsd_cam (_img_private_t *p_env) 
{
  int i_status;
  mmc_cdb_t cdb = {{0, }};
  uint8_t buf[1];
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_ALLOW_MEDIUM_REMOVAL);

  i_status = run_mmc_cmd_freebsd_cam (p_env, DEFAULT_TIMEOUT_MSECS,
				      mmc_get_cmd_len(cdb.field[0]), 
				      &cdb, SCSI_MMC_DATA_WRITE, 0, &buf);
  if (i_status) return i_status;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);
  cdb.field[4] = 1;
  i_status = run_mmc_cmd_freebsd_cam (p_env, DEFAULT_TIMEOUT_MSECS,
				 mmc_get_cmd_len(cdb.field[0]), &cdb, 
				 SCSI_MMC_DATA_WRITE, 0, &buf);
  if (i_status) return i_status;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);
  cdb.field[4] = 2; /* eject */

  return run_mmc_cmd_freebsd_cam (p_env, DEFAULT_TIMEOUT_MSECS,
				  mmc_get_cmd_len(cdb.field[0]), 
				  &cdb, 
				  SCSI_MMC_DATA_WRITE, 0, &buf);
}

#endif /* HAVE_FREEBSD_CDROM */
