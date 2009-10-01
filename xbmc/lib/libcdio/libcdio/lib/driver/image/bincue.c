/*
    $Id: bincue.c,v 1.19 2006/02/13 11:00:53 rocky Exp $

    Copyright (C) 2002, 2003, 2004, 2005, 2006 
    Rocky Bernstein <rocky@panix.com>
    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    cue parsing routine adapted from cuetools
    Copyright (C) 2003 Svend Sanjay Sorensen <ssorensen@fastmail.fm>

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

/* This code implements low-level access functions for a CD images
   residing inside a disk file (*.bin) and its associated cue sheet.
   (*.cue).
*/

static const char _rcsid[] = "$Id: bincue.c,v 1.19 2006/02/13 11:00:53 rocky Exp $";

#include "image.h"
#include "cdio_assert.h"
#include "cdio_private.h"
#include "_cdio_stdio.h"

#include <cdio/logging.h>
#include <cdio/util.h>
#include <cdio/version.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif
#include <ctype.h>

#include "portable.h"
/* reader */

#define DEFAULT_CDIO_DEVICE "videocd.bin"
#define DEFAULT_CDIO_CUE    "videocd.cue"

static lsn_t get_disc_last_lsn_bincue (void *p_user_data);
#include "image_common.h"
static bool     parse_cuefile (_img_private_t *cd, const char *toc_name);

/*!
  Initialize image structures.
 */
static bool
_init_bincue (_img_private_t *p_env)
{
  lsn_t lead_lsn;

  if (p_env->gen.init)
    return false;

  if (!(p_env->gen.data_source = cdio_stdio_new (p_env->gen.source_name))) {
    cdio_warn ("init failed");
    return false;
  }

  /* Have to set init before calling get_disc_last_lsn_bincue() or we will
     get into infinite recursion calling passing right here.
   */
  p_env->gen.init      = true;  
  p_env->gen.i_first_track = 1;
  p_env->psz_mcn       = NULL;
  p_env->disc_mode     = CDIO_DISC_MODE_NO_INFO;

  cdtext_init (&(p_env->gen.cdtext));

  lead_lsn = get_disc_last_lsn_bincue( (_img_private_t *) p_env);

  if (-1 == lead_lsn) return false;

  if ((p_env->psz_cue_name == NULL)) return false;

  /* Read in CUE sheet. */
  if ( !parse_cuefile(p_env, p_env->psz_cue_name) ) return false;

  /* Fake out leadout track and sector count for last track*/
  cdio_lsn_to_msf (lead_lsn, &p_env->tocent[p_env->gen.i_tracks].start_msf);
  p_env->tocent[p_env->gen.i_tracks].start_lba = cdio_lsn_to_lba(lead_lsn);
  p_env->tocent[p_env->gen.i_tracks - p_env->gen.i_first_track].sec_count = 
    cdio_lsn_to_lba(lead_lsn - 
		    p_env->tocent[p_env->gen.i_tracks - p_env->gen.i_first_track].start_lba);

  return true;
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Would be libc's seek() but we have to adjust for the extra track header 
  information in each sector.
*/
static off_t
_lseek_bincue (void *p_user_data, off_t offset, int whence)
{
  _img_private_t *p_env = p_user_data;

  /* real_offset is the real byte offset inside the disk image
     The number below was determined empirically. I'm guessing
     the 1st 24 bytes of a bin file are used for something.
  */
  off_t real_offset=0;

  unsigned int i;

  p_env->pos.lba = 0;
  for (i=0; i<p_env->gen.i_tracks; i++) {
    track_info_t  *this_track=&(p_env->tocent[i]);
    p_env->pos.index = i;
    if ( (this_track->sec_count*this_track->datasize) >= offset) {
      int blocks            = offset / this_track->datasize;
      int rem               = offset % this_track->datasize;
      int block_offset      = blocks * this_track->blocksize;
      real_offset          += block_offset + rem;
      p_env->pos.buff_offset = rem;
      p_env->pos.lba        += blocks;
      break;
    }
    real_offset   += this_track->sec_count*this_track->blocksize;
    offset        -= this_track->sec_count*this_track->datasize;
    p_env->pos.lba += this_track->sec_count;
  }

  if (i==p_env->gen.i_tracks) {
    cdio_warn ("seeking outside range of disk image");
    return DRIVER_OP_ERROR;
  } else {
    real_offset += p_env->tocent[i].datastart;
    return cdio_stream_seek(p_env->gen.data_source, real_offset, whence);
  }
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  FIXME: 
   At present we assume a read doesn't cross sector or track
   boundaries.
*/
static ssize_t
_read_bincue (void *p_user_data, void *data, size_t size)
{
  _img_private_t *p_env = p_user_data;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  char *p = data;
  ssize_t final_size=0;
  ssize_t this_size;
  track_info_t  *this_track=&(p_env->tocent[p_env->pos.index]);
  ssize_t skip_size = this_track->datastart + this_track->endsize;

  while (size > 0) {
    long int rem = this_track->datasize - p_env->pos.buff_offset;
    if ((long int) size <= rem) {
      this_size = cdio_stream_read(p_env->gen.data_source, buf, size, 1);
      final_size += this_size;
      memcpy (p, buf, this_size);
      break;
    }

    /* Finish off reading this sector. */
    cdio_warn ("Reading across block boundaries not finished");

    size -= rem;
    this_size = cdio_stream_read(p_env->gen.data_source, buf, rem, 1);
    final_size += this_size;
    memcpy (p, buf, this_size);
    p += this_size;
    this_size = cdio_stream_read(p_env->gen.data_source, buf, rem, 1);
    
    /* Skip over stuff at end of this sector and the beginning of the next.
     */
    cdio_stream_read(p_env->gen.data_source, buf, skip_size, 1);

    /* Get ready to read another sector. */
    p_env->pos.buff_offset=0;
    p_env->pos.lba++;

    /* Have gone into next track. */
    if (p_env->pos.lba >= p_env->tocent[p_env->pos.index+1].start_lba) {
      p_env->pos.index++;
      this_track=&(p_env->tocent[p_env->pos.index]);
      skip_size = this_track->datastart + this_track->endsize;
    }
  }
  return final_size;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static lsn_t
get_disc_last_lsn_bincue (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;
  long size;

  size = cdio_stream_stat (p_env->gen.data_source);

  if (size % CDIO_CD_FRAMESIZE_RAW)
    {
      cdio_warn ("image %s size (%ld) not multiple of blocksize (%d)", 
		 p_env->gen.source_name, size, CDIO_CD_FRAMESIZE_RAW);
      if (size % M2RAW_SECTOR_SIZE == 0)
	cdio_warn ("this may be a 2336-type disc image");
      else if (size % CDIO_CD_FRAMESIZE_RAW == 0)
	cdio_warn ("this may be a 2352-type disc image");
      /* exit (EXIT_FAILURE); */
    }

  size /= CDIO_CD_FRAMESIZE_RAW;

  return size;
}

#define MAXLINE 4096		/* maximum line length + 1 */

static bool
parse_cuefile (_img_private_t *cd, const char *psz_cue_name)
{
  /* The below declarations may be common in other image-parse routines. */
  FILE *fp;
  char         psz_line[MAXLINE];   /* text of current line read in file fp. */
  unsigned int i_line=0;            /* line number in file of psz_line. */
  int          i = -1;              /* Position in tocent. Same as 
				       cd->gen.i_tracks - 1 */
  char *psz_keyword, *psz_field;
  cdio_log_level_t log_level = (NULL == cd) ? CDIO_LOG_INFO : CDIO_LOG_WARN;
  cdtext_field_t cdtext_key;

  /* The below declarations may be unique to this image-parse routine. */
  int start_index;
  bool b_first_index_for_track=false;

  if (NULL == psz_cue_name) 
    return false;
  
  fp = fopen (psz_cue_name, "r");
  if (fp == NULL) {
    cdio_log(log_level, "error opening %s for reading: %s", 
	     psz_cue_name, strerror(errno));
    return false;
  }

  if (cd) {
    cd->gen.i_tracks=0;
    cd->gen.i_first_track=1;
    cd->gen.b_cdtext_init  = true;
    cd->gen.b_cdtext_error = false;
    cd->psz_mcn=NULL;
  }
  
  while ((fgets(psz_line, MAXLINE, fp)) != NULL) {

    i_line++;

    if (NULL != (psz_keyword = strtok (psz_line, " \t\n\r"))) {
      /* REM remarks ... */
      if (0 == strcmp ("REM", psz_keyword)) {
	;
	
	/* global section */
	/* CATALOG ddddddddddddd */
      } else if (0 == strcmp ("CATALOG", psz_keyword)) {
	if (-1 == i) {
	  if (NULL == (psz_field = strtok (NULL, " \t\n\r"))) {
	    cdio_log(log_level, 
		     "%s line %d after word CATALOG: ",
		     psz_cue_name, i_line);
	    cdio_log(log_level, 
		     "expecting 13-digit media catalog number, got nothing.");
	    goto err_exit;
	  }
	  if (strlen(psz_field) != 13) {
	    cdio_log(log_level, 
		     "%s line %d after word CATALOG: ",
		     psz_cue_name, i_line);
	    cdio_log(log_level, 
		       "Token %s has length %ld. Should be 13 digits.", 
		     psz_field, (long int) strlen(psz_field));
	    goto err_exit;
	  } else {
	    /* Check that we have all digits*/
	    unsigned int i;
	    for (i=0; i<13; i++) {
	      if (!isdigit(psz_field[i])) {
		cdio_log(log_level, 
			 "%s line %d after word CATALOG:", 
			 psz_cue_name, i_line);
		cdio_log(log_level, 
			 "Character \"%c\" at postition %i of token \"%s\" "
			 "is not all digits.", 
			 psz_field[i], i+1, psz_field);
		goto err_exit;
	      }
	    }
	  }
	      
	  if (cd) cd->psz_mcn = strdup (psz_field);
	  if (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	    goto format_error;
	  }
	} else {
	  goto not_in_global_section;
	}
	
	/* FILE "<filename>" <BINARY|WAVE|other?> */
      } else if (0 == strcmp ("FILE", psz_keyword)) {
	if (NULL != (psz_field = strtok (NULL, "\"\t\n\r"))) {
	  if (cd) cd->tocent[i + 1].filename = strdup (psz_field);
	} else {
	  goto format_error;
	}
	
	/* TRACK N <mode> */
      } else if (0 == strcmp ("TRACK", psz_keyword)) {
	int i_track;

	if (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	  if (1!=sscanf(psz_field, "%d", &i_track)) {
	    cdio_log(log_level, 
		     "%s line %d after word TRACK:",
		     psz_cue_name, i_line);
	    cdio_log(log_level, 
		     "Expecting a track number, got %s", psz_field);
	    goto err_exit;
	  }
	}
	if (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	  track_info_t  *this_track=NULL;

	  if (cd) {
	    this_track = &(cd->tocent[cd->gen.i_tracks]);
	    this_track->track_num   = cd->gen.i_tracks;
	    this_track->num_indices = 0;
	    b_first_index_for_track = false;
	    cdtext_init (&(cd->gen.cdtext_track[cd->gen.i_tracks]));
	    cd->gen.i_tracks++;
	  }
	  i++;
	  
	  if (0 == strcmp ("AUDIO", psz_field)) {
	    if (cd) {
	      this_track->mode           = AUDIO;
	      this_track->blocksize      = CDIO_CD_FRAMESIZE_RAW;
	      this_track->datasize       = CDIO_CD_FRAMESIZE_RAW;
	      this_track->datastart      = 0;
	      this_track->endsize        = 0;
	      this_track->track_format   = TRACK_FORMAT_AUDIO;
	      this_track->track_green    = false;
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_DA;
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DATA:
	      case CDIO_DISC_MODE_CD_XA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else if (0 == strcmp ("MODE1/2048", psz_field)) {
	    if (cd) {
	      this_track->mode        = MODE1;
	      this_track->blocksize   = 2048;
	      this_track->track_format= TRACK_FORMAT_DATA;
	      this_track->track_green = false;
	      /* Is the below correct? */
	      this_track->datastart   = 0;         
	      this_track->datasize    = CDIO_CD_FRAMESIZE;
	      this_track->endsize     = 0;  
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_DATA;
		break;
	      case CDIO_DISC_MODE_CD_DATA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_XA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else if (0 == strcmp ("MODE1/2352", psz_field)) {
	    if (cd) {
	      this_track->blocksize   = 2352;
	      this_track->track_format= TRACK_FORMAT_DATA;
	      this_track->track_green = false;
	      this_track->datastart   = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE;
	      this_track->datasize    = CDIO_CD_FRAMESIZE; 
	      this_track->endsize     = CDIO_CD_EDC_SIZE 
		+ CDIO_CD_M1F1_ZERO_SIZE + CDIO_CD_ECC_SIZE;
	      this_track->mode        = MODE1_RAW; 
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_DATA;
		break;
	      case CDIO_DISC_MODE_CD_DATA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_XA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else if (0 == strcmp ("MODE2/2336", psz_field)) {
	    if (cd) {
	      this_track->blocksize   = 2336;
	      this_track->track_format= TRACK_FORMAT_XA;
	      this_track->track_green = true;
	      this_track->mode        = MODE2;
	      this_track->datastart   = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE;
	      this_track->datasize    = M2RAW_SECTOR_SIZE;  
	      this_track->endsize     = 0;
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_DATA;
		break;
	      case CDIO_DISC_MODE_CD_DATA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_XA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else if (0 == strcmp ("MODE2/2048", psz_field)) {
	    if (cd) {
	      this_track->blocksize   = 2048;
	      this_track->track_format= TRACK_FORMAT_XA;
	      this_track->track_green = true;
	      this_track->mode        = MODE2_FORM1;
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_XA;
		break;
	      case CDIO_DISC_MODE_CD_XA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_DATA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else if (0 == strcmp ("MODE2/2324", psz_field)) {
	    if (cd) {
	      this_track->blocksize   = 2324;
	      this_track->track_format= TRACK_FORMAT_XA;
	      this_track->track_green = true;
	      this_track->mode        = MODE2_FORM2;
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_XA;
		break;
	      case CDIO_DISC_MODE_CD_XA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_DATA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else if (0 == strcmp ("MODE2/2336", psz_field)) {
	    if (cd) {
	      this_track->blocksize   = 2336;
	      this_track->track_format= TRACK_FORMAT_XA;
	      this_track->track_green = true;
	      this_track->mode        = MODE2_FORM_MIX;
	      this_track->datastart   = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE;
	      this_track->datasize    = M2RAW_SECTOR_SIZE;  
	      this_track->endsize     = 0;
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_XA;
		break;
	      case CDIO_DISC_MODE_CD_XA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_DATA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else if (0 == strcmp ("MODE2/2352", psz_field)) {
	    if (cd) {
	      this_track->blocksize   = 2352;
	      this_track->track_format= TRACK_FORMAT_XA;
	      this_track->track_green = true;
	      this_track->mode        = MODE2_RAW;
	      this_track->datastart   = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE;
	      this_track->datasize    = CDIO_CD_FRAMESIZE;
	      this_track->endsize     = CDIO_CD_SYNC_SIZE + CDIO_CD_ECC_SIZE;
	      switch(cd->disc_mode) {
	      case CDIO_DISC_MODE_NO_INFO:
		cd->disc_mode = CDIO_DISC_MODE_CD_XA;
		break;
	      case CDIO_DISC_MODE_CD_XA:
	      case CDIO_DISC_MODE_CD_MIXED:
	      case CDIO_DISC_MODE_ERROR:
		/* Disc type stays the same. */
		break;
	      case CDIO_DISC_MODE_CD_DA:
	      case CDIO_DISC_MODE_CD_DATA:
		cd->disc_mode = CDIO_DISC_MODE_CD_MIXED;
		break;
	      default:
		cd->disc_mode = CDIO_DISC_MODE_ERROR;
	      }
	    }
	  } else {
	    cdio_log(log_level, 
		     "%s line %d after word TRACK:",
		     psz_cue_name, i_line);
	    cdio_log(log_level, 
		     "Unknown track mode %s", psz_field);
	    goto err_exit;
	  }
	} else {
	  goto format_error;
	}
	
	/* FLAGS flag1 flag2 ... */
      } else if (0 == strcmp ("FLAGS", psz_keyword)) {
	if (0 <= i) {
	  while (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	    if (0 == strcmp ("PRE", psz_field)) {
	      if (cd) cd->tocent[i].flags |= PRE_EMPHASIS;
	    } else if (0 == strcmp ("DCP", psz_field)) {
	      if (cd) cd->tocent[i].flags |= COPY_PERMITTED;
	    } else if (0 == strcmp ("4CH", psz_field)) {
	      if (cd) cd->tocent[i].flags |= FOUR_CHANNEL_AUDIO;
	    } else if (0 == strcmp ("SCMS", psz_field)) {
	      if (cd) cd->tocent[i].flags |= SCMS;
	    } else {
	      goto format_error;
	    }
	  }
	} else {
	  goto format_error;
	}
	
	/* ISRC CCOOOYYSSSSS */
      } else if (0 == strcmp ("ISRC", psz_keyword)) {
	if (0 <= i) {
	  if (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	    if (cd) cd->tocent[i].isrc = strdup (psz_field);
	  } else {
	    goto format_error;
	  }
	} else {
	  goto in_global_section;
	}
	
	/* PREGAP MM:SS:FF */
      } else if (0 == strcmp ("PREGAP", psz_keyword)) {
	if (0 <= i) {
	  if (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	    lba_t lba = cdio_lsn_to_lba(cdio_mmssff_to_lba (psz_field));
	    if (CDIO_INVALID_LBA == lba) {
	      cdio_log(log_level, "%s line %d: after word PREGAP:", 
		       psz_cue_name, i_line);
	      cdio_log(log_level, "Invalid MSF string %s", 
		       psz_field);
	      goto err_exit;
	    }
	    if (cd) {
	      cd->tocent[i].pregap = lba;
	    }
	  } else {
	    goto format_error;
	  } if (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	    goto format_error;
	  }
	} else {
	  goto in_global_section;
	}
	
	/* INDEX [##] MM:SS:FF */
      } else if (0 == strcmp ("INDEX", psz_keyword)) {
	if (0 <= i) {
	  if (NULL != (psz_field = strtok (NULL, " \t\n\r")))
	    if (1!=sscanf(psz_field, "%d", &start_index)) {
	      cdio_log(log_level, 
		       "%s line %d after word INDEX:",
		       psz_cue_name, i_line);
	      cdio_log(log_level, 
		       "expecting an index number, got %s", 
		       psz_field);
	      goto err_exit;
	    }
	  if (NULL != (psz_field = strtok (NULL, " \t\n\r"))) {
	    lba_t lba = cdio_mmssff_to_lba (psz_field);
	    if (CDIO_INVALID_LBA == lba) {
	      cdio_log(log_level, "%s line %d: after word INDEX:", 
		       psz_cue_name, i_line);
	      cdio_log(log_level, "Invalid MSF string %s", 
		       psz_field);
	      goto err_exit;
	    }
	    if (cd) {
#if FIXED_ME
	      cd->tocent[i].indexes[cd->tocent[i].nindex++] = lba;
#else     
	      track_info_t  *this_track=
		&(cd->tocent[cd->gen.i_tracks - cd->gen.i_first_track]);

	      if (start_index != 0) {
		if (!b_first_index_for_track) {
		  lba += CDIO_PREGAP_SECTORS;
		  cdio_lba_to_msf(lba, &(this_track->start_msf));
		  b_first_index_for_track = true;
		  this_track->start_lba   = lba;
		}
		
		if (cd->gen.i_tracks > 1) {
		  /* Figure out number of sectors for previous track */
		  track_info_t *prev_track=&(cd->tocent[cd->gen.i_tracks-2]);
		  if ( this_track->start_lba < prev_track->start_lba ) {
		    cdio_log (log_level,
			      "track %d at LBA %lu starts before track %d at LBA %lu", 
			      cd->gen.i_tracks,   
			      (unsigned long int) this_track->start_lba, 
			      cd->gen.i_tracks, 
			      (unsigned long int) prev_track->start_lba);
		    prev_track->sec_count = 0;
		  } else if ( this_track->start_lba >= prev_track->start_lba 
			      + CDIO_PREGAP_SECTORS ) {
		    prev_track->sec_count = this_track->start_lba - 
		      prev_track->start_lba - CDIO_PREGAP_SECTORS ;
		  } else {
		    cdio_log (log_level, 
			      "%lu fewer than pregap (%d) sectors in track %d",
			      (long unsigned int) 
			      this_track->start_lba - prev_track->start_lba,
			      CDIO_PREGAP_SECTORS,
			      cd->gen.i_tracks);
		    /* Include pregap portion in sec_count. Maybe the pregap
		       was omitted. */
		    prev_track->sec_count = this_track->start_lba - 
		      prev_track->start_lba;
		  }
		}
		this_track->num_indices++;
	      }
	    }
#endif  
	  } else {
	    goto format_error;
	  }
	} else {
	  goto in_global_section;
	}
	
	/* CD-Text */
      } else if ( CDTEXT_INVALID != 
		  (cdtext_key = cdtext_is_keyword (psz_keyword)) ) {
	if (-1 == i) {
	  if (cd) {
	    cdtext_set (cdtext_key, 
			strtok (NULL, "\"\t\n\r"), 
			&(cd->gen.cdtext));
	  }
	} else {
	  if (cd) {
	    cdtext_set (cdtext_key, strtok (NULL, "\"\t\n\r"), 
			&(cd->gen.cdtext_track[i]));
	  }
	}
	
	/* unrecognized line */
      } else {
	cdio_log(log_level, "%s line %d: warning: unrecognized keyword: %s", 
		 psz_cue_name, i_line, psz_keyword);
	goto err_exit;
      }
    }
  }

  if (NULL != cd) {
    cd->gen.toc_init = true;
  }

  fclose (fp);
  return true;

 format_error:
  cdio_log(log_level, "%s line %d after word %s", 
	   psz_cue_name, i_line, psz_keyword);
  goto err_exit;

 in_global_section:
  cdio_log(log_level, "%s line %d: word %s not allowed in global section", 
	   psz_cue_name, i_line, psz_keyword);
  goto err_exit;

 not_in_global_section:
  cdio_log(log_level, "%s line %d: word %s only allowed in global section", 
	   psz_cue_name, i_line, psz_keyword);

 err_exit: 
  fclose (fp);
  return false;

}

/*!
   Reads a single audio sector from CD device into data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
_read_audio_sectors_bincue (void *p_user_data, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  int ret;

  /* Why the adjustment of 272, I don't know. It seems to work though */
  if (lsn != 0) {
    ret = cdio_stream_seek (p_env->gen.data_source, 
			    (lsn * CDIO_CD_FRAMESIZE_RAW) - 272, SEEK_SET);
    if (ret!=0) return ret;

    ret = cdio_stream_read (p_env->gen.data_source, data, 
			    CDIO_CD_FRAMESIZE_RAW, nblocks);
  } else {
    /* We need to pad out the first 272 bytes with 0's */
    BZERO(data, 272);
    
    ret = cdio_stream_seek (p_env->gen.data_source, 0, SEEK_SET);

    if (ret!=0) return ret;

    ret = cdio_stream_read (p_env->gen.data_source, (uint8_t *) data+272, 
			    CDIO_CD_FRAMESIZE_RAW - 272, nblocks);
  }

  /* ret is number of bytes if okay, but we need to return 0 okay. */
  return ret == 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sector_bincue (void *p_user_data, void *data, lsn_t lsn, 
			   bool b_form2)
{
  _img_private_t *p_env = p_user_data;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int blocksize = CDIO_CD_FRAMESIZE_RAW;

  ret = cdio_stream_seek (p_env->gen.data_source, lsn * blocksize, SEEK_SET);
  if (ret!=0) return ret;

  /* FIXME: Not completely sure the below is correct. */
  ret = cdio_stream_read (p_env->gen.data_source, buf, CDIO_CD_FRAMESIZE_RAW, 1);
  if (ret==0) return ret;

  memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);

  return DRIVER_OP_SUCCESS;
}

/*!
   Reads nblocks of mode1 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode1_sectors_bincue (void *p_user_data, void *data, lsn_t lsn, 
			    bool b_form2, unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode1_sector_bincue (p_env, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return DRIVER_OP_SUCCESS;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sector_bincue (void *p_user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *p_env = p_user_data;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };

  /* NOTE: The logic below seems a bit wrong and convoluted
     to me, but passes the regression tests. (Perhaps it is why we get
     valgrind errors in vcdxrip). Leave it the way it was for now.
     Review this sector 2336 stuff later.
  */

  int blocksize = CDIO_CD_FRAMESIZE_RAW;

  ret = cdio_stream_seek (p_env->gen.data_source, lsn * blocksize, SEEK_SET);
  if (ret!=0) return ret;

  ret = cdio_stream_read (p_env->gen.data_source, buf, CDIO_CD_FRAMESIZE_RAW, 1);
  if (ret==0) return ret;


  /* See NOTE above. */
  if (b_form2)
    memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	    M2RAW_SECTOR_SIZE);
  else
    memcpy (data, buf + CDIO_CD_XA_SYNC_HEADER, CDIO_CD_FRAMESIZE);

  return DRIVER_OP_SUCCESS;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sectors_bincue (void *p_user_data, void *data, lsn_t lsn, 
			    bool b_form2, unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode2_sector_bincue (p_env, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*!
  Return an array of strings giving possible BIN/CUE disk images.
 */
char **
cdio_get_devices_bincue (void)
{
  char **drives = NULL;
  unsigned int num_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  glob("*.cue", GLOB_DOOFFS, NULL, &globbuf);
  for (i=0; i<globbuf.gl_pathc; i++) {
    cdio_add_device_list(&drives, globbuf.gl_pathv[i], &num_files);
  }
  globfree(&globbuf);
#else
  cdio_add_device_list(&drives, DEFAULT_CDIO_DEVICE, &num_files);
#endif /*HAVE_GLOB_H*/
  cdio_add_device_list(&drives, NULL, &num_files);
  return drives;
}

/*!
  Return a string containing the default CD device.
 */
char *
cdio_get_default_device_bincue(void)
{
  char **drives = cdio_get_devices_bincue();
  char *drive = (drives[0] == NULL) ? NULL : strdup(drives[0]);
  cdio_free_device_list(drives);
  return drive;
}

static bool
get_hwinfo_bincue ( const CdIo_t *p_cdio, /*out*/ cdio_hwinfo_t *hw_info)
{
  strncpy(hw_info->psz_vendor, "libcdio",
	  sizeof(hw_info->psz_vendor)-1);
  hw_info->psz_vendor[sizeof(hw_info->psz_vendor)-1] = '\0';
  strncpy(hw_info->psz_model, "CDRWIN",
	 sizeof(hw_info->psz_model)-1);
  hw_info->psz_model[sizeof(hw_info->psz_model)-1] = '\0';
  strncpy(hw_info->psz_revision, CDIO_VERSION, 
	  sizeof(hw_info->psz_revision)-1);
  hw_info->psz_revision[sizeof(hw_info->psz_revision)-1] = '\0';
  return true;
  
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_format_t
_get_track_format_bincue(void *p_user_data, track_t i_track) 
{
  const _img_private_t *p_env = p_user_data;
  
  if (!p_env->gen.init) return TRACK_FORMAT_ERROR;

  if (i_track > p_env->gen.i_tracks || i_track == 0) 
    return TRACK_FORMAT_ERROR;

  return p_env->tocent[i_track-p_env->gen.i_first_track].track_format;
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_get_track_green_bincue(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if ( NULL == p_env || 
       ( i_track < p_env->gen.i_first_track
	 || i_track >= p_env->gen.i_tracks + p_env->gen.i_first_track ) )
    return false;

  return p_env->tocent[i_track-p_env->gen.i_first_track].track_green;
}

/*!  
  Return the starting LSN track number
  i_track in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static lba_t
_get_lba_track_bincue(void *p_user_data, track_t i_track)
{
  _img_private_t *p_env = p_user_data;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = p_env->gen.i_tracks+1;

  if (i_track <= p_env->gen.i_tracks + p_env->gen.i_first_track 
      && i_track != 0) {
    return p_env->tocent[i_track-p_env->gen.i_first_track].start_lba;
  } else 
    return CDIO_INVALID_LBA;
}

/*! 
  Return corresponding BIN file if psz_cue_name is a cue file or NULL
  if not a CUE file.
*/
char *
cdio_is_cuefile(const char *psz_cue_name) 
{
  int   i;
  char *psz_bin_name;
  
  if (psz_cue_name == NULL) return NULL;

  /* FIXME? Now that we have cue parsing, should we really force
     the filename extension requirement or is it enough just to 
     parse the cuefile? 
   */

  psz_bin_name=strdup(psz_cue_name);
  i=strlen(psz_bin_name)-strlen("cue");
  
  if (i>0) {
    if (psz_cue_name[i]=='c' && psz_cue_name[i+1]=='u' && psz_cue_name[i+2]=='e') {
      psz_bin_name[i++]='b'; psz_bin_name[i++]='i'; psz_bin_name[i++]='n';
      if (parse_cuefile(NULL, psz_cue_name))
	return psz_bin_name;
      else 
	goto error;
    } 
    else if (psz_cue_name[i]=='C' && psz_cue_name[i+1]=='U' && psz_cue_name[i+2]=='E') {
      psz_bin_name[i++]='B'; psz_bin_name[i++]='I'; psz_bin_name[i++]='N';
      if (parse_cuefile(NULL, psz_cue_name))
	return psz_bin_name;
      else 
	goto error;
    }
  }
 error:
  free(psz_bin_name);
  return NULL;
}

/*! 
  Return corresponding CUE file if psz_bin_name is a bin file or NULL
  if not a BIN file.
*/
char *
cdio_is_binfile(const char *psz_bin_name) 
{
  int   i;
  char *psz_cue_name;
  
  if (psz_bin_name == NULL) return NULL;

  psz_cue_name=strdup(psz_bin_name);
  i=strlen(psz_bin_name)-strlen("bin");
  
  if (i>0) {
    if (psz_bin_name[i]=='b' && psz_bin_name[i+1]=='i' && psz_bin_name[i+2]=='n') {
      psz_cue_name[i++]='c'; psz_cue_name[i++]='u'; psz_cue_name[i++]='e';
      return psz_cue_name;
    } 
    else if (psz_bin_name[i]=='B' && psz_bin_name[i+1]=='I' && psz_bin_name[i+2]=='N') {
      psz_cue_name[i++]='C'; psz_cue_name[i++]='U'; psz_cue_name[i++]='E';
      return psz_cue_name;
    }
  }
  free(psz_cue_name);
  return NULL;
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_am_bincue (const char *psz_source_name, const char *psz_access_mode)
{
  if (psz_access_mode != NULL)
    cdio_warn ("there is only one access mode for bincue. Arg %s ignored",
	       psz_access_mode);
  return cdio_open_bincue(psz_source_name);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_bincue (const char *psz_source)
{
  char *psz_bin_name = cdio_is_cuefile(psz_source);

  if (NULL != psz_bin_name) {
    free(psz_bin_name);
    return cdio_open_cue(psz_source);
  } else {
    char *psz_cue_name = cdio_is_binfile(psz_source);
    CdIo_t *cdio = cdio_open_cue(psz_cue_name);
    free(psz_cue_name);
    return cdio;
  }
}

CdIo_t *
cdio_open_cue (const char *psz_cue_name)
{
  CdIo_t *ret;
  _img_private_t *p_data;
  char *psz_bin_name;
  
  cdio_funcs_t _funcs;

  memset( &_funcs, 0, sizeof(_funcs) );
  
  _funcs.eject_media           = _eject_media_image;
  _funcs.free                  = _free_image;
  _funcs.get_arg               = _get_arg_image;
  _funcs.get_cdtext            = get_cdtext_generic;
  _funcs.get_devices           = cdio_get_devices_bincue;
  _funcs.get_default_device    = cdio_get_default_device_bincue;
  _funcs.get_disc_last_lsn     = get_disc_last_lsn_bincue;
  _funcs.get_discmode          = _get_discmode_image;
  _funcs.get_drive_cap         = _get_drive_cap_image;
  _funcs.get_first_track_num   = _get_first_track_num_image;
  _funcs.get_hwinfo            = get_hwinfo_bincue;
  _funcs.get_media_changed     = get_media_changed_image;
  _funcs.get_mcn               = _get_mcn_image;
  _funcs.get_num_tracks        = _get_num_tracks_image;
  _funcs.get_track_channels    = get_track_channels_image,
  _funcs.get_track_copy_permit = get_track_copy_permit_image,
  _funcs.get_track_format      = _get_track_format_bincue;
  _funcs.get_track_green       = _get_track_green_bincue;
  _funcs.get_track_lba         = _get_lba_track_bincue;
  _funcs.get_track_msf         = _get_track_msf_image;
  _funcs.get_track_preemphasis = get_track_preemphasis_image,
  _funcs.lseek                 = _lseek_bincue;
  _funcs.read                  = _read_bincue;
  _funcs.read_audio_sectors    = _read_audio_sectors_bincue;
  _funcs.read_data_sectors     = read_data_sectors_image;
  _funcs.read_mode1_sector     = _read_mode1_sector_bincue;
  _funcs.read_mode1_sectors    = _read_mode1_sectors_bincue;
  _funcs.read_mode2_sector     = _read_mode2_sector_bincue;
  _funcs.read_mode2_sectors    = _read_mode2_sectors_bincue;
  _funcs.run_mmc_cmd           =  NULL;
  _funcs.set_arg               = _set_arg_image;
  _funcs.set_speed             = cdio_generic_unimplemented_set_speed;
  _funcs.set_blocksize         = cdio_generic_unimplemented_set_blocksize;
  
  if (NULL == psz_cue_name) return NULL;
  
  p_data                 = calloc(1, sizeof (_img_private_t));
  p_data->gen.init       = false;
  p_data->psz_cue_name   = NULL;
  
  ret = cdio_new ((void *)p_data, &_funcs);
  
  if (ret == NULL) {
    free(p_data);
    return NULL;
  }
  
  ret->driver_id = DRIVER_BINCUE;
  psz_bin_name = cdio_is_cuefile(psz_cue_name);
  
  if (NULL == psz_bin_name) {
    cdio_error ("source name %s is not recognized as a CUE file", 
		psz_cue_name);
  }
  
  _set_arg_image (p_data, "cue", psz_cue_name);
  _set_arg_image (p_data, "source", psz_bin_name);
  _set_arg_image (p_data, "access-mode", "bincue");
  free(psz_bin_name);
  
  if (_init_bincue(p_data)) {
    return ret;
  } else {
    _free_image(p_data);
    free(ret);
    return NULL;
  }
}

bool
cdio_have_bincue (void)
{
  return true;
}
