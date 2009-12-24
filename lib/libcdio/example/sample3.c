/*
  $Id: sample3.c,v 1.9 2005/01/04 04:40:22 rocky Exp $

  Copyright (C) 2003, 2005 Rocky Bernstein <rocky@panix.com>
  
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
   A somewhat simplified program to show the use of cdio_guess_cd_type().
   Figure out the kind of CD image we've got.
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/cd_types.h>

static void
print_analysis(cdio_iso_analysis_t cdio_iso_analysis, 
	       cdio_fs_anal_t fs, int first_data, unsigned int num_audio, 
	       track_t num_tracks, track_t first_track_num, CdIo_t *p_cdio)
{
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_AUDIO:
    break;
  case CDIO_FS_ISO_9660:
    printf("CD-ROM with ISO 9660 filesystem");
    if (fs & CDIO_FS_ANAL_JOLIET) {
      printf(" and joliet extension level %d", cdio_iso_analysis.joliet_level);
    }
    if (fs & CDIO_FS_ANAL_ROCKRIDGE)
      printf(" and rockridge extensions");
    printf("\n");
    break;
  case CDIO_FS_ISO_9660_INTERACTIVE:
    printf("CD-ROM with CD-RTOS and ISO 9660 filesystem\n");
    break;
  case CDIO_FS_HIGH_SIERRA:
    printf("CD-ROM with High Sierra filesystem\n");
    break;
  case CDIO_FS_INTERACTIVE:
    printf("CD-Interactive%s\n", num_audio > 0 ? "/Ready" : "");
    break;
  case CDIO_FS_HFS:
    printf("CD-ROM with Macintosh HFS\n");
    break;
  case CDIO_FS_ISO_HFS:
    printf("CD-ROM with both Macintosh HFS and ISO 9660 filesystem\n");
    break;
  case CDIO_FS_UFS:
    printf("CD-ROM with Unix UFS\n");
    break;
  case CDIO_FS_EXT2:
    printf("CD-ROM with Linux second extended filesystem\n");
	  break;
  case CDIO_FS_3DO:
    printf("CD-ROM with Panasonic 3DO filesystem\n");
    break;
  case CDIO_FS_UNKNOWN:
    printf("CD-ROM with unknown filesystem\n");
    break;
  }
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_ISO_9660:
  case CDIO_FS_ISO_9660_INTERACTIVE:
  case CDIO_FS_ISO_HFS:
    printf("ISO 9660: %i blocks, label `%.32s'\n",
	   cdio_iso_analysis.isofs_size, cdio_iso_analysis.iso_label);
    break;
  }
  if (first_data == 1 && num_audio > 0)
    printf("mixed mode CD   ");
  if (fs & CDIO_FS_ANAL_XA)
    printf("XA sectors   ");
  if (fs & CDIO_FS_ANAL_MULTISESSION)
    printf("Multisession");
  if (fs & CDIO_FS_ANAL_HIDDEN_TRACK)
    printf("Hidden Track   ");
  if (fs & CDIO_FS_ANAL_PHOTO_CD)
    printf("%sPhoto CD   ", 
		      num_audio > 0 ? " Portfolio " : "");
  if (fs & CDIO_FS_ANAL_CDTV)
    printf("Commodore CDTV   ");
  if (first_data > 1)
    printf("CD-Plus/Extra   ");
  if (fs & CDIO_FS_ANAL_BOOTABLE)
    printf("bootable CD   ");
  if (fs & CDIO_FS_ANAL_VIDEOCD && num_audio == 0) {
    printf("Video CD   ");
  }
  if (fs & CDIO_FS_ANAL_SVCD)
    printf("Super Video CD (SVCD) or Chaoji Video CD (CVD)");
  if (fs & CDIO_FS_ANAL_CVD)
    printf("Chaoji Video CD (CVD)");
  printf("\n");
}

int
main(int argc, const char *argv[])
{
  CdIo_t *p_cdio = cdio_open (NULL, DRIVER_UNKNOWN);
  cdio_fs_anal_t fs=0;
  
  track_t num_tracks;
  track_t first_track_num;
  lsn_t start_track;          /* first sector of track */
  lsn_t data_start =0;        /* start of data area */

  int first_data = -1;        /* # of first data track */
  int first_audio = -1;       /* # of first audio track */
  unsigned int num_data  = 0; /* # of data tracks */
  unsigned int num_audio = 0; /* # of audio tracks */
  unsigned int i;

  if (NULL == p_cdio) {
    printf("Problem in trying to find a driver.\n\n");
    return 1;
  }

  first_track_num = cdio_get_first_track_num(p_cdio);
  num_tracks      = cdio_get_num_tracks(p_cdio);

  /* Count the number of data and audio tracks. */
  for (i = first_track_num; i <= num_tracks; i++) {
    if (TRACK_FORMAT_AUDIO == cdio_get_track_format(p_cdio, i)) {
      num_audio++;
      if (-1 == first_audio) first_audio = i;
    } else {
      num_data++;
      if (-1 == first_data)  first_data = i;
    }
  }

  /* try to find out what sort of CD we have */
  if (0 == num_data) {
    printf("Audio CD\n");
  } else {
    /* we have data track(s) */
    int j;
    cdio_iso_analysis_t cdio_iso_analysis; 

    memset(&cdio_iso_analysis, 0, sizeof(cdio_iso_analysis));
    
    for (j = 2, i = first_data; i <= num_tracks; i++) {
      lsn_t lsn;
      track_format_t track_format = cdio_get_track_format(p_cdio, i);
      
      lsn = cdio_get_track_lsn(p_cdio, i);
      
      switch ( track_format ) {
      case TRACK_FORMAT_AUDIO:
      case TRACK_FORMAT_ERROR:
	break;
      case TRACK_FORMAT_CDI:
      case TRACK_FORMAT_XA:
      case TRACK_FORMAT_DATA: 
      case TRACK_FORMAT_PSX: 
	;
      }
      
      start_track = (i == 1) ? 0 : lsn;
      
      /* save the start of the data area */
      if (i == first_data) 
	data_start = start_track;
      
      /* skip tracks which belong to the current walked session */
      if (start_track < data_start + cdio_iso_analysis.isofs_size)
	continue;
      
      fs = cdio_guess_cd_type(p_cdio, start_track, i, &cdio_iso_analysis);
      
      print_analysis(cdio_iso_analysis, fs, first_data, num_audio,
		     num_tracks, first_track_num, p_cdio);
      
      if ( !(CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660 ||
	     CDIO_FSTYPE(fs) == CDIO_FS_ISO_HFS  ||
	     CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660_INTERACTIVE) )
	/* no method for non-ISO9660 multisessions */
	break;	
    }
  }
  cdio_destroy(p_cdio);
  return 0;
}
