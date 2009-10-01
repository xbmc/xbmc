/*
    $Id: bsdi.c,v 1.14 2006/02/18 19:36:07 rocky Exp $

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

/* This file contains BSDI-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: bsdi.c,v 1.14 2006/02/18 19:36:07 rocky Exp $";

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"

#define DEFAULT_CDIO_DEVICE "/dev/rsr0c"
#include <string.h>

#ifdef HAVE_BSDI_CDROM

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/*#define USE_ETC_FSTAB*/
#ifdef USE_ETC_FSTAB
#include <fstab.h>
#endif

#include <dvd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include </sys/dev/scsi/scsi.h>
#include </sys/dev/scsi/scsi_ioctl.h>
#include "cdtext_private.h"

#include <cdrom.h>
/* This function is in the man page but seems to be missing from the
   above include (on Steve Schultz BSDI box). */
extern int cdpause(struct cdinfo *cdinfo, int pause_resume);

typedef  enum {
  _AM_NONE,
  _AM_IOCTL,
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  access_mode_t access_mode;

  /* Some of the more OS specific things. */
  /* Track information */
  struct cdrom_tochdr    tochdr;
  struct cdrom_tocentry  tocent[CDIO_CD_MAX_TRACKS+1]; 
  struct cdinfo *        p_cdinfo;

} _img_private_t;

/* Define the Cdrom Generic Command structure */
typedef struct  cgc
{
  mmc_cdb_t cdb;
  u_char  *buf;
  int     buflen;
  int     rw;
  unsigned int timeout;
  scsi_user_sense_t *sus;
} cgc_t;


static bool get_track_msf_bsdi(void *p_user_data, track_t i_track, msf_t *msf);
static lsn_t  get_disc_last_lsn_bsdi (void *p_user_data);

/* 
   This code adapted from Steven M. Schultz's libdvd
*/
static driver_return_code_t
run_mmc_cmd_bsdi(void *p_user_data, unsigned int i_timeout_ms,
		 unsigned int i_cdb, const mmc_cdb_t *p_cdb, 
		 cdio_mmc_direction_t e_direction, 
		 unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  int     i_status, i_asc;
  struct  scsi_user_cdb suc;
  struct  scsi_sense   *sp;
  
 again:
  suc.suc_flags = SCSI_MMC_DATA_READ == e_direction ? 
    SUC_READ : SUC_WRITE;
  suc.suc_cdblen = i_cdb;
  memcpy(suc.suc_cdb, p_cdb, i_cdb);
  suc.suc_data = p_buf;
  suc.suc_datalen = i_buf;
  suc.suc_timeout = msecs2secs(i_timeout_ms);
  if      (ioctl(p_env->gen.fd, SCSIRAWCDB, &suc) == -1)
    return(errno);
  i_status = suc.suc_sus.sus_status;

#if 0  
  /*
   * If the device returns a scsi sense error and debugging is enabled print
   * some hopefully useful information on stderr.
   */
  if      (i_status && debug)
    {
      unsigned char   *cp;
      int i;
      cp = suc.suc_sus.sus_sense;
      fprintf(stderr,"i_status = %x cdb =",
	      i_status);
      for     (i = 0; i < cdblen; i++)
	fprintf(stderr, " %x", cgc->cdb[i]);
      fprintf(stderr, "\nsense =");
      for     (i = 0; i < 16; i++)
	fprintf(stderr, " %x", cp[i]);
      fprintf(stderr, "\n");
    }
#endif

  /*
   * HACK!  Some drives return a silly "medium changed" on the first
   * command AND a non-zero i_status which gets turned into a fatal
   * (EIO) error even though the operation was a success.  Retrying
   * the operation clears the media changed status and gets the
   * answer.  */

  sp = (struct scsi_sense *)&suc.suc_sus.sus_sense;
  i_asc = XSENSE_ASC(sp);
  if      (i_status == STS_CHECKCOND && i_asc == 0x28)
    goto again;
#if 0
  if      (cgc->sus)
    memcpy(cgc->sus, &suc.suc_sus, sizeof (struct scsi_user_sense));
#endif

  return(i_status);
}



/* Check a drive to see if it is a CD-ROM 
   Return 1 if a CD-ROM. 0 if it exists but isn't a CD-ROM drive
   and -1 if no device exists .
*/
static bool
cdio_is_cdrom(char *drive, char *mnttype)
{
  bool is_cd=false;
  int cdfd;
  struct cdrom_tochdr    tochdr;
  
  /* If it doesn't exist, return -1 */
  if ( !cdio_is_device_quiet_generic(drive) ) {
    return(false);
  }
  
  /* If it does exist, verify that it's an available CD-ROM */
  cdfd = open(drive, (O_RDONLY|O_EXCL|O_NONBLOCK), 0);

  /* Should we want to test the condition in more detail:
     ENOENT is the error for /dev/xxxxx does not exist;
     ENODEV means there's no drive present. */

  if ( cdfd >= 0 ) {
    if ( ioctl(cdfd, CDROMREADTOCHDR, &tochdr) != -1 ) {
      is_cd = true;
    }
    close(cdfd);
    }
  /* Even if we can't read it, it might be mounted */
  else if ( mnttype && (strcmp(mnttype, "cd9660") == 0) ) {
    is_cd = true;
  }

  return(is_cd);
}

/*!
  Initialize CD device.
 */
static bool
_cdio_init (_img_private_t *p_env)
{
  if (p_env->gen.init) {
    cdio_warn ("init called more than once");
    return false;
  }
  
  p_env->gen.fd = open (p_env->gen.source_name, O_RDONLY|O_NONBLOCK, 0);

  if (p_env->gen.fd < 0)
    {
      cdio_warn ("open (%s): %s", p_env->gen.source_name, strerror (errno));
      return false;
    }

  p_env->gen.init     = true;
  p_env->gen.toc_init = false;
  return true;
}

/*!
  Get the volume of an audio CD.

  @param p_cdio the CD object to be acted upon.
*/
#if 0
static driver_return_code_t
audio_get_volume_bsdi (void *p_user_data,
                        /*out*/ cdio_audio_volume_t *p_volume)
{

  const _img_private_t *p_env = p_user_data;
  return ioctl(p_env->gen.fd, CDROMVOLREAD, p_volume);
}
#endif

/*!
  Pause playing CD through analog output
  
  @param p_cdio the CD object to be acted upon.
*/
static driver_return_code_t
audio_pause_bsdi (void *p_user_data)
{

  _img_private_t *p_env = p_user_data;
  if (!p_env->p_cdinfo) {
    p_env->p_cdinfo = cdopen(p_env->gen.source_name);
    if (!p_env->p_cdinfo) return DRIVER_OP_ERROR;
  }
  
  return cdpause(p_env->p_cdinfo, 0);
}

/*!
  Playing starting at given MSF through analog output
  
  @param p_cdio the CD object to be acted upon.
*/
static driver_return_code_t
audio_play_msf_bsdi (void *p_user_data, msf_t *p_start_msf, msf_t *p_end_msf)
{

  _img_private_t *p_env = p_user_data;
  lsn_t i_start_lsn = cdio_msf_to_lba(p_start_msf);
  lsn_t i_end_lsn   = cdio_msf_to_lba(p_end_msf);

  if (!p_env->p_cdinfo) {
    p_env->p_cdinfo = cdopen(p_env->gen.source_name);
    if (!p_env->p_cdinfo) return DRIVER_OP_ERROR;
  }
  return cdplay(p_env->p_cdinfo, i_start_lsn, i_end_lsn);
}

/*!
  Playing CD through analog output at the desired track and index
  
  @param p_cdio the CD object to be acted upon.
  @param p_track_index location to start/end.
*/
static driver_return_code_t
audio_play_track_index_bsdi (void *p_user_data, 
                              cdio_track_index_t *p_track_index)
{
  _img_private_t *p_env = p_user_data;
  msf_t start_msf;
  msf_t end_msf;
  lsn_t i_start_lsn = cdio_msf_to_lsn(&start_msf);
  lsn_t i_end_lsn   = cdio_msf_to_lsn(&end_msf);

  if (!p_env->p_cdinfo) {
    p_env->p_cdinfo = cdopen(p_env->gen.source_name);
    if (!p_env->p_cdinfo) return DRIVER_OP_ERROR;
  }

  get_track_msf_bsdi(p_user_data, p_track_index->i_start_track, &start_msf);
  get_track_msf_bsdi(p_user_data, p_track_index->i_end_track, &end_msf);
  
  return cdplay(p_env->p_cdinfo, i_start_lsn, i_end_lsn);
}

/*!
  Read Audio Subchannel information
  
  @param p_cdio the CD object to be acted upon.
  
*/
static driver_return_code_t
audio_read_subchannel_bsdi (void *p_user_data, cdio_subchannel_t *p_subchannel)
{
  int   i_rc;
  _img_private_t *p_env = p_user_data;
  struct cdstatus cdstat;

  if (!p_env->p_cdinfo) {
    p_env->p_cdinfo = cdopen(p_env->gen.source_name);
    if (!p_env->p_cdinfo) return DRIVER_OP_ERROR;
  }

  i_rc = cdstatus(p_env->p_cdinfo, &cdstat);
  if (0 == i_rc) {
    msf_t msf;
    p_subchannel->control      = cdstat.control;
    p_subchannel->track        = cdstat.track_num;
    p_subchannel->index        = cdstat.index_num;

    cdio_lba_to_msf(cdstat.abs_frame, &msf);
    p_subchannel->abs_addr.m = cdio_to_bcd8(msf.m);
    p_subchannel->abs_addr.s = cdio_to_bcd8(msf.s);
    p_subchannel->abs_addr.f = cdio_to_bcd8(msf.f);

    cdio_lsn_to_msf(cdstat.rel_frame, &msf);
    p_subchannel->rel_addr.m = cdio_to_bcd8(msf.m);
    p_subchannel->rel_addr.s = cdio_to_bcd8(msf.s);
    p_subchannel->rel_addr.f = cdio_to_bcd8(msf.f);

    switch(cdstat.state) {
    case cdstate_unknown:
      p_subchannel->audio_status = CDIO_MMC_READ_SUB_ST_NO_STATUS;
      break;
    case cdstate_stopped:
      p_subchannel->audio_status = CDIO_MMC_READ_SUB_ST_COMPLETED;
      break;
    case cdstate_playing:
      p_subchannel->audio_status = CDIO_MMC_READ_SUB_ST_PLAY;
      break;
    case cdstate_paused:
      p_subchannel->audio_status = CDIO_MMC_READ_SUB_ST_PAUSED;
      break;
    default:
      p_subchannel->audio_status = CDIO_MMC_READ_SUB_ST_INVALID;
    }
  }
  
  return i_rc;
}

/*!
  Resume playing an audio CD.
  
  @param p_cdio the CD object to be acted upon.
  
*/
static driver_return_code_t
audio_resume_bsdi (void *p_user_data)
{

  _img_private_t *p_env = p_user_data;

  if (!p_env->p_cdinfo) {
    p_env->p_cdinfo = cdopen(p_env->gen.source_name);
    if (!p_env->p_cdinfo) return DRIVER_OP_ERROR;
  }

  return cdpause(p_env->p_cdinfo, 1);
}

/*!  

  Set the volume of an audio CD. We use only the first (port 0) volume
  level.
  
  @param p_cdio the CD object to be acted upon.
  
*/
static driver_return_code_t
audio_set_volume_bsdi (void *p_user_data, 
                       cdio_audio_volume_t *p_volume)
{
  _img_private_t *p_env = p_user_data;

  if (!p_env->p_cdinfo) {
    p_env->p_cdinfo = cdopen(p_env->gen.source_name);
    if (!p_env->p_cdinfo) return DRIVER_OP_ERROR;
  }

  /* Convert volume from 0..255 into 0..100. */
  return cdvolume(p_env->p_cdinfo, (p_volume->level[0]*100+128) / 256);
}

/*!
  Stop playing an audio CD.
  
  @param p_user_data the CD object to be acted upon.
  
*/
static driver_return_code_t
audio_stop_bsdi (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  if (!p_env->p_cdinfo) {
    p_env->p_cdinfo = cdopen(p_env->gen.source_name);
    if (!p_env->p_cdinfo) return DRIVER_OP_ERROR;
  }
  return cdstop(p_env->p_cdinfo);
}

/* Read audio sectors
*/
static driver_return_code_t
_read_audio_sectors_bsdi (void *user_data, void *data, lsn_t lsn,
			  uint32_t i_blocks)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *p_env = user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = cdio_from_bcd8(_msf.m);
  msf->cdmsf_sec0 = cdio_from_bcd8(_msf.s);
  msf->cdmsf_frame0 = cdio_from_bcd8(_msf.f);

  if (p_env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");

  if (p_env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (p_env->gen.ioctls_debugged < 75 
      || (p_env->gen.ioctls_debugged < (30 * 75)  
	  && p_env->gen.ioctls_debugged % 75 == 0)
      || p_env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %2.2d:%2.2d:%2.2d",
	       msf->cdmsf_min0, msf->cdmsf_sec0, msf->cdmsf_frame0);
  
  p_env->gen.ioctls_debugged++;
 
  switch (p_env->access_mode) {
    case _AM_NONE:
      cdio_warn ("no way to read audio");
      return 1;
      break;
      
    case _AM_IOCTL: {
      unsigned int i;
      for (i=0; i < i_blocks; i++) {
	if (ioctl (p_env->gen.fd, CDROMREADRAW, &buf) == -1)  {
	  perror ("ioctl()");
	  return 1;
	  /* exit (EXIT_FAILURE); */
	}
	memcpy (((char *)data) + (CDIO_CD_FRAMESIZE_RAW * i), buf, 
		CDIO_CD_FRAMESIZE_RAW);
      }
      break;
    }
  }

  return DRIVER_OP_SUCCESS;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sector_bsdi (void *user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{

#if FIXED
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  do something here. 
#else
  return cdio_generic_read_form1_sector(user_data, data, lsn);
#endif
}

/*!
   Reads i_blocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sectors_bsdi (void *p_user_data, void *p_data, lsn_t lsn, 
			  bool b_form2, uint32_t i_blocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  int retval;
  uint16_t blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < i_blocks; i++) {
    if ( (retval = _read_mode1_sector_bsdi (p_env, 
					    ((char *)p_data) + (blocksize * i),
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
_read_mode2_sector_bsdi (void *p_user_data, void *p_data, lsn_t lsn, 
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

  if (p_env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");

  if (p_env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (p_env->gen.ioctls_debugged < 75 
      || (p_env->gen.ioctls_debugged < (30 * 75)  
	  && p_env->gen.ioctls_debugged % 75 == 0)
      || p_env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %2.2d:%2.2d:%2.2d",
	       msf->cdmsf_min0, msf->cdmsf_sec0, msf->cdmsf_frame0);
  
  p_env->gen.ioctls_debugged++;
 
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
    }

  if (b_form2)
    memcpy (p_data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)p_data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
  
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads i_blocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sectors_bsdi (void *user_data, void *data, lsn_t lsn, 
			  bool b_form2, uint32_t i_blocks)
{
  _img_private_t *p_env = user_data;
  unsigned int i;
  unsigned int i_blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

    /* For each frame, pick out the data part we need */
  for (i = 0; i < i_blocks; i++) {
    int retval = _read_mode2_sector_bsdi(p_env, 
					 ((char *)data) + 
					 (i_blocksize * i),
					 lsn + i, b_form2);
    if (retval) return retval;
  }
  return DRIVER_OP_SUCCESS;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static lsn_t 
get_disc_last_lsn_bsdi (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  struct cdrom_tocentry tocent;
  uint32_t size;

  tocent.cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  tocent.cdte_format = CDROM_LBA;
  if (ioctl (p_env->gen.fd, CDROMREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      exit (EXIT_FAILURE);
    }

  size = tocent.cdte_addr.lba;

  return size;
}

/*!
  Set the key "arg" to "value" in source device.
*/
static driver_return_code_t
_set_arg_bsdi (void *user_data, const char key[], const char value[])
{
  _img_private_t *p_env = user_data;

  if (!strcmp (key, "source"))
    {
      if (!value) return DRIVER_OP_ERROR;
      free (p_env->gen.source_name);
      p_env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      if (!strcmp(value, "IOCTL"))
	p_env->access_mode = _AM_IOCTL;
      else
	cdio_warn ("unknown access type: %s. ignored.", value);
    }
  else return DRIVER_OP_ERROR;
  return DRIVER_OP_SUCCESS;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return false if successful or true if an error.
*/
static bool
read_toc_bsdi (void *p_user_data) 
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
    if (ioctl(p_env->gen.fd, CDROMREADTOCENTRY, p_toc) == -1) {
      cdio_warn("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }

    set_track_flags(&(p_env->gen.track_flags[i]), p_toc->cdte_ctrl);

    /****
    struct cdrom_msf0 *msf= &p_env->tocent[i-1].cdte_addr.msf;
    
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
  struct cdrom_msf0 *msf= &p_env->tocent[p_env->gen.i_tracks].cdte_addr.msf;

  fprintf (stdout, "--- track# %d (msf %2.2x:%2.2x:%2.2x)\n",
	   i, msf->minute, msf->second, msf->frame);
  */

  p_env->gen.toc_init = true;
  return true;
}

/*!
  Eject media in CD drive. If successful, as a side effect we 
  also free obj.
 */
static driver_return_code_t
eject_media_bsdi (void *p_user_data) {

  _img_private_t *p_env = p_user_data;
  int ret=DRIVER_OP_ERROR;
  int fd;

  close(p_env->gen.fd);
  p_env->gen.fd = -1;
  if ((fd = open (p_env->gen.source_name, O_RDONLY|O_NONBLOCK)) > -1) {
    if((ret = ioctl(fd, CDROMEJECT, 0)) != 0) {
      cdio_warn("ioctl CDROMEJECT failed: %s\n", strerror(errno));  
    }
    close(fd);
  }
  return ret;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_bsdi (void *user_data, const char key[])
{
  _img_private_t *p_env = user_data;

  if (!strcmp (key, "source")) {
    return p_env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (p_env->access_mode) {
    case _AM_IOCTL:
      return "ioctl";
    case _AM_NONE:
      return "no access method";
    }
  } 
  return NULL;
}

/*!
  Return the media catalog number MCN.
  Note: string is malloc'd so caller should free() then returned
  string when done with it.
 */
static char *
_get_mcn_bsdi (const void *p_user_data) {

  struct cdrom_mcn mcn;
  const _img_private_t *p_env = p_user_data;
  if (ioctl(p_env->gen.fd, CDROM_GET_MCN, &mcn) != 0)
    return NULL;
  return strdup(mcn.medium_catalog_number);
}

/*!  
  Get format of track. 
*/
static track_format_t
get_track_format_bsdi(void *user_data, track_t i_track) 
{
  _img_private_t *p_env = user_data;
  
  if (!p_env->gen.toc_init) read_toc_bsdi (p_env) ;

  if (i_track > p_env->gen.i_tracks || i_track == 0)
    return TRACK_FORMAT_ERROR;

  i_track -= p_env->gen.i_first_track;

  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
   */
  if (p_env->tocent[i_track].cdte_ctrl & CDROM_DATA_TRACK) {
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
_get_track_green_bsdi(void *user_data, track_t i_track) 
{
  _img_private_t *p_env = user_data;
  
  if (!p_env->gen.toc_init) read_toc_bsdi (p_env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = p_env->gen.i_tracks+1;

  if (i_track > p_env->gen.i_tracks+1 || i_track == 0)
    return false;

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cdinfo for a while.
   */
  return ((p_env->tocent[i_track-1].cdte_ctrl & 2) != 0);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  i_track in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static bool
get_track_msf_bsdi(void *user_data, track_t i_track, msf_t *msf)
{
  _img_private_t *p_env = user_data;

  if (NULL == msf) return false;

  if (!p_env->gen.toc_init) read_toc_bsdi (p_env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = p_env->gen.i_tracks+1;

  if (i_track > p_env->gen.i_tracks+1 || i_track == 0) {
    return false;
  } 

  i_track -= p_env->gen.i_first_track;

  {
    struct cdrom_msf0  *msf0= &p_env->tocent[i_track].cdte_addr.msf;
    msf->m = cdio_to_bcd8(msf0->minute);
    msf->s = cdio_to_bcd8(msf0->second);
    msf->f = cdio_to_bcd8(msf0->frame);
    return true;
  }
}

#endif /* HAVE_BSDI_CDROM */

/*!
  Close tray on CD-ROM.
  
  @param p_user_data the CD object to be acted upon.
  
*/
driver_return_code_t 
close_tray_bsdi (const char *psz_device)
{
#ifdef HAVE_BSDI_CDROM
  int fd = open(psz_device, O_RDONLY | O_NONBLOCK, 0);

  if (fd < 0) return DRIVER_OP_ERROR;
  else {
    int i_rc = cdrom_tray_move(fd, 0);
    close(fd);
    return (i_rc > -1) ? DRIVER_OP_SUCCESS : DRIVER_OP_ERROR;
  }
#else 
  return DRIVER_OP_NO_DRIVER;
#endif /*HAVE_BSDI_CDROM*/
}

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_bsdi (void)
{
#ifndef HAVE_BSDI_CDROM
  return NULL;
#else
  char drive[40];
  char **drives = NULL;
  unsigned int num_drives=0;
  bool exists=true;
  char c;
  
  /* Scan the system for CD-ROM drives.
  */

#ifdef USE_ETC_FSTAB

  struct fstab *fs;
  setfsent();
  
  /* Check what's in /etc/fstab... */
  while ( (fs = getfsent()) )
    {
      if (strncmp(fs->fs_spec, "/dev/sr", 7))
	cdio_add_device_list(&drives, fs->fs_spec, &num_drives);
    }
  
#endif

  /* Scan the system for CD-ROM drives.
     Not always 100% reliable, so use the USE_MNTENT code above first.
  */
  for ( c='0'; exists && c <='9'; c++ ) {
    sprintf(drive, "/dev/rsr%cc", c);
    exists = cdio_is_cdrom(drive, NULL);
    if ( exists ) {
      cdio_add_device_list(&drives, drive, &num_drives);
    }
  }
  cdio_add_device_list(&drives, NULL, &num_drives);
  return drives;
#endif /*HAVE_BSDI_CDROM*/
}

/*!
  Return a string containing the default CD device if none is specified.
 */
char *
cdio_get_default_device_bsdi(void)
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_am_bsdi (const char *psz_source_name, const char *psz_access_mode)
{
  if (psz_access_mode != NULL)
    cdio_warn ("there is only one access mode for bsdi. Arg %s ignored",
	       psz_access_mode);
  return cdio_open_bsdi(psz_source_name);
}


/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_bsdi (const char *psz_orig_source)
{

#ifdef HAVE_BSDI_CDROM
  CdIo_t *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs_t _funcs = {
    .audio_pause           = audio_pause_bsdi,
    .audio_play_msf        = audio_play_msf_bsdi,
    .audio_play_track_index= audio_play_track_index_bsdi,
#if USE_MMC_SUBCHANNEL
    .audio_read_subchannel = audio_read_subchannel_mmc,
#else
    .audio_read_subchannel = audio_read_subchannel_bsdi,
#endif
    .audio_resume          = audio_resume_bsdi,
    .audio_set_volume      = audio_set_volume_bsdi,
    .audio_stop            = audio_stop_bsdi,
    .eject_media           = eject_media_bsdi,
    .free                  = cdio_generic_free,
    .get_arg               = _get_arg_bsdi,
    .get_cdtext            = get_cdtext_generic,
    .get_default_device    = cdio_get_default_device_bsdi,
    .get_devices           = cdio_get_devices_bsdi,
    .get_drive_cap         = get_drive_cap_mmc,
    .get_disc_last_lsn     = get_disc_last_lsn_bsdi,
    .get_discmode          = get_discmode_generic,
    .get_first_track_num   = get_first_track_num_generic,
    .get_hwinfo            = NULL,
    .get_media_changed     = get_media_changed_mmc, 
    .get_mcn               = _get_mcn_bsdi, 
    .get_num_tracks        = get_num_tracks_generic,
    .get_track_channels    = get_track_channels_generic,
    .get_track_copy_permit = get_track_copy_permit_generic,
    .get_track_format      = get_track_format_bsdi,
    .get_track_green       = _get_track_green_bsdi,
    .get_track_lba         = NULL, /* This could be implemented if need be. */
    .get_track_preemphasis = get_track_preemphasis_generic,
    .get_track_msf         = get_track_msf_bsdi,
    .lseek                 = cdio_generic_lseek,
    .read                  = cdio_generic_read,
    .read_audio_sectors    = _read_audio_sectors_bsdi,
    .read_data_sectors     = read_data_sectors_mmc,
    .read_mode1_sector     = _read_mode1_sector_bsdi,
    .read_mode1_sectors    = _read_mode1_sectors_bsdi,
    .read_mode2_sector     = _read_mode2_sector_bsdi,
    .read_mode2_sectors    = _read_mode2_sectors_bsdi,
    .read_toc              = read_toc_bsdi,
    .run_mmc_cmd           = run_mmc_cmd_bsdi,
    .set_arg               = _set_arg_bsdi,
  };

  _data                     = calloc (1, sizeof (_img_private_t));
  _data->access_mode        = _AM_IOCTL;
  _data->gen.init           = false;
  _data->gen.fd             = -1;
  _data->gen.toc_init       = false;
  _data->gen.b_cdtext_init  = false;
  _data->gen.b_cdtext_error = false;

  if (NULL == psz_orig_source) {
    psz_source=cdio_get_default_device_bsdi();
    if (NULL == psz_source) return NULL;
    _set_arg_bsdi(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      _set_arg_bsdi(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
#if 0
      cdio_info ("source %s is not a device", psz_orig_source);
#endif
      free(_data);
      return NULL;
    }
  }

  ret = cdio_new ( (void *) _data, &_funcs);
  if (ret == NULL) return NULL;

  ret->driver_id = DRIVER_BSDI;

  if (_cdio_init(_data))
    return ret;
  else {
    cdio_generic_free (_data);
    return NULL;
  }
  
#else 
  return NULL;
#endif /* HAVE_BSDI_CDROM */

}

bool
cdio_have_bsdi (void)
{
#ifdef HAVE_BSDI_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_BSDI_CDROM */
}
