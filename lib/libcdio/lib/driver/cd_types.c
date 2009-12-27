/*
    $Id: cd_types.c,v 1.2 2005/01/02 22:43:41 rocky Exp $

    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

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
/* 
   This tries to determine what kind of CD-image or filesystems on a 
   track we've got.
*/
#include "config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <cdio/logging.h>
#include <cdio/util.h>
#include <cdio/cd_types.h>

/*
Subject:   -65- How can I read an IRIX (EFS) CD-ROM on a machine which
                doesn't use EFS?
Date: 18 Jun 1995 00:00:01 EST

  You want 'efslook', at
  ftp://viz.tamu.edu/pub/sgi/software/efslook.tar.gz.

and
! Robert E. Seastrom <rs@access4.digex.net>'s software (with source
! code) for using an SGI CD-ROM on a Macintosh is at
! ftp://bifrost.seastrom.com/pub/mac/CDROM-Jumpstart.sit151.hqx.

*/

static char buffer[6][CDIO_CD_FRAMESIZE_RAW];  /* for CD-Data */

/* Some interesting sector numbers stored in the above buffer. */
#define ISO_SUPERBLOCK_SECTOR  16  /* buffer[0] */
#define UFS_SUPERBLOCK_SECTOR   4  /* buffer[2] */
#define BOOT_SECTOR            17  /* buffer[3] */
#define VCD_INFO_SECTOR       150  /* buffer[4] */
#define XISO_SECTOR	       32  /* buffer[4] */
#define UDFX_SECTOR	       32  /* buffer[4] */
#define UDF_ANCHOR_SECTOR     256  /* buffer[5] */


typedef struct signature
{
  unsigned int buf_num;
  unsigned int offset;
  const char *sig_str;
  const char *description;
} signature_t;

static signature_t sigs[] =
  {
/*buffer[x] off look for     description */
    {0,     0, "MICROSOFT*XBOX*MEDIA", "XBOX CD"},
    {0,     1, "BEA01",      "UDF"}, 
    {0,     1, ISO_STANDARD_ID,      "ISO 9660"}, 
    {0,     1, "CD-I",       "CD-I"}, 
    {0,     8, "CDTV",       "CDTV"}, 
    {0,     8, "CD-RTOS",    "CD-RTOS"}, 
    {0,     9, "CDROM",      "HIGH SIERRA"}, 
    {0,    16, "CD-BRIDGE",  "BRIDGE"}, 
    {0,  ISO_XA_MARKER_OFFSET, ISO_XA_MARKER_STRING,   "XA"}, 
    {1,    64, "PPPPHHHHOOOOTTTTOOOO____CCCCDDDD",  "PHOTO CD"}, 
    {1, 0x438, "\x53\xef",   "EXT2 FS"}, 
    {2,  1372, "\x54\x19\x01\x0", "UFS"}, 
    {3,     7, "EL TORITO",  "BOOTABLE"}, 
    {4,     0, "VIDEO_CD",   "VIDEO CD"}, 
    {4,     0, "SUPERVCD",   "SVCD or Chaoji VCD"}, 
    { 0 }
  };


/* The below index into the above sigs array. Make sure things match. */
#define INDEX_XISO      0 /* Microsoft X-BOX filesystem */
#define INDEX_UDF       1
#define INDEX_ISOFS     2
#define INDEX_CD_I      3
#define INDEX_CDTV      4 
#define INDEX_CD_RTOS   5
#define INDEX_HS        6
#define INDEX_BRIDGE    7
#define INDEX_XA        8
#define INDEX_PHOTO_CD  9
#define INDEX_EXT2     10 
#define INDEX_UFS      11
#define INDEX_BOOTABLE 12
#define INDEX_VIDEO_CD 13 /* Video CD */
#define INDEX_SVCD     14 /* CVD *or* SVCD */


/* 
   Read a particular block into the global array to be used for further
   analysis later.
*/
static int 
_cdio_read_block(const CdIo *cdio, int superblock, uint32_t offset, 
		 uint8_t bufnum, track_t i_track)
{
  unsigned int track_sec_count = cdio_get_track_sec_count(cdio, i_track);
  memset(buffer[bufnum], 0, CDIO_CD_FRAMESIZE);

  if ( track_sec_count < superblock) {
    cdio_debug("reading block %u skipped track %d has only %u sectors\n", 
	       superblock, i_track, track_sec_count);
    return -1;
  }
  
  cdio_debug("about to read sector %lu\n", 
	     (long unsigned int) offset+superblock);

  if (cdio_get_track_green(cdio,  i_track)) {
    if (0 > cdio_read_mode2_sector(cdio, buffer[bufnum], 
				   offset+superblock, false))
      return -1;
  } else {
    if (0 > cdio_read_mode1_sector(cdio, buffer[bufnum], 
				    offset+superblock, false))
      return -1;
  }

  return 0;
}

/* 
   Return true if the previously read-in buffer contains a "signature" that
   matches index "num".
 */
static bool 
_cdio_is_it(int num) 
{
  signature_t *sigp=&sigs[num];
  int len=strlen(sigp->sig_str);

  /* TODO: check that num < largest sig. */
  return 0 == memcmp(&buffer[sigp->buf_num][sigp->offset], sigp->sig_str, len);
}

static int 
_cdio_is_hfs(void)
{
  return (0 == memcmp(&buffer[1][512],"PM",2)) ||
    (0 == memcmp(&buffer[1][512],"TS",2)) ||
    (0 == memcmp(&buffer[1][1024], "BD",2));
}

static int 
_cdio_is_3do(void)
{
  return (0 == memcmp(&buffer[1][0],"\x01\x5a\x5a\x5a\x5a\x5a\x01", 7)) &&
    (0 == memcmp(&buffer[1][40], "CD-ROM", 6));
}

static int 
_cdio_is_joliet(void)
{
  return 2 == buffer[3][0] && buffer[3][88] == 0x25 && buffer[3][89] == 0x2f;
}

static int 
_cdio_is_UDF(void)
{
  return 2 == ((uint16_t)buffer[5][0] | ((uint16_t)buffer[5][1] << 8));
}

/* ISO 9660 volume space in M2F1_SECTOR_SIZE byte units */
static int 
_cdio_get_iso9660_fs_sec_count(void)
{
  return ((buffer[0][80] & 0xff) |
	 ((buffer[0][81] & 0xff) << 8) |
	 ((buffer[0][82] & 0xff) << 16) |
	 ((buffer[0][83] & 0xff) << 24));
}

static int 
_cdio_get_joliet_level( void )
{
  switch (buffer[3][90]) {
  case 0x40: return 1;
  case 0x43: return 2;
  case 0x45: return 3;
  }
  return 0;
}

/* 
   Try to determine what kind of CD-image and/or filesystem we
   have at track i_track. Return information about the CD image
   is returned in cdio_analysis and the return value.
*/
cdio_fs_anal_t
cdio_guess_cd_type(const CdIo_t *cdio, int start_session, track_t i_track, 
		   /*out*/ cdio_iso_analysis_t *iso_analysis)
{
  int ret = CDIO_FS_UNKNOWN;
  bool sector0_read_ok;
  
  if (TRACK_FORMAT_AUDIO == cdio_get_track_format(cdio, i_track))
    return CDIO_FS_AUDIO;

  if ( _cdio_read_block(cdio, ISO_PVD_SECTOR, start_session, 
			0, i_track) < 0 )
    return CDIO_FS_UNKNOWN;
  
  if ( _cdio_is_it(INDEX_XISO) )
    return CDIO_FS_ANAL_XISO;

  if (_cdio_read_block(cdio, ISO_SUPERBLOCK_SECTOR, start_session, 0, 
		       i_track) < 0)
    return ret;

  if ( _cdio_is_it(INDEX_UDF) ) {
    /* Detect UDF version 
       Test if we have a valid version of UDF the xbox can read natively */
    if (_cdio_read_block(cdio, 35, start_session, 5, i_track) < 0)
      return CDIO_FS_UNKNOWN;

     iso_analysis->UDFVerMinor=(unsigned int)buffer[5][240];
     iso_analysis->UDFVerMajor=(unsigned int)buffer[5][241];
     /*	Read disc label */
     if (_cdio_read_block(cdio, 32, start_session, 5, i_track) < 0)
       return CDIO_FS_UDF;

     strncpy(iso_analysis->iso_label, buffer[5]+25, 33);
     iso_analysis->iso_label[32] = '\0';
     return CDIO_FS_UDF;
   }

  /* We have something that smells of a filesystem. */
  if (_cdio_is_it(INDEX_CD_I) && _cdio_is_it(INDEX_CD_RTOS) 
      && !_cdio_is_it(INDEX_BRIDGE) && !_cdio_is_it(INDEX_XA)) {
    return CDIO_FS_INTERACTIVE;
  } else {	
    /* read sector 0 ONLY, when NO greenbook CD-I !!!! */

    sector0_read_ok = 
      _cdio_read_block(cdio, 0, start_session, 1, i_track) == 0;
    
    if (_cdio_is_it(INDEX_HS))
      ret |= CDIO_FS_HIGH_SIERRA;
    else if (_cdio_is_it(INDEX_ISOFS)) {
      if (_cdio_is_it(INDEX_CD_RTOS) && _cdio_is_it(INDEX_BRIDGE))
	ret = CDIO_FS_ISO_9660_INTERACTIVE;
      else if (_cdio_is_hfs())
	ret = CDIO_FS_ISO_HFS;
      else
	ret = CDIO_FS_ISO_9660;
      iso_analysis->isofs_size = _cdio_get_iso9660_fs_sec_count();
      strncpy(iso_analysis->iso_label, buffer[0]+40,33);
      iso_analysis->iso_label[32] = '\0';
      
      if ( _cdio_read_block(cdio, UDF_ANCHOR_SECTOR, start_session, 5, 
			    i_track) < 0)
	return ret;
      
      /* Maybe there is an UDF anchor in IOS session
	 so its ISO/UDF session and we prefere UDF */
      if ( _cdio_is_UDF() ) {
	/* Detect UDF version.
	   Test if we have a valid version of UDF the xbox can read natively */
	if ( _cdio_read_block(cdio, 35, start_session, 5, i_track) < 0)
	  return ret;
	  
	  iso_analysis->UDFVerMinor=(unsigned int)buffer[5][240];
	  iso_analysis->UDFVerMajor=(unsigned int)buffer[5][241];
#if 0
	  /*  We are using ISO/UDF cd's as iso,
	      no need to get UDF disc label */
	  if (_cdio_read_block(cdio, 32, start_session, 5, i_track) < 0)
	    return ret;
	  stnrcpy(iso_analysis->iso_label, buffer[5]+25, 33);
	  iso_analysis->iso_label[32] = '\0';
#endif
	  ret=CDIO_FS_ISO_UDF;
	}

#if 0
      if (_cdio_is_rockridge())
	ret |= CDIO_FS_ANAL_ROCKRIDGE;
#endif

      if (_cdio_read_block(cdio, BOOT_SECTOR, start_session, 3, i_track) < 0)
	return ret;
      
      if (_cdio_is_joliet()) {
	iso_analysis->joliet_level = _cdio_get_joliet_level();
	ret |= CDIO_FS_ANAL_JOLIET;
      }
      if (_cdio_is_it(INDEX_BOOTABLE))
	ret |= CDIO_FS_ANAL_BOOTABLE;
      
      if ( _cdio_is_it(INDEX_XA) && _cdio_is_it(INDEX_ISOFS) 
	  && !(sector0_read_ok && _cdio_is_it(INDEX_PHOTO_CD)) ) {

        if ( _cdio_read_block(cdio, VCD_INFO_SECTOR, start_session, 4, 
			     i_track) < 0 )
	  return ret;
	
	if (_cdio_is_it(INDEX_BRIDGE) && _cdio_is_it(INDEX_CD_RTOS)) {
	  if (_cdio_is_it(INDEX_VIDEO_CD))  ret |= CDIO_FS_ANAL_VIDEOCD;
	  else if (_cdio_is_it(INDEX_SVCD)) ret |= CDIO_FS_ANAL_SVCD;
	} else if (_cdio_is_it(INDEX_SVCD)) ret |= CDIO_FS_ANAL_CVD;

      }
    } 
    else if (_cdio_is_hfs())          ret |= CDIO_FS_HFS;
    else if (sector0_read_ok && _cdio_is_it(INDEX_EXT2)) ret |= CDIO_FS_EXT2;
    else if (_cdio_is_3do())          ret |= CDIO_FS_3DO;
    else {
      if ( _cdio_read_block(cdio, UFS_SUPERBLOCK_SECTOR, start_session, 2, 
			    i_track) < 0 )
	return ret;
      
      if (sector0_read_ok && _cdio_is_it(INDEX_UFS)) 
	ret |= CDIO_FS_UFS;
      else
	ret |= CDIO_FS_UNKNOWN;
    }
  }
  
  /* other checks */
  if (_cdio_is_it(INDEX_XA))       ret |= CDIO_FS_ANAL_XA;
  if (_cdio_is_it(INDEX_PHOTO_CD)) ret |= CDIO_FS_ANAL_PHOTO_CD;
  if (_cdio_is_it(INDEX_CDTV))     ret |= CDIO_FS_ANAL_CDTV;
  return ret;
}
