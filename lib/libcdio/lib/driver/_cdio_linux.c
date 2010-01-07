/*
    $Id: _cdio_linux.c,v 1.19 2005/01/24 00:06:31 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002, 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>

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

/* This file contains Linux-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_linux.c,v 1.19 2005/01/24 00:06:31 rocky Exp $";

#include <string.h>

#include <cdio/sector.h>
#include <cdio/util.h>
#include <cdio/types.h>
#include <cdio/scsi_mmc.h>
#include <cdio/cdtext.h>
#include "cdtext_private.h"
#include "cdio_assert.h"
#include "cdio_private.h"

#ifdef HAVE_LINUX_CDROM

#if defined(HAVE_LINUX_VERSION_H)
# include <linux/version.h>
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,16)
#   define __CDIO_LINUXCD_BUILD
# else
#  error "You need a kernel greater than 2.2.16 to have CDROM support"
# endif
#else 
#  error "You need <linux/version.h> to have CDROM support"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <mntent.h>

#include <linux/cdrom.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>
#include <sys/mount.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

typedef enum {
  _AM_NONE,
  _AM_IOCTL,
  _AM_READ_CD,
  _AM_READ_10
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  access_mode_t access_mode;

  /* Some of the more OS specific things. */
  /* Entry info for each track, add 1 for leadout. */
  struct cdrom_tocentry  tocent[CDIO_CD_MAX_TRACKS+1]; 

  struct cdrom_tochdr    tochdr;

} _img_private_t;

/* Some ioctl() errno values which occur when the tray is empty */
#define ERRNO_TRAYEMPTY(errno)	\
	((errno == EIO) || (errno == ENOENT) || (errno == EINVAL))

/**** prototypes for static functions ****/
static bool is_cdrom_linux(const char *drive, char *mnttype);
static bool read_toc_linux (void *p_user_data);
static driver_return_code_t run_scsi_cmd_linux( void *p_user_data, 
                                                unsigned int i_timeout,
                                                unsigned int i_cdb, 
                                                const scsi_mmc_cdb_t *p_cdb, 
                                                scsi_mmc_direction_t e_direction, 
                                                unsigned int i_buf, 
                                                /*in/out*/ void *p_buf );
static access_mode_t 

str_to_access_mode_linux(const char *psz_access_mode) 
{
  const access_mode_t default_access_mode = _AM_IOCTL;

  if (NULL==psz_access_mode) return default_access_mode;
  
  if (!strcmp(psz_access_mode, "IOCTL"))
    return _AM_IOCTL;
  else if (!strcmp(psz_access_mode, "READ_CD"))
    return _AM_READ_CD;
  else if (!strcmp(psz_access_mode, "READ_10"))
    return _AM_READ_10;
  else {
    cdio_warn ("unknown access type: %s. Default IOCTL used.", 
	       psz_access_mode);
    return default_access_mode;
  }
}

static char *
check_mounts_linux(const char *mtab)
{
  FILE *mntfp;
  struct mntent *mntent;
  
  mntfp = setmntent(mtab, "r");
  if ( mntfp != NULL ) {
    char *tmp;
    char *mnt_type;
    char *mnt_dev;
    
    while ( (mntent=getmntent(mntfp)) != NULL ) {
      mnt_type = malloc(strlen(mntent->mnt_type) + 1);
      if (mnt_type == NULL)
	continue;  /* maybe you'll get lucky next time. */
      
      mnt_dev = malloc(strlen(mntent->mnt_fsname) + 1);
      if (mnt_dev == NULL) {
	free(mnt_type);
	continue;
      }
      
      strcpy(mnt_type, mntent->mnt_type);
      strcpy(mnt_dev, mntent->mnt_fsname);
      
      /* Handle "supermount" filesystem mounts */
      if ( strcmp(mnt_type, "supermount") == 0 ) {
	tmp = strstr(mntent->mnt_opts, "fs=");
	if ( tmp ) {
	  free(mnt_type);
	  mnt_type = strdup(tmp + strlen("fs="));
	  if ( mnt_type ) {
	    tmp = strchr(mnt_type, ',');
	    if ( tmp ) {
	      *tmp = '\0';
	    }
	  }
	}
	tmp = strstr(mntent->mnt_opts, "dev=");
	if ( tmp ) {
	  free(mnt_dev);
	  mnt_dev = strdup(tmp + strlen("dev="));
	  if ( mnt_dev ) {
	    tmp = strchr(mnt_dev, ',');
	    if ( tmp ) {
	      *tmp = '\0';
	    }
	  }
	}
      }
      if ( strcmp(mnt_type, "iso9660") == 0 ) {
	if (is_cdrom_linux(mnt_dev, mnt_type) > 0) {
	  free(mnt_type);
	  endmntent(mntfp);
	  return mnt_dev;
	}
      }
      free(mnt_dev);
      free(mnt_type);
    }
    endmntent(mntfp);
  }
  return NULL;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
get_arg_linux (void *env, const char key[])
{
  _img_private_t *_obj = env;

  if (!strcmp (key, "source")) {
    return _obj->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (_obj->access_mode) {
    case _AM_IOCTL:
      return "ioctl";
    case _AM_READ_CD:
      return "READ_CD";
    case _AM_READ_10:
      return "READ_10";
    case _AM_NONE:
      return "no access method";
    }
  } 
  return NULL;
}

#undef USE_LINUX_CAP
#ifdef USE_LINUX_CAP
/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static void
get_drive_cap_linux (const void *p_user_data,
		     /*out*/ cdio_drive_read_cap_t  *p_read_cap,
		     /*out*/ cdio_drive_write_cap_t *p_write_cap,
		     /*out*/ cdio_drive_misc_cap_t  *p_misc_cap)
{
  const _img_private_t *p_env = p_user_data;
  int32_t i_drivetype;

  i_drivetype = ioctl (p_env->gen.fd, CDROM_GET_CAPABILITY, CDSL_CURRENT);

  if (i_drivetype < 0) {
    *p_read_cap  = CDIO_DRIVE_CAP_ERROR;
    *p_write_cap = CDIO_DRIVE_CAP_ERROR;
    *p_misc_cap  = CDIO_DRIVE_CAP_ERROR;
    return;
  }
  
  *p_read_cap  = 0;
  *p_write_cap = 0;
  *p_misc_cap  = 0;

  /* Reader */
  if (i_drivetype & CDC_PLAY_AUDIO) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_AUDIO;
  if (i_drivetype & CDC_CD_R) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_R;
  if (i_drivetype & CDC_CD_RW) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_RW;
  if (i_drivetype & CDC_DVD) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_DVD_ROM;

  /* Writer */
  if (i_drivetype & CDC_CD_RW) 
    *p_read_cap  |= CDIO_DRIVE_CAP_WRITE_CD_RW;
  if (i_drivetype & CDC_DVD_R) 
    *p_read_cap  |= CDIO_DRIVE_CAP_WRITE_DVD_R;
  if (i_drivetype & CDC_DVD_RAM) 
    *p_read_cap  |= CDIO_DRIVE_CAP_WRITE_DVD_RAM;

  /* Misc */
  if (i_drivetype & CDC_CLOSE_TRAY) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_CLOSE_TRAY;
  if (i_drivetype & CDC_OPEN_TRAY) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_EJECT;
  if (i_drivetype & CDC_LOCK) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_LOCK;
  if (i_drivetype & CDC_SELECT_SPEED) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_SELECT_SPEED;
  if (i_drivetype & CDC_SELECT_DISC) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_SELECT_DISC;
  if (i_drivetype & CDC_MULTI_SESSION) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_MULTI_SESSION;
  if (i_drivetype & CDC_MEDIA_CHANGED) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED;
  if (i_drivetype & CDC_RESET) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_RESET;
}
#endif

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static char *
get_mcn_linux (const void *p_user_data) {

  struct cdrom_mcn mcn;
  const _img_private_t *p_env = p_user_data;
  memset(&mcn, 0, sizeof(mcn));
  if (ioctl(p_env->gen.fd, CDROM_GET_MCN, &mcn) != 0)
    return NULL;
  return strdup(mcn.medium_catalog_number);
}

/*!  
  Get format of track. 
*/
static track_format_t
get_track_format_linux(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if ( !p_env ) return TRACK_FORMAT_ERROR;

  if (!p_env->gen.toc_init) read_toc_linux (p_user_data) ;

  if (i_track > (p_env->gen.i_tracks+p_env->gen.i_first_track) 
      || i_track < p_env->gen.i_first_track)
    return TRACK_FORMAT_ERROR;

  i_track -= p_env->gen.i_first_track;

  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
   */
  if (p_env->tocent[i_track].cdte_ctrl & CDIO_CDROM_DATA_TRACK) {
    if (p_env->tocent[i_track].cdte_format == CDIO_CDROM_CDI_TRACK)
      return TRACK_FORMAT_CDI;
    else if (p_env->tocent[i_track].cdte_format == CDIO_CDROM_XA_TRACK)
      return TRACK_FORMAT_XA;
    else
      return TRACK_FORMAT_DATA;
  } else
    return TRACK_FORMAT_AUDIO;
  
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8

  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
get_track_green_linux(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if (!p_env->gen.toc_init) read_toc_linux (p_user_data) ;

  if (i_track >= (p_env->gen.i_tracks+p_env->gen.i_first_track) 
      || i_track < p_env->gen.i_first_track)
    return false;

  i_track -= p_env->gen.i_first_track;

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cd-info for a while.
   */
  return ((p_env->tocent[i_track].cdte_ctrl & 2) != 0);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers usually start at something 
  greater than 0, usually 1.

  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static bool
get_track_msf_linux(void *p_user_data, track_t i_track, msf_t *msf)
{
  _img_private_t *p_env = p_user_data;

  if (NULL == msf) return false;

  if (!p_env->gen.toc_init) read_toc_linux (p_user_data) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) 
    i_track = p_env->gen.i_tracks + p_env->gen.i_first_track;

  if (i_track > (p_env->gen.i_tracks+p_env->gen.i_first_track) 
      || i_track < p_env->gen.i_first_track) {
    return false;
  } else {
    struct cdrom_msf0  *msf0= 
      &p_env->tocent[i_track-p_env->gen.i_first_track].cdte_addr.msf;
    msf->m = cdio_to_bcd8(msf0->minute);
    msf->s = cdio_to_bcd8(msf0->second);
    msf->f = cdio_to_bcd8(msf0->frame);
    return true;
  }
}

/*!
  Eject media in CD drive. 
 */
static driver_return_code_t
eject_media_linux (void *p_user_data) {

  _img_private_t *p_env = p_user_data;
  driver_return_code_t ret=DRIVER_OP_SUCCESS;
  int status;
  int fd;

  if ((fd = open (p_env->gen.source_name, O_RDONLY|O_NONBLOCK)) > -1) {
    if((status = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT)) > 0) {
      switch(status) {
      case CDS_TRAY_OPEN:
	if((ret = ioctl(fd, CDROMCLOSETRAY)) != 0) {
	  cdio_warn ("ioctl CDROMCLOSETRAY failed: %s\n", strerror(errno));  
	  ret = DRIVER_OP_ERROR;
	}
	break;
      case CDS_DISC_OK:
	if((ret = ioctl(fd, CDROMEJECT)) != 0) {
	  int eject_error = errno;
	  /* Try ejecting the MMC way... */
	  ret = scsi_mmc_eject_media(p_env->gen.cdio);
	  if (0 != ret) {
	    cdio_warn("ioctl CDROMEJECT failed: %s\n", 
		      strerror(eject_error));
	    ret = DRIVER_OP_ERROR;
	  }
	}
	/* force kernel to reread partition table when new disc inserted */
	ret = ioctl(p_env->gen.fd, BLKRRPART);
	break;
      default:
	cdio_warn ("Unknown CD-ROM (%d)\n", status);
	ret = DRIVER_OP_ERROR;
      }
    } else {
      cdio_warn ("CDROM_DRIVE_STATUS failed: %s\n", strerror(errno));
      ret=DRIVER_OP_ERROR;
    }
    close(fd);
  } else
    ret = DRIVER_OP_ERROR;
  close(p_env->gen.fd);
  p_env->gen.fd = -1;
  return ret;
}

/*! 
  Get disc type associated with the cd object.
*/
static discmode_t
get_discmode_linux (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  discmode_t discmode = CDIO_DISC_MODE_NO_INFO;

  /* See if this is a DVD. */
  cdio_dvd_struct_t dvd;  /* DVD READ STRUCT for layer 0. */

  dvd.physical.type = CDIO_DVD_STRUCT_PHYSICAL;
  dvd.physical.layer_num = 0;
  if (0 == ioctl (p_env->gen.fd, DVD_READ_STRUCT, &dvd)) {
    switch(dvd.physical.layer[0].book_type) {
    case CDIO_DVD_BOOK_DVD_ROM:  return CDIO_DISC_MODE_DVD_ROM;
    case CDIO_DVD_BOOK_DVD_RAM:  return CDIO_DISC_MODE_DVD_RAM;
    case CDIO_DVD_BOOK_DVD_R:    return CDIO_DISC_MODE_DVD_R;
    case CDIO_DVD_BOOK_DVD_RW:   return CDIO_DISC_MODE_DVD_RW;
    case CDIO_DVD_BOOK_DVD_PR:   return CDIO_DISC_MODE_DVD_PR;
    case CDIO_DVD_BOOK_DVD_PRW:  return CDIO_DISC_MODE_DVD_PRW;
    default:                     return CDIO_DISC_MODE_DVD_OTHER;
    }
  }

  /* 
     Justin B Ruggles <jruggle@earthlink.net> reports that the
     GNU/Linux ioctl(.., CDROM_DISC_STATUS) does not return "CD DATA
     Form 2" for SVCD's even though they are are form 2. There we
     issue a SCSI MMC-2 FULL TOC command first to try get more
     accurate information.
  */
  discmode = scsi_mmc_get_discmode(p_env->gen.cdio);
  if (CDIO_DISC_MODE_NO_INFO != discmode) 
    return discmode;
  else {
    int32_t i_discmode = ioctl (p_env->gen.fd, CDROM_DISC_STATUS);

    if (i_discmode < 0) return CDIO_DISC_MODE_ERROR;
    
    /* FIXME Need to add getting DVD types. */
    switch(i_discmode) {
    case CDS_AUDIO:
      return CDIO_DISC_MODE_CD_DA;
    case CDS_DATA_1:
    case CDS_DATA_2: /* Actually, recent GNU/Linux kernels don't return 
			CDS_DATA_2, but just in case. */
      return CDIO_DISC_MODE_CD_DATA;
    case CDS_MIXED:
      return CDIO_DISC_MODE_CD_MIXED;
    case CDS_XA_2_1:
    case CDS_XA_2_2: 
      return CDIO_DISC_MODE_CD_XA;
    case CDS_NO_INFO:
      return CDIO_DISC_MODE_NO_INFO;
    default:
      return CDIO_DISC_MODE_ERROR;
    }
  }
}

/* Check a drive to see if it is a CD-ROM 
   Return 1 if a CD-ROM. 0 if it exists but isn't a CD-ROM drive
   and -1 if no device exists .
*/
static bool
is_cdrom_linux(const char *drive, char *mnttype)
{
  bool is_cd=false;
  int cdfd;
  struct cdrom_tochdr    tochdr;
  
  /* If it doesn't exist, return -1 */
  if ( !cdio_is_device_quiet_generic(drive) ) {
    return(false);
  }
  
  /* If it does exist, verify that it's an available CD-ROM */
  cdfd = open(drive, (O_RDONLY|O_NONBLOCK), 0);
  if ( cdfd >= 0 ) {
    if ( ioctl(cdfd, CDROMREADTOCHDR, &tochdr) != -1 ) {
      is_cd = true;
    }
    close(cdfd);
    }
  /* Even if we can't read it, it might be mounted */
  else if ( mnttype && (strcmp(mnttype, "iso9660") == 0) ) {
    is_cd = true;
  }
  return(is_cd);
}

/* MMC driver to read audio sectors. 
   Can read only up to 25 blocks.
*/
static driver_return_code_t
_read_audio_sectors_linux (void *p_user_data, void *buf, lsn_t lsn, 
			   unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  return scsi_mmc_read_sectors( p_env->gen.cdio, buf, lsn, 
				CDIO_MMC_READ_TYPE_CDDA, nblocks);
}

/* Packet driver to read mode2 sectors. 
   Can read only up to 25 blocks.
*/
static driver_return_code_t
_read_mode2_sectors_mmc (_img_private_t *p_env, void *p_buf, lba_t lba, 
			 unsigned int nblocks, bool b_read_10)
{
  scsi_mmc_cdb_t cdb = {{0, }};

  CDIO_MMC_SET_READ_LBA(cdb.field, lba);

  if (b_read_10) {
    int retval;
    
    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_10);
    CDIO_MMC_SET_READ_LENGTH16(cdb.field, nblocks);

    if ((retval = scsi_mmc_set_blocksize (p_env->gen.cdio, M2RAW_SECTOR_SIZE)))
      return retval;
    
    if ((retval = run_scsi_cmd_linux (p_env, 0, 
				      scsi_mmc_get_cmd_len(cdb.field[0]),
				      &cdb, 
				      SCSI_MMC_DATA_READ,
				      M2RAW_SECTOR_SIZE * nblocks, 
				      p_buf)))
      {
	scsi_mmc_set_blocksize (p_env->gen.cdio, CDIO_CD_FRAMESIZE);
	return retval;
      }
    
    /* Restore blocksize. */
    retval = scsi_mmc_set_blocksize (p_env->gen.cdio, CDIO_CD_FRAMESIZE);
    return retval;
  } else {

    cdb.field[1] = 0; /* sector size mode2 */
    cdb.field[9] = 0x58; /* 2336 mode2 */

    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_CD);
    CDIO_MMC_SET_READ_LENGTH24(cdb.field, nblocks);

    return run_scsi_cmd_linux (p_env, 0, 
			       scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
			       SCSI_MMC_DATA_READ,
			       M2RAW_SECTOR_SIZE * nblocks, p_buf);
  }
}

static driver_return_code_t
_read_mode2_sectors (_img_private_t *p_env, void *p_buf, lba_t lba, 
		     unsigned int nblocks, bool b_read_10)
{
  unsigned int l = 0;
  int retval = 0;

  while (nblocks > 0)
    {
      const unsigned nblocks2 = (nblocks > 25) ? 25 : nblocks;
      void *p_buf2 = ((char *)p_buf ) + (l * M2RAW_SECTOR_SIZE);
      
      retval |= _read_mode2_sectors_mmc (p_env, p_buf2, lba + l, 
					 nblocks2, b_read_10);

      if (retval)
	break;

      nblocks -= nblocks2;
      l += nblocks2;
    }

  return retval;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sector_linux (void *p_user_data, void *p_data, lsn_t lsn, 
			 bool b_form2)
{

#if FIXED
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  struct cdrom_msf *p_msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *p_env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = cdio_from_bcd8(_msf.m);
  msf->cdmsf_sec0 = cdio_from_bcd8(_msf.s);
  msf->cdmsf_frame0 = cdio_from_bcd8(_msf.f);

 retry:
  switch (p_env->access_mode)
    {
    case _AM_NONE:
      cdio_warn ("no way to read mode1");
      return 1;
      break;
      
    case _AM_IOCTL:
      if (ioctl (p_env->gen.fd, CDROMREADMODE1, &buf) == -1)
	{
	  perror ("ioctl()");
	  return 1;
	  /* exit (EXIT_FAILURE); */
	}
      break;
      
    case _AM_READ_CD:
    case _AM_READ_10:
      if (_read_mode2_sectors (p_env->gen.fd, buf, lsn, 1, 
				      (p_env->access_mode == _AM_READ_10)))
	{
	  perror ("ioctl()");
	  if (p_env->access_mode == _AM_READ_CD)
	    {
	      cdio_info ("READ_CD failed; switching to READ_10 mode...");
	      p_env->access_mode = _AM_READ_10;
	      goto retry;
	    }
	  else
	    {
	      cdio_info ("READ_10 failed; switching to ioctl(CDROMREADMODE2) mode...");
	      p_env->access_mode = _AM_IOCTL;
	      goto retry;
	    }
	  return 1;
	}
      break;
    }

  memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
  
#else
  return cdio_generic_read_form1_sector(p_user_data, p_data, lsn);
#endif
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sectors_linux (void *p_user_data, void *p_data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode1_sector_linux (p_env,
					    ((char *)p_data) + (blocksize*i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sector_linux (void *p_user_data, void *p_data, lsn_t lsn, 
			  bool b_form2)
{
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *p_env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = cdio_from_bcd8(_msf.m);
  msf->cdmsf_sec0 = cdio_from_bcd8(_msf.s);
  msf->cdmsf_frame0 = cdio_from_bcd8(_msf.f);

 retry:
  switch (p_env->access_mode)
    {
    case _AM_NONE:
      cdio_warn ("no way to read mode2");
      return 1;
      break;
      
    case _AM_IOCTL:
      if (ioctl (p_env->gen.fd, CDROMREADMODE2, &buf) == -1)
	{
	  perror ("ioctl()");
	  return 1;
	  /* exit (EXIT_FAILURE); */
	}
      break;
      
    case _AM_READ_CD:
    case _AM_READ_10:
      if (_read_mode2_sectors (p_env, buf, lsn, 1, 
			       (p_env->access_mode == _AM_READ_10)))
	{
	  perror ("ioctl()");
	  if (p_env->access_mode == _AM_READ_CD)
	    {
	      cdio_info ("READ_CD failed; switching to READ_10 mode...");
	      p_env->access_mode = _AM_READ_10;
	      goto retry;
	    }
	  else
	    {
	      cdio_info ("READ_10 failed; switching to ioctl(CDROMREADMODE2) mode...");
	      p_env->access_mode = _AM_IOCTL;
	      goto retry;
	    }
	  return 1;
	}
      break;
    }

  if (b_form2)
    memcpy (p_data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)p_data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
  
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sectors_linux (void *p_user_data, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  unsigned int i_blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  /* For each frame, pick out the data part we need */
  for (i = 0; i < nblocks; i++) {
    int retval;
    if ( (retval = _read_mode2_sector_linux (p_env, 
					    ((char *)data) + (i_blocksize*i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return DRIVER_OP_SUCCESS;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return false if successful or true if an error.
*/
static bool
read_toc_linux (void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  int i;

  /* read TOC header */
  if ( ioctl(p_env->gen.fd, CDROMREADTOCHDR, &p_env->tochdr) == -1 ) {
    cdio_warn("%s: %s\n", 
            "error in ioctl CDROMREADTOCHDR", strerror(errno));
    return false;
  }

  p_env->gen.i_first_track = p_env->tochdr.cdth_trk0;
  p_env->gen.i_tracks      = p_env->tochdr.cdth_trk1;

  /* read individual tracks */
  for (i= p_env->gen.i_first_track; i<=p_env->gen.i_tracks; i++) {
    struct cdrom_tocentry *p_toc = 
      &(p_env->tocent[i-p_env->gen.i_first_track]);
    
    p_toc->cdte_track = i;
    p_toc->cdte_format = CDROM_MSF;
    if ( ioctl(p_env->gen.fd, CDROMREADTOCENTRY, p_toc) == -1 ) {
      cdio_warn("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }

    set_track_flags(&(p_env->gen.track_flags[i]), p_toc->cdte_ctrl);
    
    /****
    struct cdrom_msf0 *msf= &env->tocent[i-1].cdte_addr.msf;
    
    fprintf (stdout, "--- track# %d (msf %2.2x:%2.2x:%2.2x)\n",
	     i, msf->minute, msf->second, msf->frame);
    ****/

  }

  /* read the lead-out track */
  p_env->tocent[p_env->gen.i_tracks].cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  p_env->tocent[p_env->gen.i_tracks].cdte_format = CDROM_MSF;

  if (ioctl(p_env->gen.fd, CDROMREADTOCENTRY, 
	    &p_env->tocent[p_env->gen.i_tracks]) == -1 ) {
    cdio_warn("%s: %s\n", 
	     "error in ioctl CDROMREADTOCENTRY for lead-out",
            strerror(errno));
    return false;
  }

  /*
  struct cdrom_msf0 *msf= &env->tocent[p_env->gen.i_tracks].cdte_addr.msf;

  fprintf (stdout, "--- track# %d (msf %2.2x:%2.2x:%2.2x)\n",
	   i, msf->minute, msf->second, msf->frame);
  */

  p_env->gen.toc_init = true;
  return true;
}

/*!
  Run a SCSI MMC command. 
 
  cdio	        CD structure set by cdio_open().
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  We return true if command completed successfully and false if not.
 */
static driver_return_code_t
run_scsi_cmd_linux( void *p_user_data, 
		    unsigned int i_timeout_ms,
		    unsigned int i_cdb, const scsi_mmc_cdb_t *p_cdb, 
		    scsi_mmc_direction_t e_direction, 
		    unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  struct cdrom_generic_command cgc;
  memset (&cgc, 0, sizeof (struct cdrom_generic_command));
  memcpy(&cgc.cmd, p_cdb, i_cdb);
  cgc.buflen = i_buf;
  cgc.buffer = p_buf;
  cgc.data_direction = (SCSI_MMC_DATA_READ == cgc.data_direction)
    ? CGC_DATA_READ : CGC_DATA_WRITE;

#ifdef HAVE_LINUX_CDROM_TIMEOUT
  cgc.timeout = i_timeout_ms;
#endif

  return ioctl (p_env->gen.fd, CDROM_SEND_PACKET, &cgc);
}

/*!
   Return the size of the CD in logical block address (LBA) units.
   @return the lsn. On error return CDIO_INVALID_LSN.
 */
static lsn_t
get_disc_last_lsn_linux (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  struct cdrom_tocentry tocent;
  uint32_t i_size;

  tocent.cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  tocent.cdte_format = CDROM_LBA;
  if (ioctl (p_env->gen.fd, CDROMREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      return CDIO_INVALID_LSN;
    }

  i_size = tocent.cdte_addr.lba;

  return i_size;
}

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" and "access-mode" are valid keys.
  "source" sets the source device in I/O operations 
  "access-mode" sets the the method of CD access 

  DRIVER_OP_SUCCESS is returned if no error was found, 
  and nonzero if there as an error.
*/
static driver_return_code_t
set_arg_linux (void *p_user_data, const char key[], const char value[])
{
  _img_private_t *p_env = p_user_data;

  if (!strcmp (key, "source"))
    {
      if (!value) return DRIVER_OP_ERROR;
      free (p_env->gen.source_name);
      p_env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      return str_to_access_mode_linux(value);
    }
  else return DRIVER_OP_ERROR;

  return DRIVER_OP_SUCCESS;
}

/* checklist: /dev/cdrom, /dev/dvd /dev/hd?, /dev/scd? /dev/sr? */
static char checklist1[][40] = {
  {"cdrom"}, {"dvd"}, {""}
};
static char checklist2[][40] = {
  {"?a hd?"}, {"?0 scd?"}, {"?0 sr?"}, {""}
};

/* Set CD-ROM drive speed */
static driver_return_code_t
set_speed_linux (void *p_user_data, int i_speed)
{
  const _img_private_t *p_env = p_user_data;

  if (!p_env) return DRIVER_OP_UNINIT;
  return ioctl(p_env->gen.fd, CDROM_SELECT_SPEED, i_speed);
}

#endif /* HAVE_LINUX_CDROM */

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_linux (void)
{
#ifndef HAVE_LINUX_CDROM
  return NULL;
#else
  unsigned int i;
  char drive[40];
  char *ret_drive;
  bool exists;
  char **drives = NULL;
  unsigned int num_drives=0;
  
  /* Scan the system for CD-ROM drives.
  */
  for ( i=0; strlen(checklist1[i]) > 0; ++i ) {
    sprintf(drive, "/dev/%s", checklist1[i]);
    if ( (exists=is_cdrom_linux(drive, NULL)) > 0 ) {
      cdio_add_device_list(&drives, drive, &num_drives);
    }
  }

  /* Now check the currently mounted CD drives */
  if (NULL != (ret_drive = check_mounts_linux("/etc/mtab"))) {
    cdio_add_device_list(&drives, ret_drive, &num_drives);
    free(ret_drive);
  }
  
  /* Finally check possible mountable drives in /etc/fstab */
  if (NULL != (ret_drive = check_mounts_linux("/etc/fstab"))) {
    cdio_add_device_list(&drives, ret_drive, &num_drives);
    free(ret_drive);
  }

  /* Scan the system for CD-ROM drives.
     Not always 100% reliable, so use the USE_MNTENT code above first.
  */
  for ( i=0; strlen(checklist2[i]) > 0; ++i ) {
    unsigned int j;
    char *insert;
    exists = true;
    for ( j=checklist2[i][1]; exists; ++j ) {
      sprintf(drive, "/dev/%s", &checklist2[i][3]);
      insert = strchr(drive, '?');
      if ( insert != NULL ) {
	*insert = j;
      }
      if ( (exists=is_cdrom_linux(drive, NULL)) > 0 ) {
	cdio_add_device_list(&drives, drive, &num_drives);
      }
    }
  }
  cdio_add_device_list(&drives, NULL, &num_drives);
  return drives;
#endif /*HAVE_LINUX_CDROM*/
}

/*!
  Return a string containing the default CD device.
 */
char *
cdio_get_default_device_linux(void)
{
#ifndef HAVE_LINUX_CDROM
  return NULL;
  
#else
  unsigned int i;
  char drive[40];
  bool exists;
  char *ret_drive;

  /* Scan the system for CD-ROM drives.
  */
  for ( i=0; strlen(checklist1[i]) > 0; ++i ) {
    sprintf(drive, "/dev/%s", checklist1[i]);
    if ( (exists=is_cdrom_linux(drive, NULL)) > 0 ) {
      return strdup(drive);
    }
  }

  /* Now check the currently mounted CD drives */
  if (NULL != (ret_drive = check_mounts_linux("/etc/mtab")))
    return ret_drive;
  
  /* Finally check possible mountable drives in /etc/fstab */
  if (NULL != (ret_drive = check_mounts_linux("/etc/fstab")))
    return ret_drive;

  /* Scan the system for CD-ROM drives.
     Not always 100% reliable, so use the USE_MNTENT code above first.
  */
  for ( i=0; strlen(checklist2[i]) > 0; ++i ) {
    unsigned int j;
    char *insert;
    exists = true;
    for ( j=checklist2[i][1]; exists; ++j ) {
      sprintf(drive, "/dev/%s", &checklist2[i][3]);
      insert = strchr(drive, '?');
      if ( insert != NULL ) {
	*insert = j;
      }
      if ( (exists=is_cdrom_linux(drive, NULL)) > 0 ) {
	return(strdup(drive));
      }
    }
  }
  return NULL;
#endif /*HAVE_LINUX_CDROM*/
}
/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_linux (const char *psz_source_name)
{
  return cdio_open_am_linux(psz_source_name, NULL);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_am_linux (const char *psz_orig_source, const char *access_mode)
{

#ifdef HAVE_LINUX_CDROM
  CdIo_t *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs_t _funcs = {
    .eject_media           = eject_media_linux,
    .free                  = cdio_generic_free,
    .get_arg               = get_arg_linux,
    .get_blocksize         = get_blocksize_mmc,
    .get_cdtext            = get_cdtext_generic,
    .get_default_device    = cdio_get_default_device_linux,
    .get_devices           = cdio_get_devices_linux,
    .get_disc_last_lsn     = get_disc_last_lsn_linux,
    .get_discmode          = get_discmode_linux,
#if USE_LINUX_CAP
    .get_drive_cap         = get_drive_cap_linux,
#else
    .get_drive_cap         = get_drive_cap_mmc,
#endif
    .get_first_track_num   = get_first_track_num_generic,
    .get_hwinfo            = NULL,
    .get_mcn               = get_mcn_linux,
    .get_num_tracks        = get_num_tracks_generic,
    .get_track_channels    = get_track_channels_generic,
    .get_track_copy_permit = get_track_copy_permit_generic,
    .get_track_format      = get_track_format_linux,
    .get_track_green       = get_track_green_linux,
    .get_track_lba         = NULL, /* This could be implemented if need be. */
    .get_track_preemphasis = get_track_preemphasis_generic,
    .get_track_msf         = get_track_msf_linux,
    .lseek                 = cdio_generic_lseek,
    .read                  = cdio_generic_read,
    .read_audio_sectors    = _read_audio_sectors_linux,
    .read_mode1_sector     = _read_mode1_sector_linux,
    .read_mode1_sectors    = _read_mode1_sectors_linux,
    .read_mode2_sector     = _read_mode2_sector_linux,
    .read_mode2_sectors    = _read_mode2_sectors_linux,
    .read_toc              = read_toc_linux,
    .run_scsi_mmc_cmd      = run_scsi_cmd_linux,
    .set_arg               = set_arg_linux,
    .set_blocksize         = set_blocksize_mmc,
    .set_speed             = set_speed_linux,
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));

  _data->access_mode    = str_to_access_mode_linux(access_mode);
  _data->gen.init       = false;
  _data->gen.toc_init   = false;
  _data->gen.fd         = -1;
  _data->gen.b_cdtext_init  = false;
  _data->gen.b_cdtext_error = false;

  if (NULL == psz_orig_source) {
    psz_source=cdio_get_default_device_linux();
    if (NULL == psz_source) return NULL;
    set_arg_linux(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      set_arg_linux(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
#if 0
      cdio_info ("source %s is not a device", psz_orig_source);
#endif
      free(_data);
      return NULL;
    }
  }

  ret = cdio_new ((void *)_data, &_funcs);
  if (ret == NULL) return NULL;

  ret->driver_id = DRIVER_LINUX;

  if (cdio_generic_init(_data)) {
    return ret;
  } else {
    cdio_generic_free (_data);
    return NULL;
  }
  
#else 
  return NULL;
#endif /* HAVE_LINUX_CDROM */

}

bool
cdio_have_linux (void)
{
#ifdef HAVE_LINUX_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_LINUX_CDROM */
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
