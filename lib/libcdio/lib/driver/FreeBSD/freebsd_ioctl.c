/*
    $Id: freebsd_ioctl.c,v 1.3 2005/01/27 04:00:48 rocky Exp $

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

/* This file contains FreeBSD-specific code and implements low-level
   control of the CD drive. Culled initially I think from xine's or
   mplayer's FreeBSD code with lots of modifications.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: freebsd_ioctl.c,v 1.3 2005/01/27 04:00:48 rocky Exp $";

#ifdef HAVE_FREEBSD_CDROM

#include "freebsd.h"

/* Check a drive to see if it is a CD-ROM 
   Return 1 if a CD-ROM. 0 if it exists but isn't a CD-ROM drive
   and -1 if no device exists .
*/
bool
cdio_is_cdrom_freebsd_ioctl(char *drive, char *mnttype)
{
  bool is_cd=false;
  int cdfd;
  struct ioc_toc_header    tochdr;
  
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
    if ( ioctl(cdfd, CDIOREADTOCHEADER, &tochdr) != -1 ) {
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

/*!
   Reads a single mode2 sector from cd device into data starting from lsn.
   Returns 0 if no error. 
 */
int
read_audio_sectors_freebsd_ioctl (_img_private_t *_obj, void *data, lsn_t lsn,
				  unsigned int nblocks)
{
  unsigned char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct ioc_read_audio cdda;

  cdda.address.lba    = lsn;
  cdda.buffer         = buf;
  cdda.nframes        = nblocks;
  cdda.address_format = CDIO_CDROM_LBA;

  /* read a frame */
  if(ioctl(_obj->gen.fd, CDIOCREADAUDIO, &cdda) < 0) {
    perror("CDIOCREADAUDIO");
    return 1;
  }
  memcpy (data, buf, CDIO_CD_FRAMESIZE_RAW);

  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int
read_mode2_sector_freebsd_ioctl (_img_private_t *env, void *data, lsn_t lsn, 
				 bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int retval;

  if ( !b_form2 )
    return cdio_generic_read_form1_sector (env, buf, lsn);
  
  if ( (retval = read_audio_sectors_freebsd_ioctl (env, buf, lsn, 1)) )
    return retval;
    
  memcpy (data, buf + CDIO_CD_XA_SYNC_HEADER, M2RAW_SECTOR_SIZE);
  
  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
lsn_t
get_disc_last_lsn_freebsd_ioctl (_img_private_t *_obj)
{
  struct ioc_read_toc_single_entry tocent;
  uint32_t size;

  tocent.track = CDIO_CDROM_LEADOUT_TRACK;
  tocent.address_format = CDIO_CDROM_LBA;
  if (ioctl (_obj->gen.fd, CDIOREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      exit (EXIT_FAILURE);
    }

  size = tocent.entry.addr.lba;

  return size;
}

/*!
  Eject media. Return 1 if successful, 0 otherwise.
 */
int 
eject_media_freebsd_ioctl (_img_private_t *env) 
{
  _img_private_t *_obj = env;
  int ret=2;
  int fd;

  if ((fd = open(_obj->gen.source_name, O_RDONLY|O_NONBLOCK)) > -1) {
    ret = 1;
    if (ioctl(fd, CDIOCALLOW) == -1) {
      cdio_warn("ioctl(fd, CDIOCALLOW) failed: %s\n", strerror(errno));
    } else if (ioctl(fd, CDIOCEJECT) == -1) {
      cdio_warn("ioctl(CDIOCEJECT) failed: %s\n", strerror(errno));
    } else {
      ret = 0;
    }
    close(fd);
  }

  return ret;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

  FIXME: This is just a guess. 

 */
char *
get_mcn_freebsd_ioctl (const _img_private_t *env) {

  struct ioc_read_subchannel subchannel;
  struct cd_sub_channel_info subchannel_info;

  subchannel.address_format = CDIO_CDROM_MSF;
  subchannel.data_format    = CDIO_SUBCHANNEL_MEDIA_CATALOG;
  subchannel.track          = 0;
  subchannel.data_len       = sizeof(subchannel_info);
  subchannel.data           = &subchannel_info;

  if(ioctl(env->gen.fd, CDIOCREADSUBCHANNEL, &subchannel) < 0) {
    perror("CDIOCREADSUBCHANNEL");
    return NULL;
  }

  /* Probably need a loop over tracks rather than give up if we 
     can't find in track 0.
   */
  if (subchannel_info.what.media_catalog.mc_valid)
    return strdup(subchannel_info.what.media_catalog.mc_number);
  else 
    return NULL;
}

/*!  
  Get format of track. 

  FIXME: We're just guessing this from the GNU/Linux code.
  
*/
track_format_t
get_track_format_freebsd_ioctl(const _img_private_t *env, track_t i_track) 
{
  struct ioc_read_subchannel subchannel;
  struct cd_sub_channel_info subchannel_info;

  subchannel.address_format = CDIO_CDROM_LBA;
  subchannel.data_format    = CDIO_SUBCHANNEL_CURRENT_POSITION;
  subchannel.track          = i_track;
  subchannel.data_len       = 1;
  subchannel.data           = &subchannel_info;

  if(ioctl(env->gen.fd, CDIOCREADSUBCHANNEL, &subchannel) < 0) {
    perror("CDIOCREADSUBCHANNEL");
    return 1;
  }
  
  if (subchannel_info.what.position.control == 0x04) {
    if (subchannel_info.what.position.data_format == 0x10)
      return TRACK_FORMAT_CDI;
    else if (subchannel_info.what.position.data_format == 0x20) 
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
bool
get_track_green_freebsd_ioctl(const _img_private_t *env, track_t i_track) 
{
  struct ioc_read_subchannel subchannel;
  struct cd_sub_channel_info subchannel_info;

  subchannel.address_format = CDIO_CDROM_LBA;
  subchannel.data_format    = CDIO_SUBCHANNEL_CURRENT_POSITION;
  subchannel.track          = i_track;
  subchannel.data_len       = 1;
  subchannel.data           = &subchannel_info;

  if(ioctl(env->gen.fd, CDIOCREADSUBCHANNEL, &subchannel) < 0) {
    perror("CDIOCREADSUBCHANNEL");
    return 1;
  }
  
  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cdinfo for a while.
   */
  return (subchannel_info.what.position.control & 2) != 0;
}

#endif /* HAVE_FREEBSD_CDROM */
