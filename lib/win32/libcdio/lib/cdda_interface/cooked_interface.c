/*
  $Id: cooked_interface.c,v 1.14 2005/01/25 11:04:45 rocky Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Original interface.c Copyright (C) 1994-1997 
             Eissfeldt heiko@colossus.escape.de
  Current blenderization Copyright (C) 1998-1999 Monty xiphmont@mit.edu
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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
/******************************************************************
 *
 * CDROM code specific to the cooked ioctl interface
 *
 ******************************************************************/

#include "common_interface.h"
#include "low_interface.h"
#include "utils.h"

#ifdef _XBOX
#include "xtl.h"
#else
#include <windows.h>
#endif

#include "portable.h"

/*! reads TOC via libcdio and returns the number of tracks in the disc. 
    0 is returned if there was an error.
*/
static int 
cooked_readtoc (cdrom_drive_t *d)
{
  int i;
  track_t i_track;

  /* Save TOC Entries */
  d->tracks = cdio_get_num_tracks(d->p_cdio) ;

  if (CDIO_INVALID_TRACK == d->tracks) return 0;

  i_track   = cdio_get_first_track_num(d->p_cdio);
  
  for ( i=0; i < d->tracks; i++) {
    d->disc_toc[i].bTrack = i_track;
    d->disc_toc[i].dwStartSector = cdio_get_track_lsn(d->p_cdio, i_track);
    i_track++;
  }

  d->disc_toc[i].bTrack = i_track;
  d->disc_toc[i].dwStartSector = cdio_get_track_lsn(d->p_cdio, 
						    CDIO_CDROM_LEADOUT_TRACK);

  d->cd_extra=FixupTOC(d, i_track);
  return --i_track;  /* without lead-out */
}


/* Set operating speed */
static int 
cooked_setspeed(cdrom_drive_t *d, int i_speed)
{
  return cdio_set_speed(d->p_cdio, i_speed);
}

/* read 'i_sector' adjacent audio sectors
 * into buffer '*p' beginning at sector 'begin'
 */

static long int
read_blocks (cdrom_drive_t *d, void *p, lsn_t begin, long i_sectors)
{
  int retry_count = 0;
  int err;
  char *buffer=(char *)p;

  do {
    err = cdio_read_audio_sectors( d->p_cdio, buffer, begin, i_sectors);
    
    if ( 0 != err ) {
      if (!d->error_retry) return -7;
      
      if (i_sectors==1) {
	/* *Could* be I/O or media error.  I think.  If we're at
	   30 retries, we better skip this unhappy little
	   sector. */
	if (retry_count>MAX_RETRIES-1) {
	  char b[256];
	  snprintf(b, sizeof(b), 
		   "010: Unable to access sector %ld: skipping...\n",
		   (long int) begin);
	  cderror(d, b);
	  return -10;
	}
	break;
      }

      if(retry_count>4)
	if(i_sectors>1)
	  i_sectors=i_sectors*3/4;
      retry_count++;
      if (retry_count>MAX_RETRIES) {
	cderror(d,"007: Unknown, unrecoverable error reading data\n");
	return(-7);
      }
    } else
      break;
  } while (err);
  
  return(i_sectors);
}

typedef enum  {
  JITTER_NONE = 0,
  JITTER_SMALL= 1,
  JITTER_LARGE= 2,
  JITTER_MASSIVE=3
} jitter_baddness_t;

/* read 'i_sector' adjacent audio sectors
 * into buffer '*p' beginning at sector 'begin'
 */

static long int
jitter_read (cdrom_drive_t *d, void *p, lsn_t begin, long i_sectors,
	     jitter_baddness_t jitter_badness)
{
  static int i_jitter=0;
  int jitter_flag;
  long i_sectors_orig = i_sectors;
  long i_jitter_offset = 0;
  
  char *p_buf=malloc(CDIO_CD_FRAMESIZE_RAW*(i_sectors+1));
  
  if (d->i_test_flags & CDDA_TEST_ALWAYS_JITTER)
    jitter_flag = 1;
  else 
    jitter_flag = (rand() > .9) ? 1 : 0;
  
  if (jitter_flag) {
    int i_coeff = 0;
    int i_jitter_sectors = 0;
    switch(jitter_badness) {
    case JITTER_SMALL  : i_coeff =   4; break;
    case JITTER_LARGE  : i_coeff =  32; break;
    case JITTER_MASSIVE: i_coeff = 128; break;
    case JITTER_NONE   :
    default            : ;
    }
    i_jitter = i_coeff * (int)((rand()-.5)*CDIO_CD_FRAMESIZE_RAW/8);
    
    /* We may need to add another sector to compensate for the bytes that
       will be dropped off when jittering, and the begin location may
       be a little different.
    */
    i_jitter_sectors = i_jitter / CDIO_CD_FRAMESIZE_RAW;

    if (i_jitter >= 0) 
      i_jitter_offset  = i_jitter % CDIO_CD_FRAMESIZE_RAW;
    else {
      i_jitter_offset  = CDIO_CD_FRAMESIZE_RAW - 
	(-i_jitter % CDIO_CD_FRAMESIZE_RAW);
      i_jitter_sectors--;
    }
    
    
    if (begin + i_jitter_sectors > 0) {
      char buffer[256];
      sprintf(buffer, "jittering by %d, offset %ld\n", i_jitter, 
	      i_jitter_offset);
      cdmessage(d,buffer); 

      begin += i_jitter_sectors;
      i_sectors ++; 
    } else 
      i_jitter_offset = 0;
    
  }

  i_sectors = read_blocks(d, p_buf, begin, i_sectors);

  if (i_sectors < 0) return i_sectors;
  
  if (i_sectors < i_sectors_orig) 
    /* Had to reduce # of sectors due to read errors. So give full amount,
       with no jittering. */
    memcpy(p, p_buf, i_sectors*CDIO_CD_FRAMESIZE_RAW);
  else 
    /* Got full amount, but now adjust size for jittering. */
    memcpy(p, p_buf+i_jitter_offset, i_sectors_orig*CDIO_CD_FRAMESIZE_RAW);

  free(p_buf);
  return(i_sectors);
}

/* read 'i_sector' adjacent audio sectors
 * into buffer '*p' beginning at sector 'begin'
 */

static long int
cooked_read (cdrom_drive_t *d, void *p, lsn_t begin, long i_sectors)
{
  jitter_baddness_t jitter_badness = d->i_test_flags & 0x3;

  /* read d->nsectors at a time, max. */
  i_sectors = ( i_sectors > d->nsectors && d->nsectors > 0 ) 
    ? d->nsectors : i_sectors;

  /* If we are testing under-run correction, we will deliberately set
     what we read a frame short.  */
  if (d->i_test_flags & CDDA_TEST_UNDERRUN ) 
    i_sectors--;

  if (jitter_badness) {
    return jitter_read(d, p, begin, i_sectors, jitter_badness);
  } else 
    return read_blocks(d, p, begin, i_sectors);
  
}

/* hook */
static int Dummy (cdrom_drive_t *d,int Switch)
{
  return(0);
}

static int 
verify_read_command(cdrom_drive_t *d)
{
  int i;
  int16_t *buff=malloc(CDIO_CD_FRAMESIZE_RAW);
  int audioflag=0;
  int i_test_flags = d->i_test_flags;

  d->i_test_flags = 0;

  cdmessage(d,"Verifying drive can read CDDA...\n");

  d->enable_cdda(d,1);

  for(i=1;i<=d->tracks;i++){
    if(cdda_track_audiop(d,i)==1){
      long firstsector=cdda_track_firstsector(d,i);
      long lastsector=cdda_track_lastsector(d,i);
      long sector=(firstsector+lastsector)>>1;
      audioflag=1;

      if(d->read_audio(d,buff,sector,1)>0){
	cdmessage(d,"\tExpected command set reads OK.\n");
	d->enable_cdda(d,0);
	free(buff);
	d->i_test_flags = i_test_flags;
	return(0);
      }
    }
  }
 
  d->enable_cdda(d,0);

  if(!audioflag){
    cdmessage(d,"\tCould not find any audio tracks on this disk.\n");
    return(-403);
  }

  cdmessage(d,"\n\tUnable to read any data; "
	    "drive probably not CDDA capable.\n");
  
  cderror(d,"006: Could not read any data from drive\n");

  free(buff);
  return(-6);
}

#include "drive_exceptions.h"

#ifdef HAVE_LINUX_MAJOR_H
static void 
check_exceptions(cdrom_drive_t *d, const exception_t *list)
{

  int i=0;
  while(list[i].model){
    if(!strncmp(list[i].model,d->drive_model,strlen(list[i].model))){
      if(list[i].bigendianp!=-1)d->bigendianp=list[i].bigendianp;
      return;
    }
    i++;
  }
}
#endif /* HAVE_LINUX_MAJOR_H */

/* set function pointers to use the ioctl routines */
int 
cooked_init_drive (cdrom_drive_t *d)
{
  int ret;

#if HAVE_LINUX_MAJOR_H
  switch(d->drive_type){
  case MATSUSHITA_CDROM_MAJOR:	/* sbpcd 1 */
  case MATSUSHITA_CDROM2_MAJOR:	/* sbpcd 2 */
  case MATSUSHITA_CDROM3_MAJOR:	/* sbpcd 3 */
  case MATSUSHITA_CDROM4_MAJOR:	/* sbpcd 4 */
    /* don't make the buffer too big; this sucker don't preempt */

    cdmessage(d,"Attempting to set sbpcd buffer size...\n");

    d->nsectors=8;

#if BUFSIZE_DETERMINATION_FIXED
    while(1){

      /* this ioctl returns zero on error; exactly wrong, but that's
         what it does. */

      if (ioctl(d->ioctl_fd, CDROMAUDIOBUFSIZ, d->nsectors)==0) {
	d->nsectors>>=1;
	if(d->nsectors==0){
	  char buffer[256];
	  d->nsectors=8;
	  sprintf(buffer,"\tTrouble setting buffer size.  Defaulting to %d sectors.\n",
		  d->nsectors);
	  cdmessage(d,buffer);
	  break; /* Oh, well.  Try to read anyway.*/
	}
      } else {
	char buffer[256];
	sprintf(buffer,"\tSetting read block size at %d sectors (%ld bytes).\n",
		d->nsectors,(long)d->nsectors*CDIO_CD_FRAMESIZE_RAW);
	cdmessage(d,buffer);
	break;
      }
    }
#endif /* BUFSIZE_DETERMINATION_FIXED */

    break;
  case IDE0_MAJOR:
  case IDE1_MAJOR:
  case IDE2_MAJOR:
  case IDE3_MAJOR:
    d->nsectors=8; /* it's a define in the linux kernel; we have no
		      way of determining other than this guess tho */
    d->bigendianp=0;
    d->is_atapi=1;

    check_exceptions(d, atapi_list);

    break;
  default:
    d->nsectors=25;  /* The max for SCSI MMC2 */
  }
#else
  { 	 
    char buffer[256]; 	 
    d->nsectors = 8; 	 
    sprintf(buffer,"\tSetting read block size at %d sectors (%ld bytes).\n", 	 
	    d->nsectors,(long)d->nsectors*CDIO_CD_FRAMESIZE_RAW); 	 
    cdmessage(d,buffer); 	 
  } 	 
#endif /*HAVE_LINUX_MAJOR_H*/

  d->enable_cdda = Dummy;
  d->set_speed   = cooked_setspeed;
  d->read_toc    = cooked_readtoc;
  d->read_audio  = cooked_read;

  ret = d->tracks = d->read_toc(d);
  if(d->tracks<1)
    return(ret);

  d->opened=1;

  if( (ret=verify_read_command(d)) ) return(ret);

  d->error_retry=1;

  return(0);
}

