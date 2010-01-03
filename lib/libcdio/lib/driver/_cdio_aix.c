/*
    $Id: _cdio_aix.c,v 1.8 2005/01/24 17:36:56 rocky Exp $

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include <cdio/scsi_mmc.h>
#include "cdio_assert.h"
#include "cdio_private.h"

#define DEFAULT_CDIO_DEVICE "/dev/rcd0"

#ifdef HAVE_AIX_CDROM

static const char _rcsid[] = "$Id: _cdio_aix.c,v 1.8 2005/01/24 17:36:56 rocky Exp $";

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/scsi.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/devinfo.h>
#include <sys/ide.h>
#include <sys/idecdrom.h>
#include <sys/scdisk.h>
#include "cdtext_private.h"

typedef struct _TRACK_DATA {
    uchar Format;
    uchar Control : 4;
    uchar Adr : 4;
    uchar TrackNumber;
    uchar Reserved1;
    uchar Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {
    uchar Length[2];
    uchar FirstTrack;
    uchar LastTrack;
    TRACK_DATA TrackData[CDIO_CD_MAX_TRACKS+1];
} CDROM_TOC, *PCDROM_TOC;


typedef struct _TRACK_DATA_FULL {
    uchar SessionNumber;
    uchar Control : 4;
    uchar Adr : 4;
    uchar TNO;
    uchar POINT;  /* Tracknumber (of session?) or lead-out/in (0xA0, 0xA1, 0xA2)  */ 
    uchar Min;  /* Only valid if disctype is CDDA ? */
    uchar Sec;  /* Only valid if disctype is CDDA ? */
    uchar Frame;  /* Only valid if disctype is CDDA ? */
    uchar Zero;  /* Always zero */
    uchar PMIN;  /* start min, if POINT is a track; if lead-out/in 0xA0: First Track */
    uchar PSEC;
    uchar PFRAME;
} TRACK_DATA_FULL, *PTRACK_DATA_FULL;

typedef struct _CDROM_TOC_FULL {
    uchar Length[2];
    uchar FirstSession;
    uchar LastSession;
    TRACK_DATA_FULL TrackData[CDIO_CD_MAX_TRACKS+3];
} CDROM_TOC_FULL, *PCDROM_TOC_FULL;


/* reader */

typedef  enum {
    _AM_NONE,
    _AM_CTRL_SCSI
} access_mode_t;


typedef struct {
  lsn_t          start_lsn;
  uchar          Control : 4;
  uchar          Format;
  cdtext_t       cdtext;	         /* CD-TEXT */
} track_info_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  
  access_mode_t access_mode;

  /* Some of the more OS specific things. */
  /* Entry info for each track, add 1 for leadout. */
  track_info_t tocent[CDIO_CD_MAX_TRACKS+1]; 

} _img_private_t;

static track_format_t get_track_format_aix(void *p_user_data, 
					       track_t i_track);

static access_mode_t 
str_to_access_mode_aix(const char *psz_access_mode) 
{
  const access_mode_t default_access_mode = _AM_CTRL_SCSI;

  if (NULL==psz_access_mode) return default_access_mode;
  
  if (!strcmp(psz_access_mode, "SCSI"))
    return _AM_CTRL_SCSI;
  else {
    cdio_warn ("unknown access type: %s. Default SCSI used.", 
	       psz_access_mode);
    return default_access_mode;
  }
}


/*!
  Initialize CD device.
 */
static bool
init_aix (_img_private_t *p_env)
{

  if (p_env->gen.init) {
    cdio_warn ("init called more than once");
    return false;
  }
  
  p_env->gen.fd = openx (p_env->gen.source_name, O_RDONLY, NULL, 
			 SC_DIAGNOSTIC);

  /*p_env->gen.fd = openx (p_env->gen.source_name, O_RDONLY, NULL, 
    IDE_SINGLE);*/

  if (p_env->gen.fd < 0)
    {
      cdio_warn ("open (%s): %s", p_env->gen.source_name, strerror (errno));
      return false;
    }

  p_env->gen.init = true;
  p_env->gen.toc_init = false;
  p_env->gen.b_cdtext_init  = false;
  p_env->gen.b_cdtext_error = false;
  p_env->gen.i_joliet_level = 0;  /* Assume no Joliet extensions initally */
  p_env->access_mode = _AM_CTRL_SCSI;    

  return true;
}

/*!
  Run a SCSI MMC command. 
 
  p_user_data   internal CD structure.
  i_timeout_ms   time in milliseconds we will wait for the command
                to complete. 
  i_cdb	        Size of p_cdb
  p_cdb	        CDB bytes. 
  e_direction	direction the transfer is to go.
  i_buf	        Size of buffer
  p_buf	        Buffer for data, both sending and receiving
 */
static driver_return_code_t
run_scsi_cmd_aix( void *p_user_data, unsigned int i_timeout_ms,
                  unsigned int i_cdb, const scsi_mmc_cdb_t *p_cdb, 
                  scsi_mmc_direction_t e_direction, 
                  unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  struct sc_passthru cgc;
  int i_rc;

  memset (&cgc, 0, sizeof (cgc));
  memcpy(cgc.scsi_cdb,  p_cdb, sizeof(scsi_mmc_cdb_t));

#ifdef AIX_DISABLE_ASYNC
	/* This enables synchronous negotiation mode.  Some CD-ROM drives
	 * don't handle this well.
	 */
  cgc.flags = 0; 
#else
  cgc.flags = SC_ASYNC; 
#endif

  if (0 != i_buf) 
    cgc.flags     |= SCSI_MMC_DATA_READ == e_direction ? B_READ : B_WRITE;

  cgc.timeout_value = msecs2secs(i_timeout_ms);
  cgc.buffer        = p_buf;   
  cgc.data_length   = i_buf;
  cgc.command_length= i_cdb;

  i_rc = ioctl(p_env->gen.fd, DK_PASSTHRU, &cgc);
  if (-1 == i_rc) {
    cdio_warn("DKIOCMD error: %s", strerror(errno));
  }
  return i_rc;
}

/*!
   Reads audio sectors from CD device into data starting from lsn.
   Returns 0 if no error. 

   May have to check size of nblocks. There may be a limit that
   can be read in one go, e.g. 25 blocks.
*/

static driver_return_code_t
_read_audio_sectors_aix (void *p_user_data, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };

#ifdef FINISHED
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;
  struct cdrom_cdda cdda;

  _img_private_t *env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0   = from_bcd8(_msf.m);
  msf->cdmsf_sec0   = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);
  
  if (env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");
  
  if (env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (env->gen.ioctls_debugged < 75 
      || (env->gen.ioctls_debugged < (30 * 75)  
	  && env->gen.ioctls_debugged % 75 == 0)
      || env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %d", lsn);
  
  env->gen.ioctls_debugged++;
  
  cdda.cdda_addr   = lsn;
  cdda.cdda_length = nblocks;
  cdda.cdda_data   = (caddr_t) data;
  if (ioctl (env->gen.fd, CDROMCDDA, &cdda) == -1) {
    perror ("ioctl(..,CDROMCDDA,..)");
	return 1;
	/* exit (EXIT_FAILURE); */
  }
#endif
  memcpy (data, buf, CDIO_CD_FRAMESIZE_RAW);
  
  return 0;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sector_aix (void *env, void *data, lsn_t lsn, 
			    bool b_form2)
{

#if FIXED
  do something here. 
#else
  return cdio_generic_read_form1_sector(env, data, lsn);
#endif
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sectors_aix (void *p_user_data, void *p_data, lsn_t lsn, 
			     bool b_form2, unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode1_sector_aix (p_env, 
					    ((char *)p_data) + (blocksize * i),
					       lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sector_aix (void *p_user_data, void *p_data, lsn_t lsn, 
			    bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int offset = 0;
#ifdef FINISHED
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;
  struct cdrom_cdxa cd_read;

  _img_private_t *p_env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);
  
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
  
  /* Using CDROMXA ioctl will actually use the same uscsi command
   * as ATAPI, except we don't need to be root
   */      
  offset = CDIO_CD_XA_SYNC_HEADER;
  cd_read.cdxa_addr = lsn;
  cd_read.cdxa_data = buf;
  cd_read.cdxa_length = 1;
  cd_read.cdxa_format = CDROM_XA_SECTOR_DATA;

  if (ioctl (p_env->gen.fd, CDROMCDXA, &cd_read) == -1) {
    perror ("ioctl(..,CDROMCDXA,..)");
    return 1;
    /* exit (EXIT_FAILURE); */
  }
#endif
  
  if (b_form2)
    memcpy (p_data, buf + (offset-CDIO_CD_SUBHEADER_SIZE), M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)p_data), buf + offset, CDIO_CD_FRAMESIZE);
  
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sectors_aix (void *p_user_data, void *data, lsn_t lsn, 
			     bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = p_user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode2_sector_aix (env, 
					    ((char *)data) + (blocksize * i),
					       lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}


/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static lsn_t
get_disc_last_lsn_aix (void *p_user_data)
{
  uint32_t i_size=0;
#ifdef FINISHED
  _img_private_t *env = p_user_data;

  struct cdrom_tocentry tocent;

  tocent.cdte_track  = CDIO_CDROM_LEADOUT_TRACK;
  tocent.cdte_format = CDIO_CDROM_LBA;
  if (ioctl (env->gen.fd, CDROMREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      exit (EXIT_FAILURE);
    }

  i_size = tocent.cdte_addr.lba;
#endif

  return i_size;
}

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" and "access-mode" are valid keys.
  "source" sets the source device in I/O operations 
  "access-mode" sets the the method of CD access 

  0 is returned if no error was found, and nonzero if there as an error.
*/
static driver_return_code_t
_set_arg_aix (void *p_user_data, const char key[], const char value[])
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
      p_env->access_mode = str_to_access_mode_aix(key);
    }
  else return DRIVER_OP_ERROR;

  return DRIVER_OP_SUCCESS;
}

/*
 * aixioc_send
 	Issue ioctl command.
 
  Args:
    p_env - environment
    cmd - ioctl command
    arg - ioctl argument
    b_print_err - whether an error message is to be displayed if the
    ioctl fails

  Return:
    true/false - ioctl successful
*/
static bool
aixioc_send(_img_private_t *p_env, int cmd, void *arg, bool b_print_err)
{
  struct cd_audio_cmd	*ac;
  
  if (p_env->gen.fd < 0)
    return false;
  
  if (cmd == DKAUDIO) {
    ac = (struct cd_audio_cmd *) arg;
    ac->status = 0;	/* Nuke status for audio cmds */
  }
  
  if (ioctl(p_env->gen.fd, cmd, arg) < 0) {
    if (b_print_err) {
      cdio_warn("errno=%d (%s)",  errno, strerror(errno));
    }
    return false;
  }
  return true;
}

/*!  
  Read and cache the CD's Track Table of Contents and track info.
  via a SCSI MMC READ_TOC (FULTOC).  Return true if successful or
  false if an error.
*/
static bool
read_toc_ioctl_aix (void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  struct cd_audio_cmd	cmdbuf;
  int i;

  cmdbuf.msf_flag = false;
  cmdbuf.audio_cmds = CD_TRK_INFO_AUDIO;
  if (!aixioc_send(p_env, IDE_CDAUDIO, (void *) &cmdbuf, true))
    return false;

  p_env->gen.i_first_track = cmdbuf.indexing.track_index.first_track;
  p_env->gen.i_tracks      = ( cmdbuf.indexing.track_index.last_track
			       - p_env->gen.i_first_track ) + 1;
  
  /* Do it again to get the last MSF data */
  cmdbuf.msf_flag = true;
  if (!aixioc_send(p_env, IDE_CDAUDIO, (void *) &cmdbuf, true))
    return false;      
  
  cmdbuf.audio_cmds = CD_GET_TRK_MSF;
  
  for (i = 0; i <= p_env->gen.i_tracks; i++) {
    int i_track = i + p_env->gen.i_first_track;
    
    /* Get the track info */
    cmdbuf.indexing.track_msf.track = i_track;
    if (!aixioc_send(p_env, IDE_CDAUDIO, (void *) &cmdbuf, TRUE)) 
      return false;
    
    p_env->tocent[ i_track ].start_lsn = 
      cdio_msf3_to_lba(
		       cmdbuf.indexing.track_msf.mins,
		       cmdbuf.indexing.track_msf.secs,
		       cmdbuf.indexing.track_msf.frames );
  }
  
  return true;
}

/*!  
  Read and cache the CD's Track Table of Contents and track info.
  via a SCSI MMC READ_TOC (FULTOC).  Return true if successful or
  false if an error.
*/
static bool
read_toc_aix (void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  scsi_mmc_cdb_t  cdb = {{0, }};
  CDROM_TOC_FULL  cdrom_toc_full;
  int             i_status, i, i_seen_flag;
  int             i_track_format = 0;

  /* Operation code */
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC);

  cdb.field[1] = 0x00;

  /* Format */
  cdb.field[2] = CDIO_MMC_READTOC_FMT_FULTOC;

  memset(&cdrom_toc_full, 0, sizeof(cdrom_toc_full));

  /* Setup to read header, to get length of data */
  CDIO_MMC_SET_READ_LENGTH16(cdb.field, sizeof(cdrom_toc_full));

  i_status = run_scsi_cmd_aix (p_env, 1000*60*3,
			       scsi_mmc_get_cmd_len(cdb.field[0]), 
			       &cdb, SCSI_MMC_DATA_READ, 
			       sizeof(cdrom_toc_full), &cdrom_toc_full);

  if ( 0 != i_status ) {
    cdio_debug ("SCSI MMC READ_TOC failed\n");  
    return read_toc_ioctl_aix(p_user_data);
  } 
    
  i_seen_flag=0;
  for( i = 0 ; i <= CDIO_CD_MAX_TRACKS+3; i++ ) {
    
    if ( 0xA0 == cdrom_toc_full.TrackData[i].POINT ) { 
      /* First track number */
      p_env->gen.i_first_track = cdrom_toc_full.TrackData[i].PMIN;
      i_track_format = cdrom_toc_full.TrackData[i].PSEC;
      i_seen_flag|=0x01;
    }
    
    if ( 0xA1 == cdrom_toc_full.TrackData[i].POINT ) { 
      /* Last track number */
      p_env->gen.i_tracks = 
	cdrom_toc_full.TrackData[i].PMIN - p_env->gen.i_first_track + 1;
      i_seen_flag|=0x02;
    }
    
    if ( 0xA2 == cdrom_toc_full.TrackData[i].POINT ) { 
      /* Start position of the lead out */
      p_env->tocent[ p_env->gen.i_tracks ].start_lsn = 
	cdio_msf3_to_lba(
			 cdrom_toc_full.TrackData[i].PMIN,
			 cdrom_toc_full.TrackData[i].PSEC,
			 cdrom_toc_full.TrackData[i].PFRAME );
      p_env->tocent[ p_env->gen.i_tracks ].Control 
	= cdrom_toc_full.TrackData[i].Control;
      p_env->tocent[ p_env->gen.i_tracks ].Format  = i_track_format;
      i_seen_flag|=0x04;
    }
    
    if (cdrom_toc_full.TrackData[i].POINT > 0 
	&& cdrom_toc_full.TrackData[i].POINT <= p_env->gen.i_tracks) {
      p_env->tocent[ cdrom_toc_full.TrackData[i].POINT - 1 ].start_lsn = 
	cdio_msf3_to_lba(
			 cdrom_toc_full.TrackData[i].PMIN,
			 cdrom_toc_full.TrackData[i].PSEC,
			 cdrom_toc_full.TrackData[i].PFRAME );
      p_env->tocent[ cdrom_toc_full.TrackData[i].POINT - 1 ].Control = 
	cdrom_toc_full.TrackData[i].Control;
      p_env->tocent[ cdrom_toc_full.TrackData[i].POINT - 1 ].Format  = 
	i_track_format;
      
      cdio_debug("p_sectors: %i, %lu", i, 
		 (unsigned long int) (p_env->tocent[i].start_lsn));
      
      if (cdrom_toc_full.TrackData[i].POINT == p_env->gen.i_tracks)
	i_seen_flag|=0x08;
    }
    
    if ( 0x0F == i_seen_flag ) break;
  }
  if ( 0x0F == i_seen_flag ) {
    p_env->gen.toc_init = true; 
    return true;
  }
  return false;
}

/*!
  Eject media in CD drive. If successful, as a side effect we 
  also free obj.
 */
static driver_return_code_t
eject_media_aix (void *p_user_data) {

  _img_private_t *p_env = p_user_data;
  driver_return_code_t ret=DRIVER_OP_SUCCESS;
  int i_status;

  if (p_env->gen.fd <= -1) return DRIVER_OP_UNINIT;
  i_status = ioctl(p_env->gen.fd, DKEJECT);
  if ( i_status != 0) {
    cdio_generic_free((void *) p_env);
    cdio_warn ("DKEJECT failed: %s", strerror(errno));
    ret = DRIVER_OP_ERROR;
  }
  close(p_env->gen.fd);
  p_env->gen.fd = -1;
  return ret;
}

#if 0
static void *
_cdio_malloc_and_zero(size_t size) {
  void *ptr;

  if( !size ) size++;
    
  if((ptr = malloc(size)) == NULL) {
    cdio_warn("malloc() failed: %s", strerror(errno));
    return NULL;
  }

  memset(ptr, 0, size);
  return ptr;
}
#endif

/*!
  Return the value associated with the key "arg".
*/
static const char *
get_arg_aix (void *p_user_data, const char key[])
{
  _img_private_t *p_env = p_user_data;

  if (!strcmp (key, "source")) {
    return p_env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (p_env->access_mode) {
    case _AM_CTRL_SCSI:
      return "SCSI";
    case _AM_NONE:
      return "no access method";
    }
  } 
  return NULL;
}

/*!
  Return a string containing the default CD device if none is specified.
 */
char *
cdio_get_default_device_aix(void)
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*! 
  Get disc type associated with cd object.
*/

static discmode_t
get_discmode_aix (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;
  struct mode_form_op media;
  int ret;

  /* Get the media info */
  media.action= CD_GET_MODE;

  if((ret = ioctl(p_env->gen.fd, DK_CD_MODE, &media)) != 0) {
     cdio_warn ("DK_CD_MODE failed: %s", strerror(errno));
	 return CDIO_DISC_MODE_NO_INFO;
  }
  switch(media.cd_mode_form) {
  case CD_DA:
    return CDIO_DISC_MODE_CD_DA;
  case DVD_ROM:	
    return CDIO_DISC_MODE_DVD_ROM;
  case DVD_RAM:
    return CDIO_DISC_MODE_DVD_RAM;
  case DVD_RW:
    return CDIO_DISC_MODE_DVD_RW;
  default: /* no valid match */
    return CDIO_DISC_MODE_NO_INFO; 
  }
}

/*!  
  Get format of track. 
*/
static track_format_t
get_track_format_aix(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if ( !p_env ) return TRACK_FORMAT_ERROR;
  if (!p_env->gen.init) init_aix(p_env);
  if (!p_env->gen.toc_init) read_toc_aix (p_user_data) ;

  if ( (i_track > p_env->gen.i_tracks+p_env->gen.i_first_track) 
       || i_track < p_env->gen.i_first_track)
    return TRACK_FORMAT_ERROR;

  i_track -= p_env->gen.i_first_track;

#ifdef FINISHED
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
#else 
  return TRACK_FORMAT_ERROR;
#endif
  
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8

  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_cdio_get_track_green(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if ( !p_env ) return false;
  if (!p_env->gen.init) init_aix(p_env);
  if (!p_env->gen.toc_init) read_toc_aix (p_env) ;

  if (i_track >= p_env->gen.i_tracks+p_env->gen.i_first_track 
      || i_track < p_env->gen.i_first_track)
    return false;

  i_track -= p_env->gen.i_first_track;

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cd-info for a while.
   */

#ifdef FINISHED
  return ((p_env->tocent[i_track].cdte_ctrl & 2) != 0);
#else 
  return false;
#endif
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers usually start at something 
  greater than 0, usually 1.

  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no entry.
*/
static bool
_cdio_get_track_msf(void *p_user_data, track_t i_track, msf_t *msf)
{
  _img_private_t *p_env = p_user_data;

  if (NULL == msf) return false;

  if (!p_env->gen.init) init_aix(p_env);
  if (!p_env->gen.toc_init) read_toc_aix (p_env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) 
    i_track = p_env->gen.i_tracks + p_env->gen.i_first_track;

#ifdef FINISHED
  if (i_track > (p_env->gen.i_tracks+p_env->gen.i_first_track) 
      || i_track < p_env->gen.i_first_track) {
    return false;
  } else {
    struct cdrom_tocentry *msf0 = &p_env->tocent[i_track-1];
    msf->m = to_bcd8(msf0->cdte_addr.msf.minute);
    msf->s = to_bcd8(msf0->cdte_addr.msf.second);
    msf->f = to_bcd8(msf0->cdte_addr.msf.frame);
    return true;
  }
#else 
    return FALSE;
#endif
}

#else 
/*!
  Return a string containing the default VCD device if none is specified.
 */
char *
cdio_get_default_device_aix(void)
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

#endif /* HAVE_AIX_CDROM */

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_aix (void)
{
#ifndef HAVE_AIX_CDROM
  return NULL;
#else
  struct stat st;
  char **drives = NULL;
  unsigned int i_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;

  globbuf.gl_offs = 0;
  glob("/dev/rcd?", 0, NULL, &globbuf);
  for (i=0; i<globbuf.gl_pathc; i++) {
    if(stat(globbuf.gl_pathv[i], &st) < 0)
      continue;

    /* Check if this is a block device; if so it's probably a AIX media */
    if(S_ISCHR(st.st_mode)) {
      cdio_add_device_list(&drives, globbuf.gl_pathv[i], &i_files);
    }
  }
  globfree(&globbuf);
#else
  char psz_device[30];
  for (i=0; i<9; i++) {
    sprintf(psz_device, "/dev/cd%d", i);
    if(stat(psz_device, &st) < 0)
      continue;

    /* Check if this is a block device; if so it's probably a AIX media */
    if(S_ISCHR(st.st_mode)) {
      cdio_add_device_list(&drives, psz_device, &i_files);
  }
#endif /*HAVE_GLOB_H*/
  cdio_add_device_list(&drives, NULL, &i_files);
  return drives;
#endif /*HAVE_AIX_CDROM*/
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_aix (const char *psz_source_name)
{
  return cdio_open_am_aix(psz_source_name, NULL);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_am_aix (const char *psz_orig_source, const char *access_mode)
{

#ifdef HAVE_AIX_CDROM
  CdIo_t *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs_t _funcs;

  _funcs.eject_media        = eject_media_aix;
  _funcs.free               = cdio_generic_free;
  _funcs.get_arg            = get_arg_aix;
  _funcs.get_cdtext         = get_cdtext_generic;
  _funcs.get_default_device = cdio_get_default_device_aix;
  _funcs.get_devices        = cdio_get_devices_aix;
  _funcs.get_disc_last_lsn  = get_disc_last_lsn_aix;
  _funcs.get_discmode       = get_discmode_aix;
  _funcs.get_drive_cap      = get_drive_cap_mmc;
  _funcs.get_first_track_num= get_first_track_num_generic;
  _funcs.get_hwinfo         = NULL;
  _funcs.get_mcn            = get_mcn_mmc,
  _funcs.get_num_tracks     = get_num_tracks_generic;
  _funcs.get_track_format   = get_track_format_aix;
  _funcs.get_track_green    = _cdio_get_track_green;
  _funcs.get_track_lba      = NULL; /* This could be implemented if need be. */
  _funcs.get_track_msf      = _cdio_get_track_msf;
  _funcs.lseek              = cdio_generic_lseek;
  _funcs.read               = cdio_generic_read;
  _funcs.read_audio_sectors = _read_audio_sectors_aix;
  _funcs.read_mode1_sector  = _read_mode1_sector_aix;
  _funcs.read_mode1_sectors = _read_mode1_sectors_aix;
  _funcs.read_mode2_sector  = _read_mode2_sector_aix;
  _funcs.read_mode2_sectors = _read_mode2_sectors_aix;
  _funcs.read_toc           = read_toc_aix;
  _funcs.run_scsi_mmc_cmd   = run_scsi_cmd_aix;
  _funcs.set_arg            = _set_arg_aix;

  _data                 = _cdio_malloc (sizeof (_img_private_t));

  _data->access_mode    = _AM_CTRL_SCSI;
  _data->gen.init       = false;
  _data->gen.fd         = -1;
  _data->gen.toc_init   = false;
  _data->gen.b_cdtext_init  = false;
  _data->gen.b_cdtext_error = false;

  if (NULL == psz_orig_source) {
    psz_source = cdio_get_default_device_aix();
    if (NULL == psz_source) return NULL;
    _set_arg_aix(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      _set_arg_aix(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
#if 0
      cdio_info ("source %s is not a device", psz_orig_source);
#endif
      return NULL;
    }
  }

  ret = cdio_new ( (void *) _data, &_funcs );
  ret->driver_id = DRIVER_AIX;

  if (ret == NULL) return NULL;

  if (init_aix(_data))
    return ret;
  else {
    cdio_generic_free (_data);
    return NULL;
  }

#else 
  return NULL;
#endif /* HAVE_AIX_CDROM */

}

bool
cdio_have_aix (void)
{
#ifdef HAVE_AIX_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_AIX_CDROM */
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
