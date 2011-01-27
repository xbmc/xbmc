/*
    $Id: _cdio_sunos.c,v 1.17 2005/01/24 00:10:46 rocky Exp $

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

#define DEFAULT_CDIO_DEVICE "/vol/dev/aliases/cdrom0"

#ifdef HAVE_SOLARIS_CDROM

static const char _rcsid[] = "$Id: _cdio_sunos.c,v 1.17 2005/01/24 00:10:46 rocky Exp $";

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h> /* CDIOCALLOW etc... */
#else 
#error "You need <sys/cdio.h> to have CDROM support"
#endif

#include <sys/dkio.h>
#include <sys/scsi/generic/commands.h>
#include <sys/scsi/impl/uscsi.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "cdtext_private.h"

/* not defined in dkio.h yet */
#define DK_DVDRW 0x13

/* reader */

typedef  enum {
    _AM_NONE,
    _AM_SUN_CTRL_ATAPI,
    _AM_SUN_CTRL_SCSI
#if FINISHED
    _AM_READ_CD,
    _AM_READ_10
#endif
} access_mode_t;


typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  
  access_mode_t access_mode;

  /* Some of the more OS specific things. */
  /* Entry info for each track, add 1 for leadout. */
  struct cdrom_tocentry  tocent[CDIO_CD_MAX_TRACKS+1]; 

  /* Track information */
  struct cdrom_tochdr    tochdr;
} _img_private_t;

static track_format_t get_track_format_solaris(void *p_user_data, 
					       track_t i_track);

static access_mode_t 
str_to_access_mode_sunos(const char *psz_access_mode) 
{
  const access_mode_t default_access_mode = _AM_SUN_CTRL_SCSI;

  if (NULL==psz_access_mode) return default_access_mode;
  
  if (!strcmp(psz_access_mode, "ATAPI"))
    return _AM_SUN_CTRL_SCSI; /* force ATAPI to be SCSI */
  else if (!strcmp(psz_access_mode, "SCSI"))
    return _AM_SUN_CTRL_SCSI;
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
init_solaris (_img_private_t *p_env)
{

  if (!cdio_generic_init(p_env)) return false;
  
  p_env->access_mode = _AM_SUN_CTRL_SCSI;    

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
run_scsi_cmd_solaris( void *p_user_data, unsigned int i_timeout_ms,
		      unsigned int i_cdb, const scsi_mmc_cdb_t *p_cdb, 
		      scsi_mmc_direction_t e_direction, 
		      unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  struct uscsi_cmd cgc;

  memset (&cgc, 0, sizeof (struct uscsi_cmd));
  cgc.uscsi_cdb = (caddr_t) p_cdb;

  cgc.uscsi_flags = SCSI_MMC_DATA_READ == e_direction ? 
    USCSI_READ : USCSI_WRITE;

  cgc.uscsi_timeout = msecs2secs(i_timeout_ms);
  cgc.uscsi_bufaddr = p_buf;   
  cgc.uscsi_buflen  = i_buf;
  cgc.uscsi_cdblen  = i_cdb;
  
  return ioctl(p_env->gen.fd, USCSICMD, &cgc);
}

/*!
   Reads audio sectors from CD device into data starting from lsn.
   Returns 0 if no error. 

   May have to check size of nblocks. There may be a limit that
   can be read in one go, e.g. 25 blocks.
*/

static int
_read_audio_sectors_solaris (void *p_user_data, void *data, lsn_t lsn, 
			     unsigned int i_blocks)
{
  struct cdrom_msf solaris_msf;
  msf_t _msf;
  struct cdrom_cdda cdda;

  _img_private_t *p_env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  solaris_msf.cdmsf_min0   = cdio_from_bcd8(_msf.m);
  solaris_msf.cdmsf_sec0   = cdio_from_bcd8(_msf.s);
  solaris_msf.cdmsf_frame0 = cdio_from_bcd8(_msf.f);
  
  if (p_env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");
  
  if (p_env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (p_env->gen.ioctls_debugged < 75 
      || (p_env->gen.ioctls_debugged < (30 * 75)  
	  && p_env->gen.ioctls_debugged % 75 == 0)
      || p_env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %d", lsn);
  
  p_env->gen.ioctls_debugged++;

  if (i_blocks > 60) {
    cdio_warn("%s:\n", 
	      "we can't handle reading more than 60 blocks. Reset to 60");
  }

  cdda.cdda_addr    = lsn;
  cdda.cdda_length  = i_blocks;
  cdda.cdda_data    = (caddr_t) data;
  cdda.cdda_subcode = CDROM_DA_NO_SUBCODE;
  
  if (ioctl (p_env->gen.fd, CDROMCDDA, &cdda) == -1) {
    perror ("ioctl(..,CDROMCDDA,..)");
    return DRIVER_OP_ERROR;
    /* exit (EXIT_FAILURE); */
  }
  
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. 
 */
static driver_return_code_t
_read_mode1_sector_solaris (void *p_env, void *data, lsn_t lsn, 
			    bool b_form2)
{

#if FIXED
  do something here. 
#else
  return cdio_generic_read_form1_sector(p_env, data, lsn);
#endif
}

/*!
   Reads i_blocks of mode2 sectors from cd device into data starting
   from lsn.
 */
static driver_return_code_t
_read_mode1_sectors_solaris (void *p_user_data, void *p_data, lsn_t lsn, 
			     bool b_form2, unsigned int i_blocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < i_blocks; i++) {
    if ( (retval = _read_mode1_sector_solaris (p_env, 
					    ((char *)p_data) + (blocksize * i),
					       lsn + i, b_form2)) )
      return retval;
  }
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads a single mode2 sector from cd device into data starting from lsn.
 */
static driver_return_code_t
_read_mode2_sector_solaris (void *p_user_data, void *p_data, lsn_t lsn, 
			    bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct cdrom_msf solaris_msf;
  msf_t _msf;
  int offset = 0;
  struct cdrom_cdxa cd_read;

  _img_private_t *p_env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  solaris_msf.cdmsf_min0   = cdio_from_bcd8(_msf.m);
  solaris_msf.cdmsf_sec0   = cdio_from_bcd8(_msf.s);
  solaris_msf.cdmsf_frame0 = cdio_from_bcd8(_msf.f);
  
  if (p_env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");
  
  if (p_env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (p_env->gen.ioctls_debugged < 75 
      || (p_env->gen.ioctls_debugged < (30 * 75)  
	  && p_env->gen.ioctls_debugged % 75 == 0)
      || p_env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %2.2d:%2.2d:%2.2d",
		solaris_msf.cdmsf_min0, solaris_msf.cdmsf_sec0, 
		solaris_msf.cdmsf_frame0);
  
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
  
  if (b_form2)
    memcpy (p_data, buf + (offset-CDIO_CD_SUBHEADER_SIZE), M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)p_data), buf + offset, CDIO_CD_FRAMESIZE);
  
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads i_blocks of mode2 sectors from cd device into data starting
   from lsn.
 */
static driver_return_code_t
_read_mode2_sectors_solaris (void *p_user_data, void *data, lsn_t lsn, 
			     bool b_form2, unsigned int i_blocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < i_blocks; i++) {
    if ( (retval = _read_mode2_sector_solaris (p_env, 
					    ((char *)data) + (blocksize * i),
					       lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}


/*!
   Return the size of the CD in logical block address (LBA) units.
   @return the lsn. On error return CDIO_INVALID_LSN.
 */
static lsn_t 
get_disc_last_lsn_sunos (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  struct cdrom_tocentry tocent;
  uint32_t size;

  tocent.cdte_track  = CDIO_CDROM_LEADOUT_TRACK;
  tocent.cdte_format = CDIO_CDROM_LBA;
  if (ioctl (p_env->gen.fd, CDROMREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      exit (EXIT_FAILURE);
    }

  size = tocent.cdte_addr.lba;

  return size;
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
_set_arg_solaris (void *p_user_data, const char key[], const char value[])
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
      p_env->access_mode = str_to_access_mode_sunos(key);
    }
  else return DRIVER_OP_ERROR;

  return DRIVER_OP_SUCCESS;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
static bool
read_toc_solaris (void *p_user_data) 
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
  for (i=p_env->gen.i_first_track; i<=p_env->gen.i_tracks; i++) {
    struct cdrom_tocentry *p_toc = 
      &(p_env->tocent[i-p_env->gen.i_first_track]);

    p_toc->cdte_track = i;
    p_toc->cdte_format = CDIO_CDROM_MSF;
    if ( ioctl(p_env->gen.fd, CDROMREADTOCENTRY, p_toc) == -1 ) {
      cdio_warn("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }

    set_track_flags(&(p_env->gen.track_flags[i]), p_toc->cdte_ctrl);
  }

  /* read the lead-out track */
  p_env->tocent[p_env->tochdr.cdth_trk1].cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  p_env->tocent[p_env->tochdr.cdth_trk1].cdte_format = CDIO_CDROM_MSF;

  if (ioctl(p_env->gen.fd, CDROMREADTOCENTRY, 
	    &p_env->tocent[p_env->tochdr.cdth_trk1]) == -1 ) {
    cdio_warn("%s: %s\n", 
	     "error in ioctl CDROMREADTOCENTRY for lead-out",
            strerror(errno));
    return false;
  }

  p_env->gen.toc_init = true;
  return true;
}

/*!
  Eject media in CD drive. If successful, as a side effect we 
  also free obj.
 */
static driver_return_code_t
eject_media_solaris (void *p_user_data) {

  _img_private_t *p_env = p_user_data;
  int ret;

  close(p_env->gen.fd);
  p_env->gen.fd = -1;
  if (p_env->gen.fd > -1) {
    if ((ret = ioctl(p_env->gen.fd, CDROMEJECT)) != 0) {
      cdio_generic_free((void *) p_env);
      cdio_warn ("CDROMEJECT failed: %s\n", strerror(errno));
      return DRIVER_OP_ERROR;
    } else {
      return DRIVER_OP_SUCCESS;
    }
  }
  return DRIVER_OP_ERROR;
}


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

/*!
  Return the value associated with the key "arg".
*/
static const char *
get_arg_solaris (void *p_user_data, const char key[])
{
  _img_private_t *p_env = p_user_data;

  if (!strcmp (key, "source")) {
    return p_env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (p_env->access_mode) {
    case _AM_SUN_CTRL_ATAPI:
      return "ATAPI";
    case _AM_SUN_CTRL_SCSI:
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
cdio_get_default_device_solaris(void)
{
  char *volume_device;
  char *volume_name;
  char *volume_action;
  char *device;
  struct stat stb;

  if ((volume_device = getenv("VOLUME_DEVICE")) != NULL &&
      (volume_name   = getenv("VOLUME_NAME"))   != NULL &&
      (volume_action = getenv("VOLUME_ACTION")) != NULL &&
      strcmp(volume_action, "insert") == 0) {

    device = _cdio_malloc_and_zero(strlen(volume_device) 
				  + strlen(volume_name) + 2);
    if (device == NULL)
      return strdup(DEFAULT_CDIO_DEVICE);
    sprintf(device, "%s/%s", volume_device, volume_name);
    if (stat(device, &stb) != 0 || !S_ISCHR(stb.st_mode)) {
      free(device);
      return strdup(DEFAULT_CDIO_DEVICE);
    }
    return device;
  }
  /* Check if it could be a Solaris media*/
  if((stat(DEFAULT_CDIO_DEVICE, &stb) == 0) && S_ISDIR(stb.st_mode)) {
    device = _cdio_malloc_and_zero(strlen(DEFAULT_CDIO_DEVICE) + 4);
    sprintf(device, "%s/s0", DEFAULT_CDIO_DEVICE);
    return device;
  }
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*! 
  Get disc type associated with cd object.
*/

static discmode_t
get_discmode_solaris (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;
  track_t i_track;
  discmode_t discmode=CDIO_DISC_MODE_NO_INFO;
  struct dk_minfo media;
  int ret;

  /* Get the media info */
  if((ret = ioctl(p_env->gen.fd, DKIOCGMEDIAINFO, &media)) != 0) {
     cdio_warn ("DKIOCGMEDIAINFO failed: %s\n", strerror(errno));
	 return CDIO_DISC_MODE_NO_INFO;
  }
  switch(media.dki_media_type) {
  case DK_CDROM:
  case DK_CDR:
  case DK_CDRW:
  /* Do cdrom detection */
  break;
  case DK_DVDROM:	return CDIO_DISC_MODE_DVD_ROM;
  case DK_DVDR:		discmode = CDIO_DISC_MODE_DVD_R;
  break;
  case DK_DVDRAM:	discmode = CDIO_DISC_MODE_DVD_RAM;
  break;
  case DK_DVDRW:
  case DK_DVDRW+1:	discmode = CDIO_DISC_MODE_DVD_RW;
  break;
  default: /* no valid match */
  return CDIO_DISC_MODE_NO_INFO; 
  }

  /* 
     GNU/Linux ioctl(.., CDROM_DISC_STATUS) does not return "CD DATA
     Form 2" for SVCD's even though they are are form 2. 
     Issue a SCSI MMC-2 FULL TOC command first to try get more
     accurate information.
  */
  discmode = scsi_mmc_get_discmode(p_env->gen.cdio);
  if (CDIO_DISC_MODE_NO_INFO != discmode) 
    return discmode;

  if((discmode == CDIO_DISC_MODE_DVD_RAM || 
      discmode == CDIO_DISC_MODE_DVD_RW ||
      discmode == CDIO_DISC_MODE_DVD_R)) {
    /* Fallback to uscsi if we can */
    if(geteuid() == 0)
      return get_discmode_solaris(p_user_data);
    return discmode;
  }

  if (!p_env->gen.toc_init) 
    read_toc_solaris (p_env);

  if (!p_env->gen.toc_init) 
    return CDIO_DISC_MODE_NO_INFO;

  for (i_track = p_env->gen.i_first_track; 
       i_track < p_env->gen.i_first_track + p_env->tochdr.cdth_trk1 ; 
       i_track ++) {
    track_format_t track_fmt=get_track_format_solaris(p_env, i_track);

    switch(track_fmt) {
    case TRACK_FORMAT_AUDIO:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_DA;
	  break;
	case CDIO_DISC_MODE_CD_DA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	  discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_XA:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_XA;
	  break;
	case CDIO_DISC_MODE_CD_XA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_DATA:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_DATA;
	  break;
	case CDIO_DISC_MODE_CD_DATA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_ERROR:
    default:
      discmode = CDIO_DISC_MODE_ERROR;
    }
  }
  return discmode;
}

/*!  
  Get format of track. 
*/
static track_format_t
get_track_format_solaris(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if ( !p_env ) return TRACK_FORMAT_ERROR;
  if (!p_env->gen.init) init_solaris(p_env);
  if (!p_env->gen.toc_init) read_toc_solaris (p_user_data) ;

  if ( (i_track > p_env->gen.i_tracks+p_env->gen.i_first_track) 
       || i_track < p_env->gen.i_first_track)
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
_cdio_get_track_green(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if ( !p_env ) return false;
  if (!p_env->gen.init) init_solaris(p_env);
  if (!p_env->gen.toc_init) read_toc_solaris (p_env) ;

  if (i_track >= p_env->gen.i_tracks+p_env->gen.i_first_track 
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
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no entry.
*/
static bool
_cdio_get_track_msf(void *p_user_data, track_t i_track, msf_t *msf)
{
  _img_private_t *p_env = p_user_data;

  if (NULL == msf) return false;

  if (!p_env->gen.init) init_solaris(p_env);
  if (!p_env->gen.toc_init) read_toc_solaris (p_env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) 
    i_track = p_env->gen.i_tracks + p_env->gen.i_first_track;

  if (i_track > (p_env->gen.i_tracks+p_env->gen.i_first_track) 
      || i_track < p_env->gen.i_first_track) {
    return false;
  } else {
    struct cdrom_tocentry *msf0 = &p_env->tocent[i_track-1];
    msf->m = cdio_to_bcd8(msf0->cdte_addr.msf.minute);
    msf->s = cdio_to_bcd8(msf0->cdte_addr.msf.second);
    msf->f = cdio_to_bcd8(msf0->cdte_addr.msf.frame);
    return true;
  }
}

/* Set CD-ROM drive speed */
static driver_return_code_t
set_speed_solaris (void *p_user_data, int i_speed)
{
  const _img_private_t *p_env = p_user_data;

  if (!p_env) return DRIVER_OP_UNINIT;
  return ioctl(p_env->gen.fd, CDROMSDRVSPEED, i_speed);
}

#else 
/*!
  Return a string containing the default VCD device if none is specified.
 */
char *
cdio_get_default_device_solaris(void)
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

#endif /* HAVE_SOLARIS_CDROM */

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_solaris (void)
{
#ifndef HAVE_SOLARIS_CDROM
  return NULL;
#else
  char volpath[256];
  struct stat st;
  char **drives = NULL;
  unsigned int i_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;

  globbuf.gl_offs = 0;
  glob("/vol/dev/aliases/cdrom*", GLOB_DOOFFS, NULL, &globbuf);
  for (i=0; i<globbuf.gl_pathc; i++) {
    if(stat(globbuf.gl_pathv[i], &st) < 0)
      continue;

    /* Check if this is a directory, if so it's probably Solaris media */
    if(S_ISDIR(st.st_mode)) {
      sprintf(volpath, "%s/s0", globbuf.gl_pathv[i]);
      if(stat(volpath, &st) == 0)
        cdio_add_device_list(&drives, volpath, &i_files);
	}else
      cdio_add_device_list(&drives, globbuf.gl_pathv[i], &i_files);
  }
  globfree(&globbuf);
#else
  if(stat(DEFAULT_CDIO_DEVICE, &st) == 0) {
    /* Check if this is a directory, if so it's probably Solaris media */
    if(S_ISDIR(st.st_mode)) {
      sprintf(volpath, "%s/s0", DEFAULT_CDIO_DEVICE);
      if(stat(volpath, &st) == 0)
        cdio_add_device_list(&drives, volpath, &i_files);
    }else
      cdio_add_device_list(&drives, DEFAULT_CDIO_DEVICE, &i_files);
  }
#endif /*HAVE_GLOB_H*/
  cdio_add_device_list(&drives, NULL, &i_files);
  return drives;
#endif /*HAVE_SOLARIS_CDROM*/
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_solaris (const char *psz_source_name)
{
  return cdio_open_am_solaris(psz_source_name, NULL);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_solaris (const char *psz_orig_source, const char *access_mode)
{

#ifdef HAVE_SOLARIS_CDROM
  CdIo *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs_t _funcs;

  _funcs.eject_media            = eject_media_solaris;
  _funcs.free                   = cdio_generic_free;
  _funcs.get_arg                = get_arg_solaris;
  _funcs.get_blocksize          = get_blocksize_mmc,
  _funcs.get_cdtext             = get_cdtext_generic;
  _funcs.get_default_device     = cdio_get_default_device_solaris;
  _funcs.get_devices            = cdio_get_devices_solaris;
  _funcs.get_disc_last_lsn      = get_disc_last_lsn_sunos;
  _funcs.get_discmode           = get_discmode_solaris;
  _funcs.get_drive_cap          = get_drive_cap_mmc;
  _funcs.get_first_track_num    = get_first_track_num_generic;
  _funcs.get_hwinfo             = NULL;
  _funcs.get_mcn                = get_mcn_mmc,
  _funcs.get_num_tracks         = get_num_tracks_generic;
  _funcs.get_track_channels     = get_track_channels_generic,
  _funcs.get_track_copy_permit  = get_track_copy_permit_generic,
  _funcs.get_track_format       = get_track_format_solaris;
  _funcs.get_track_green        = _cdio_get_track_green;
  _funcs.get_track_lba          = NULL; /* This could be done if need be. */
  _funcs.get_track_preemphasis  = get_track_preemphasis_generic,
  _funcs.get_track_msf          = _cdio_get_track_msf;
  _funcs.lseek                  = cdio_generic_lseek;
  _funcs.read                   = cdio_generic_read;
  _funcs.read_audio_sectors     = _read_audio_sectors_solaris;
  _funcs.read_mode1_sector      = _read_mode1_sector_solaris;
  _funcs.read_mode1_sectors     = _read_mode1_sectors_solaris;
  _funcs.read_mode2_sector      = _read_mode2_sector_solaris;
  _funcs.read_mode2_sectors     = _read_mode2_sectors_solaris;
  _funcs.read_toc               = read_toc_solaris;
  _funcs.run_scsi_mmc_cmd       = run_scsi_cmd_solaris;
  _funcs.set_arg                = _set_arg_solaris;
  _funcs.set_blocksize          = set_blocksize_mmc;
  _funcs.set_speed              = set_speed_solaris;

  _data                 = _cdio_malloc (sizeof (_img_private_t));

  _data->access_mode    = _AM_SUN_CTRL_SCSI;
  _data->gen.init       = false;
  _data->gen.fd         = -1;
  _data->gen.toc_init   = false;
  _data->gen.b_cdtext_init  = false;
  _data->gen.b_cdtext_error = false;

  if (NULL == psz_orig_source) {
    psz_source = cdio_get_default_device_solaris();
    if (NULL == psz_source) return NULL;
    _set_arg_solaris(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      _set_arg_solaris(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
#if 0
      cdio_info ("source %s is not a device", psz_orig_source);
#endif
      free(_data);
      return NULL;
    }
  }

  ret = cdio_new ( (void *) _data, &_funcs );
  if (ret == NULL) return NULL;

  ret->driver_id = DRIVER_SOLARIS;

  if (init_solaris(_data))
    return ret;
  else {
    cdio_generic_free (_data);
    return NULL;
  }

#else 
  return NULL;
#endif /* HAVE_SOLARIS_CDROM */

}

bool
cdio_have_solaris (void)
{
#ifdef HAVE_SOLARIS_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_SOLARIS_CDROM */
}
