/*
    $Id: nrg.c,v 1.9 2005/01/24 00:06:31 rocky Exp $

    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 2001, 2003 Herbert Valerio Riedel <hvr@gnu.org>

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
/*! This code implements low-level access functions for the Nero native
   CD-image format residing inside a disk file (*.nrg).
*/

#include "image.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include <cdio/bytesex.h>
#include <cdio/ds.h>
#include <cdio/logging.h>
#include <cdio/util.h>
#include <cdio/version.h>
#include "cdio_assert.h"
#include "_cdio_stdio.h"
#include "nrg.h"

static const char _rcsid[] = "$Id: nrg.c,v 1.9 2005/01/24 00:06:31 rocky Exp $";


/* reader */

#define DEFAULT_CDIO_DEVICE "image.nrg"

/* 
   Link element of track structure as a linked list.
   Possibly redundant with above track_info_t */
typedef struct {
  uint32_t start_lsn;
  uint32_t sec_count;     /* Number of sectors in track. Does not 
			     include pregap before next entry. */
  uint64_t img_offset;    /* Bytes offset from beginning of disk image file.*/
  uint32_t blocksize;     /* Number of bytes in a block */
} _mapping_t;


#define NEED_NERO_STRUCT
#include "image_common.h"

static bool  parse_nrg (_img_private_t *env, const char *psz_cue_name);
static lsn_t get_disc_last_lsn_nrg (void *p_user_data);

/* Updates internal track TOC, so we can later 
   simulate ioctl(CDROMREADTOCENTRY).
 */
static void
_register_mapping (_img_private_t *env, lsn_t start_lsn, uint32_t sec_count,
		   uint64_t img_offset, uint32_t blocksize,
		   track_format_t track_format, bool track_green)
{
  const int track_num=env->gen.i_tracks;
  track_info_t  *this_track=&(env->tocent[env->gen.i_tracks]);
  _mapping_t *_map = _cdio_malloc (sizeof (_mapping_t));

  _map->start_lsn  = start_lsn;
  _map->sec_count  = sec_count;
  _map->img_offset = img_offset;
  _map->blocksize  = blocksize;

  if (!env->mapping) env->mapping = _cdio_list_new ();
  _cdio_list_append (env->mapping, _map);

  env->size = MAX (env->size, (start_lsn + sec_count));

  /* Update *this_track and track_num. These structures are
     in a sense redundant witht the obj->mapping list. Perhaps one
     or the other can be eliminated.
   */

  cdio_lba_to_msf (cdio_lsn_to_lba(start_lsn), &(this_track->start_msf));
  this_track->start_lba = cdio_msf_to_lba(&this_track->start_msf);
  this_track->track_num = track_num+1;
  this_track->blocksize = blocksize;
  if (env->is_cues) 
    this_track->datastart = img_offset;
  else 
    this_track->datastart = 0;

  if (track_green) 
    this_track->datastart += CDIO_CD_SUBHEADER_SIZE;
      
  this_track->sec_count = sec_count;

  this_track->track_format= track_format;
  this_track->track_green = track_green;

  switch (this_track->track_format) {
  case TRACK_FORMAT_AUDIO:
    this_track->blocksize   = CDIO_CD_FRAMESIZE_RAW;
    this_track->datasize    = CDIO_CD_FRAMESIZE_RAW;
    /*this_track->datastart   = 0;*/
    this_track->endsize     = 0;
    break;
  case TRACK_FORMAT_CDI:
    this_track->datasize=CDIO_CD_FRAMESIZE;
    break;
  case TRACK_FORMAT_XA:
    if (track_green) {
      this_track->blocksize = CDIO_CD_FRAMESIZE;
      /*this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE;*/
      this_track->datasize  = M2RAW_SECTOR_SIZE;
      this_track->endsize   = 0;
    } else {
      /*this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE +
	CDIO_CD_SUBHEADER_SIZE;*/
      this_track->datasize  = CDIO_CD_FRAMESIZE;
      this_track->endsize   = CDIO_CD_SYNC_SIZE + CDIO_CD_ECC_SIZE;
    }
    break;
  case TRACK_FORMAT_DATA:
    if (track_green) {
      /*this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE;*/
      this_track->datasize  = CDIO_CD_FRAMESIZE;
      this_track->endsize   = CDIO_CD_EDC_SIZE + CDIO_CD_M1F1_ZERO_SIZE 
	  + CDIO_CD_ECC_SIZE;
    } else {
      /* Is the below correct? */
      /*this_track->datastart = 0;*/
      this_track->datasize  = CDIO_CD_FRAMESIZE;
      this_track->endsize   = 0;  
    }
    break;
  default:
    /*this_track->datasize=CDIO_CD_FRAMESIZE_RAW;*/
    cdio_warn ("track %d has unknown format %d",
	       env->gen.i_tracks, this_track->track_format);
  }
  
  env->gen.i_tracks++;

  cdio_debug ("start lsn: %lu sector count: %0lu -> %8ld (%08lx)", 
	      (long unsigned int) start_lsn, 
	      (long unsigned int) sec_count, 
	      (long unsigned int) img_offset,
	      (long unsigned int) img_offset);
}


/* 
   Disk and track information for a Nero file are located at the end
   of the file. This routine extracts that information.

   FIXME: right now psz_nrg_name is not used. It will be in the future.
 */
static bool
parse_nrg (_img_private_t *p_env, const char *psz_nrg_name)
{
  long unsigned int footer_start;
  long unsigned int size;
  char *footer_buf = NULL;
  cdio_log_level_t log_level = (NULL == p_env) ? CDIO_LOG_INFO : CDIO_LOG_WARN;

  size = cdio_stream_stat (p_env->gen.data_source);
  if (-1 == size) return false;

  {
    _footer_t buf;
    cdio_assert (sizeof (buf) == 12);
 
    cdio_stream_seek (p_env->gen.data_source, size - sizeof (buf), SEEK_SET);
    cdio_stream_read (p_env->gen.data_source, (void *) &buf, sizeof (buf), 1);
    
    if (buf.v50.ID == UINT32_TO_BE (NERO_ID)) {
      cdio_info ("detected Nero version 5.0 (32-bit offsets) NRG magic");
      footer_start = uint32_to_be (buf.v50.footer_ofs); 
    } else if (buf.v55.ID == UINT32_TO_BE (NER5_ID)) {
      cdio_info ("detected Nero version 5.5.x (64-bit offsets) NRG magic");
      footer_start = uint64_from_be (buf.v55.footer_ofs);
    } else {
      cdio_log (log_level, "Image not recognized as either version 5.0 or "
		"version 5.5.x-6.x type NRG");
      return false;
    }

    cdio_debug (".NRG footer start = %ld, length = %ld", 
	       (long) footer_start, (long) (size - footer_start));

    cdio_assert (IN ((size - footer_start), 0, 4096));

    footer_buf = _cdio_malloc (size - footer_start);

    cdio_stream_seek (p_env->gen.data_source, footer_start, SEEK_SET);
    cdio_stream_read (p_env->gen.data_source, footer_buf, 
		      size - footer_start, 1);
  }
  {
    int pos = 0;

    while (pos < size - footer_start) {
      _chunk_t *chunk = (void *) (footer_buf + pos);
      uint32_t opcode = UINT32_FROM_BE (chunk->id);
      
      bool break_out = false;
      
      switch (opcode) {

      case CUES_ID: /* "CUES" Seems to have sector size 2336 and 150 sector
		       pregap seems to be included at beginning of image.
		       */
      case CUEX_ID: /* "CUEX" */ 
	{
	  unsigned entries = UINT32_FROM_BE (chunk->len);
	  _cuex_array_t *_entries = (void *) chunk->data;
	  
	  cdio_assert (p_env->mapping == NULL);
	  
	  cdio_assert ( sizeof (_cuex_array_t) == 8 );
	  cdio_assert ( UINT32_FROM_BE (chunk->len) % sizeof(_cuex_array_t) 
			== 0 );
	  
	  entries /= sizeof (_cuex_array_t);
	  
	  if (CUES_ID == opcode) {
	    lsn_t lsn = UINT32_FROM_BE (_entries[0].lsn);
	    unsigned int idx;
	    unsigned int i = 0;
	    
	    cdio_info ("CUES type image detected" );

	    /* CUES LSN has 150 pregap include at beginning? -/
	       cdio_assert (lsn == 0?);
	    */
	    
	    p_env->is_cues           = true; /* HACK alert. */
	    p_env->gen.i_tracks      = 0;
	    p_env->gen.i_first_track = 1;
	    for (idx = 1; idx < entries-1; idx += 2, i++) {
	      lsn_t sec_count;
	      int cdte_format = _entries[idx].addr_ctrl / 16;
	      int cdte_ctrl   = _entries[idx].type >> 4;

	      if ( COPY_PERMITTED & cdte_ctrl ) {
		if (p_env) p_env->tocent[i].flags |= COPY_PERMITTED;
	      } else {
		if (p_env) p_env->tocent[i].flags &= ~COPY_PERMITTED;
	      }
	      
	      if ( PRE_EMPHASIS & cdte_ctrl ) {
		if (p_env) p_env->tocent[i].flags |= PRE_EMPHASIS;
	      } else {
		if (p_env) p_env->tocent[i].flags &= ~PRE_EMPHASIS;
	      }
	      
	      if ( FOUR_CHANNEL_AUDIO & cdte_ctrl ) {
		if (p_env) p_env->tocent[i].flags |= FOUR_CHANNEL_AUDIO;
	      } else {
		if (p_env) p_env->tocent[i].flags &= ~FOUR_CHANNEL_AUDIO;
	      }
	      
	      cdio_assert (_entries[idx].track == _entries[idx + 1].track);
	      
	      /* lsn and sec_count*2 aren't correct, but it comes closer on the
		 single example I have: svcdgs.nrg
		 We are picking up the wrong fields and/or not interpreting
		 them correctly.
	      */

	      switch (cdte_format) {
	      case 0:
		lsn = UINT32_FROM_BE (_entries[idx].lsn);
		break;
	      case 1: 
		{
#if 0
		  msf_t msf = (msf_t) _entries[idx].lsn;
		  lsn = cdio_msf_to_lsn(&msf);
#else
		  lsn = CDIO_INVALID_LSN;
#endif		  
		  cdio_warn ("untested (i.e. probably wrong) CUE MSF code");
		  break;
		}
	      default:
		lsn = CDIO_INVALID_LSN;
		cdio_warn("unknown cdte_format %d", cdte_format);
	      }
	      
	      sec_count = UINT32_FROM_BE (_entries[idx + 1].lsn);
	      
	      _register_mapping (p_env, lsn, sec_count*2, 
				 (lsn+CDIO_PREGAP_SECTORS) * M2RAW_SECTOR_SIZE,
				 M2RAW_SECTOR_SIZE, TRACK_FORMAT_XA, true);
	    }
	  } else {
	    lsn_t lsn = UINT32_FROM_BE (_entries[0].lsn);
	    unsigned int idx;
	    unsigned int i = 0;
	    
	    cdio_info ("CUEX type image detected");

	    /* LSN must start at -150 (LBA 0)? */
	    cdio_assert (lsn == -150); 
	    
	    for (idx = 2; idx < entries; idx += 2, i++) {
	      lsn_t sec_count;
	      int cdte_format = _entries[idx].addr_ctrl >> 4;
	      int cdte_ctrl   = _entries[idx].type >> 4;

	      if ( COPY_PERMITTED & cdte_ctrl ) {
		if (p_env) p_env->tocent[i].flags |= COPY_PERMITTED;
	      } else {
		if (p_env) p_env->tocent[i].flags &= ~COPY_PERMITTED;
	      }
	      
	      if ( PRE_EMPHASIS & cdte_ctrl ) {
		if (p_env) p_env->tocent[i].flags |= PRE_EMPHASIS;
	      } else {
		if (p_env) p_env->tocent[i].flags &= ~PRE_EMPHASIS;
	      }
	      
	      if ( FOUR_CHANNEL_AUDIO & cdte_ctrl ) {
		if (p_env) p_env->tocent[i].flags |= FOUR_CHANNEL_AUDIO;
	      } else {
		if (p_env) p_env->tocent[i].flags &= ~FOUR_CHANNEL_AUDIO;
	      }
	      
	      /* extractnrg.pl has cdte_format for LBA's 0, and
		 for MSF 1. ???

		 FIXME: Should decode as appropriate for cdte_format.
	       */
	      cdio_assert ( cdte_format == 0 || cdte_format == 1 );

	      cdio_assert (_entries[idx].track != _entries[idx + 1].track);
	      
	      lsn       = UINT32_FROM_BE (_entries[idx].lsn);
	      sec_count = UINT32_FROM_BE (_entries[idx + 1].lsn);
	      
	      _register_mapping (p_env, lsn, sec_count - lsn, 
				 (lsn + CDIO_PREGAP_SECTORS)*M2RAW_SECTOR_SIZE,
				 M2RAW_SECTOR_SIZE, TRACK_FORMAT_XA, true);
	    }
	  }
	  break;
	}
	
      case DAOX_ID: /* "DAOX" */ 
      case DAOI_ID: /* "DAOI" */
	{
	  track_format_t track_format;
	  int disc_mode;

	  /* We include an extra 0 byte so these can be used as C strings.*/
	  p_env->psz_mcn    = _cdio_malloc (CDIO_MCN_SIZE+1);

	  if (DAOX_ID == opcode) {
	    _daox_array_t *_entries = (void *) chunk->data;
	    disc_mode    = _entries->_unknown[1];
	    p_env->dtyp  = _entries->_unknown[19];
	    memcpy(p_env->psz_mcn, &(_entries->psz_mcn), CDIO_MCN_SIZE);
	    p_env->psz_mcn[CDIO_MCN_SIZE] = '\0';
	  } else {
	    _daoi_array_t *_entries = (void *) chunk->data;
	    disc_mode    = _entries->_unknown[1];
	    p_env->dtyp  = _entries->_unknown[19];
	    memcpy(p_env->psz_mcn, &(_entries->psz_mcn), CDIO_MCN_SIZE);
	    p_env->psz_mcn[CDIO_MCN_SIZE] = '\0';
	  }

	  p_env->is_dao = true;
	  cdio_debug ("DAO%c tag detected, track format %d, mode %x\n", 
		      opcode==DAOX_ID ? 'X': 'I', p_env->dtyp, disc_mode);
	  switch (p_env->dtyp) {
	  case 0:
	    /* Mode 1 */
	    track_format     = TRACK_FORMAT_DATA;
	    p_env->disc_mode = CDIO_DISC_MODE_CD_DATA;
	    break;
	  case 2:
	    /* Mode 2 form 1 */
	    disc_mode             = 0;
	    track_format     = TRACK_FORMAT_XA;
	    p_env->disc_mode = CDIO_DISC_MODE_CD_XA;
	    break;
	  case 3:
	    /* Mode 2 */
	    track_format     = TRACK_FORMAT_XA;
	    p_env->disc_mode = CDIO_DISC_MODE_CD_XA; /* ?? */
	    break;
	  case 0x6:
	    /* Mode2 form mix */
	    track_format     = TRACK_FORMAT_XA;
	    p_env->disc_mode = CDIO_DISC_MODE_CD_MIXED;
	    break;
	  case 0x20: /* ??? Mode2 form 2, Mode2 raw?? */
	    track_format     = TRACK_FORMAT_XA;
	    p_env->disc_mode = CDIO_DISC_MODE_CD_XA; /* ??. */
	    break;
	  case 0x7:
	    track_format     = TRACK_FORMAT_AUDIO;
	    p_env->disc_mode = CDIO_DISC_MODE_CD_DA;
	    break;
	  default:
	    cdio_log (log_level, "Unknown track format %x\n", 
		      p_env->dtyp);
	    track_format = TRACK_FORMAT_AUDIO;
	  }
	  if (0 == disc_mode) {
	    int i;
	    for (i=0; i<p_env->gen.i_tracks; i++) {
	      cdtext_init (&(p_env->gen.cdtext_track[i]));
	      p_env->tocent[i].track_format= track_format;
	      p_env->tocent[i].datastart   = 0;
	      p_env->tocent[i].track_green = false;
	      if (TRACK_FORMAT_AUDIO == track_format) {
		p_env->tocent[i].blocksize   = CDIO_CD_FRAMESIZE_RAW;
		p_env->tocent[i].datasize    = CDIO_CD_FRAMESIZE_RAW;
		p_env->tocent[i].endsize     = 0;
	      } else {
		p_env->tocent[i].datasize    = CDIO_CD_FRAMESIZE;
		p_env->tocent[i].datastart  =  0;
	      }
	    }
	  } else if (2 == disc_mode) {
	    int i;
	    for (i=0; i<p_env->gen.i_tracks; i++) {
	      cdtext_init (&(p_env->gen.cdtext_track[i]));
	      p_env->tocent[i].track_green = true;
	      p_env->tocent[i].track_format= track_format;
	      p_env->tocent[i].datasize    = CDIO_CD_FRAMESIZE;
	      if (TRACK_FORMAT_XA == track_format) {
		p_env->tocent[i].datastart   = CDIO_CD_SYNC_SIZE 
		  + CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE;
		p_env->tocent[i].endsize     = CDIO_CD_SYNC_SIZE 
		  + CDIO_CD_ECC_SIZE;
	      } else {
		p_env->tocent[i].datastart   = CDIO_CD_SYNC_SIZE 
		  + CDIO_CD_HEADER_SIZE;
		p_env->tocent[i].endsize     = CDIO_CD_EDC_SIZE 
		  + CDIO_CD_M1F1_ZERO_SIZE + CDIO_CD_ECC_SIZE;
	      
	      }
	    }
	  } else if (0x20 == disc_mode) {
	    cdio_debug ("Mixed mode CD?\n");
	  } else {
	    /* Mixed mode CD */
	    cdio_log (log_level, 
		      "Don't know if mode 1, mode 2 or mixed: %x\n", 
		      disc_mode);
	  }
	  break;
	}
      case NERO_ID: 
      case NER5_ID: 
	cdio_error ("unexpected nrg magic ID NER%c detected",
		    opcode==NERO_ID ? 'O': '5');
	free(footer_buf);
	return false;
	break;

      case END1_ID: /* "END!" */
	cdio_debug ("nrg end tag detected");
	break_out = true;
	break;
	
      case ETNF_ID: /* "ETNF" */ {
	unsigned entries = UINT32_FROM_BE (chunk->len);
	_etnf_array_t *_entries = (void *) chunk->data;
	
	cdio_assert (p_env->mapping == NULL);
	
	cdio_assert ( sizeof (_etnf_array_t) == 20 );
	cdio_assert ( UINT32_FROM_BE(chunk->len) % sizeof(_etnf_array_t) 
		      == 0 );
	
	entries /= sizeof (_etnf_array_t);
	
	cdio_info ("SAO type image (ETNF) detected");
	
	{
	  int idx;
	  for (idx = 0; idx < entries; idx++) {
	    uint32_t _len = UINT32_FROM_BE (_entries[idx].length);
	    uint32_t _start = UINT32_FROM_BE (_entries[idx].start_lsn);
	    uint32_t _start2 = UINT32_FROM_BE (_entries[idx].start);
	    uint32_t track_mode= uint32_from_be (_entries[idx].type);
	    bool     track_green = true;
	    track_format_t track_format = TRACK_FORMAT_XA;
	    uint16_t  blocksize;     
	    
	    switch (track_mode) {
	    case 0:
	      /* Mode 1 */
	      track_format   = TRACK_FORMAT_DATA;
	      track_green    = false; /* ?? */
	      blocksize      = CDIO_CD_FRAMESIZE;
	      p_env->disc_mode = CDIO_DISC_MODE_CD_DATA;
	      break;
	    case 2:
	      /* Mode 2 form 1 */
	      track_format   = TRACK_FORMAT_XA;
	      track_green    = false; /* ?? */
	      blocksize      = CDIO_CD_FRAMESIZE;
	      p_env->disc_mode = CDIO_DISC_MODE_CD_XA;
	      break;
	    case 3:
	      /* Mode 2 */
	      track_format   = TRACK_FORMAT_XA;
	      track_green    = true;
	      blocksize      = M2RAW_SECTOR_SIZE;
	      p_env->disc_mode = CDIO_DISC_MODE_CD_XA; /* ?? */
	      break;
	    case 06:
	      /* Mode2 form mix */
	      track_format   = TRACK_FORMAT_XA;
	      track_green    = true;
	      blocksize      = M2RAW_SECTOR_SIZE;
	      p_env->disc_mode = CDIO_DISC_MODE_CD_MIXED;
	      break;
	    case 0x20: /* ??? Mode2 form 2, Mode2 raw?? */
	      track_format   = TRACK_FORMAT_XA;
	      track_green    = true;
	      blocksize      = M2RAW_SECTOR_SIZE;
	      p_env->disc_mode = CDIO_DISC_MODE_CD_XA; /* ??. */
	      break;
	    case 7:
	      track_format   = TRACK_FORMAT_AUDIO;
	      track_green    = false;
	      blocksize      = CDIO_CD_FRAMESIZE_RAW;
	      p_env->disc_mode = CDIO_DISC_MODE_CD_DA;
	      break;
	    default:
	      cdio_log (log_level, 
			"Don't know how to handle track mode (%lu)?",
			(long unsigned int) track_mode);
	      free(footer_buf);
	      return false;
	    }
	    
	    cdio_assert (_len % blocksize == 0);
	    
	    _len /= blocksize;
	    
	    cdio_assert (_start * blocksize == _start2);
	    
	    _start += idx * CDIO_PREGAP_SECTORS;
	    _register_mapping (p_env, _start, _len, _start2, blocksize,
			       track_format, track_green);

	  }
	}
	break;
      }
      
      case ETN2_ID: { /* "ETN2", same as above, but with 64bit stuff instead */
	unsigned entries = uint32_from_be (chunk->len);
	_etn2_array_t *_entries = (void *) chunk->data;
	
	cdio_assert (p_env->mapping == NULL);
	
	cdio_assert (sizeof (_etn2_array_t) == 32);
	cdio_assert (uint32_from_be (chunk->len) % sizeof (_etn2_array_t) == 0);
	
	entries /= sizeof (_etn2_array_t);
	
	cdio_info ("SAO type image (ETN2) detected");

	{
	  int idx;
	  for (idx = 0; idx < entries; idx++) {
	    uint32_t _len = uint64_from_be (_entries[idx].length);
	    uint32_t _start = uint32_from_be (_entries[idx].start_lsn);
	    uint32_t _start2 = uint64_from_be (_entries[idx].start);
	    uint32_t track_mode= uint32_from_be (_entries[idx].type);
	    bool     track_green = true;
	    track_format_t track_format = TRACK_FORMAT_XA;
	    uint16_t  blocksize;     


	    switch (track_mode) {
	    case 0:
	      track_format = TRACK_FORMAT_DATA;
	      track_green  = false; /* ?? */
	      blocksize    = CDIO_CD_FRAMESIZE;
	      break;
	    case 2:
	      track_format = TRACK_FORMAT_XA;
	      track_green  = false; /* ?? */
	      blocksize    = CDIO_CD_FRAMESIZE;
	      break;
	    case 3:
	      track_format = TRACK_FORMAT_XA;
	      track_green  = true;
	      blocksize    = M2RAW_SECTOR_SIZE;
	      break;
	    case 7:
	      track_format = TRACK_FORMAT_AUDIO;
	      track_green  = false;
	      blocksize    = CDIO_CD_FRAMESIZE_RAW;
	      break;
	    default:
	      cdio_log (log_level, 
			"Don't know how to handle track mode (%lu)?",
			(long unsigned int) track_mode);
	      free(footer_buf);
	      return false;
	    }
	    
	    if (_len % blocksize != 0) {
	      cdio_log (log_level, 
			"length is not a multiple of blocksize " 
			 "len %lu, size %d, rem %lu", 
			 (long unsigned int) _len, blocksize, 
			 (long unsigned int) _len % blocksize);
	      if (0 == _len % CDIO_CD_FRAMESIZE) {
		cdio_log(log_level, "Adjusting blocksize to %d", 
			 CDIO_CD_FRAMESIZE);
		blocksize = CDIO_CD_FRAMESIZE;
	      } else if (0 == _len % M2RAW_SECTOR_SIZE) {
		cdio_log(log_level,
			 "Adjusting blocksize to %d", M2RAW_SECTOR_SIZE);
		blocksize = M2RAW_SECTOR_SIZE;
	      } else if (0 == _len % CDIO_CD_FRAMESIZE_RAW) {
		cdio_log(log_level, 
			 "Adjusting blocksize to %d", CDIO_CD_FRAMESIZE_RAW);
		blocksize = CDIO_CD_FRAMESIZE_RAW;
	      }
	    }
	    
	    _len /= blocksize;
	    
	    if (_start * blocksize != _start2) {
	      cdio_log (log_level,
			"%lu * %d != %lu", 
			 (long unsigned int) _start, blocksize, 
			 (long unsigned int) _start2);
	      if (_start * CDIO_CD_FRAMESIZE == _start2) {
		cdio_log(log_level,
			 "Adjusting blocksize to %d", CDIO_CD_FRAMESIZE);
		blocksize = CDIO_CD_FRAMESIZE;
	      } else if (_start * M2RAW_SECTOR_SIZE == _start2) {
		cdio_log(log_level,
			 "Adjusting blocksize to %d", M2RAW_SECTOR_SIZE);
		blocksize = M2RAW_SECTOR_SIZE;
	      } else if (0 == _start * CDIO_CD_FRAMESIZE_RAW == _start2) {
		cdio_log(log_level,
			 "Adjusting blocksize to %d", CDIO_CD_FRAMESIZE_RAW);
		blocksize = CDIO_CD_FRAMESIZE_RAW;
	      }
	    }
	    
	    _start += idx * CDIO_PREGAP_SECTORS;
	    _register_mapping (p_env, _start, _len, _start2, blocksize,
			       track_format, track_green);
	  }
	}
	break;
      }
	
      case SINF_ID: { /* "SINF" */
	
	uint32_t *_sessions = (void *) chunk->data;
	
	cdio_assert (UINT32_FROM_BE (chunk->len) == 4);
	
	cdio_debug ("SINF: %lu sessions", 
		    (long unsigned int) UINT32_FROM_BE (*_sessions));
      }
	break;
	
      case MTYP_ID: { /* "MTYP" */
	uint32_t *mtyp_p = (void *) chunk->data;
	uint32_t mtyp  = UINT32_FROM_BE (*mtyp_p);
	
	cdio_assert (UINT32_FROM_BE (chunk->len) == 4);

	cdio_debug ("MTYP: %lu", 
		    (long unsigned int) UINT32_FROM_BE (*mtyp_p));

	if (mtyp != MTYP_AUDIO_CD) {
	  cdio_log (log_level,
		    "Unknown MTYP value: %u", (unsigned int) mtyp);
	}
	p_env->mtyp = mtyp;
      }
	break;
	
      case CDTX_ID: { /* "CD TEXT" */
	
	cdio_log (log_level,
		  "Don't know how to handle CD TEXT yet" );
	break;
      }

      default:
	cdio_log (log_level,
		  "unknown tag %8.8x seen", 
		  (unsigned int) UINT32_FROM_BE (chunk->id));
	break;
      }
	
      if (break_out)
	break;
      
      pos += 8;
      pos += UINT32_FROM_BE (chunk->len);
    }
  }

  /* Fake out leadout track. */
  /* Don't use get_disc_last_lsn_nrg since that will lead to recursion since
     we haven't fully initialized things yet.
  */
  cdio_lsn_to_msf (p_env->size, &p_env->tocent[p_env->gen.i_tracks].start_msf);
  p_env->tocent[p_env->gen.i_tracks].start_lba = cdio_lsn_to_lba(p_env->size);
  p_env->tocent[p_env->gen.i_tracks-1].sec_count = 
    cdio_lsn_to_lba(p_env->size - p_env->tocent[p_env->gen.i_tracks-1].start_lba);

  p_env->gen.b_cdtext_init  = true;
  p_env->gen.b_cdtext_error = false;
  p_env->gen.toc_init       = true;
  free(footer_buf);
  return true;
}

/*!
  Initialize image structures.
 */
static bool
_init_nrg (_img_private_t *p_env)
{
  if (p_env->gen.init) {
    cdio_error ("init called more than once");
    return false;
  }
  
  if (!(p_env->gen.data_source = cdio_stdio_new (p_env->gen.source_name))) {
    cdio_warn ("can't open nrg image file %s for reading", 
	       p_env->gen.source_name);
    return false;
  }

  p_env->psz_mcn       = NULL;
  p_env->disc_mode     = CDIO_DISC_MODE_NO_INFO;

  cdtext_init (&(p_env->gen.cdtext));

  if ( !parse_nrg (p_env, p_env->gen.source_name) ) {
    cdio_warn ("image file %s is not a Nero image", 
	       p_env->gen.source_name);
    return false;
  }
  
  p_env->gen.init = true;
  return true;

}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Would be libc's seek() but we have to adjust for the extra track header 
  information in each sector.
*/
static off_t
_lseek_nrg (void *p_user_data, off_t offset, int whence)
{
  _img_private_t *p_env = p_user_data;

  /* real_offset is the real byte offset inside the disk image
     The number below was determined empirically. 
  */
  off_t real_offset= p_env->is_dao ? 0x4b000 : 0;

  unsigned int i;

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
    return -1;
  } else
    real_offset += p_env->tocent[i].datastart;
    return cdio_stream_seek(p_env->gen.data_source, real_offset, whence);
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  FIXME: 
   At present we assume a read doesn't cross sector or track
   boundaries.
*/
static ssize_t
_read_nrg (void *p_user_data, void *buf, size_t size)
{
  _img_private_t *p_env = p_user_data;
  return cdio_stream_read(p_env->gen.data_source, buf, size, 1);
}

/*!
  Get the size of the CD in logical block address (LBA) units.
  
  @param p_cdio the CD object queried
  @return the lsn. On error 0 or CDIO_INVALD_LSN.
*/
static lsn_t
get_disc_last_lsn_nrg (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  return p_env->size;
}

/*!
   Reads a single audio sector from CD device into data starting
   from LSN.
 */
static driver_return_code_t
_read_audio_sectors_nrg (void *p_user_data, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;

  CdioListNode_t *node;

  if (lsn >= p_env->size)
    {
      cdio_warn ("trying to read beyond image size (%lu >= %lu)", 
		 (long unsigned int) lsn, (long unsigned int) p_env->size);
      return -1;
    }

  _CDIO_LIST_FOREACH (node, p_env->mapping) {
    _mapping_t *_map = _cdio_list_node_data (node);
    
    if (IN (lsn, _map->start_lsn, (_map->start_lsn + _map->sec_count - 1))) {
      int ret;
      long int img_offset = _map->img_offset;
      
      img_offset += (lsn - _map->start_lsn) * CDIO_CD_FRAMESIZE_RAW;
      
      ret = cdio_stream_seek (p_env->gen.data_source, img_offset, 
			      SEEK_SET); 
      if (ret!=0) return ret;
      ret = cdio_stream_read (p_env->gen.data_source, data, 
			      CDIO_CD_FRAMESIZE_RAW, nblocks);
      if (ret==0) return ret;
      break;
    }
  }

  if (!node) cdio_warn ("reading into pre gap (lsn %lu)", 
			(long unsigned int) lsn);

  return 0;
}

static driver_return_code_t
_read_mode1_sector_nrg (void *p_user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *p_env = p_user_data;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };

  CdioListNode_t *node;

  if (lsn >= p_env->size)
    {
      cdio_warn ("trying to read beyond image size (%lu >= %lu)", 
		 (long unsigned int) lsn, (long unsigned int) p_env->size);
      return -1;
    }

  _CDIO_LIST_FOREACH (node, p_env->mapping) {
    _mapping_t *_map = _cdio_list_node_data (node);
    
    if (IN (lsn, _map->start_lsn, (_map->start_lsn + _map->sec_count - 1))) {
      int ret;
      long int img_offset = _map->img_offset;
      
      img_offset += (lsn - _map->start_lsn) * _map->blocksize;
      
      ret = cdio_stream_seek (p_env->gen.data_source, img_offset, 
			      SEEK_SET); 
      if (ret!=0) return ret;

      /* FIXME: Not completely sure the below is correct. */
      ret = cdio_stream_read (p_env->gen.data_source, 
			      (M2RAW_SECTOR_SIZE == _map->blocksize)
			      ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE)
			      : buf,
			      _map->blocksize, 1); 
      if (ret==0) return ret;
      break;
    }
  }

  if (!node)
    cdio_warn ("reading into pre gap (lsn %lu)", (long unsigned int) lsn);

  memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);

  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
 */
static driver_return_code_t
_read_mode1_sectors_nrg (void *p_user_data, void *data, lsn_t lsn, 
			 bool b_form2, unsigned nblocks)
{
  _img_private_t *p_env = p_user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode1_sector_nrg (p_env, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

static driver_return_code_t
_read_mode2_sector_nrg (void *p_user_data, void *data, lsn_t lsn, 
			bool b_form2)
{
  _img_private_t *p_env = p_user_data;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };

  CdioListNode_t *node;

  if (lsn >= p_env->size)
    {
      cdio_warn ("trying to read beyond image size (%lu >= %lu)", 
		 (long unsigned int) lsn, (long unsigned int) p_env->size);
      return -1;
    }

  _CDIO_LIST_FOREACH (node, p_env->mapping) {
    _mapping_t *_map = _cdio_list_node_data (node);
    
    if (IN (lsn, _map->start_lsn, (_map->start_lsn + _map->sec_count - 1))) {
      int ret;
      long int img_offset = _map->img_offset;
      
      img_offset += (lsn - _map->start_lsn) * _map->blocksize;
      
      ret = cdio_stream_seek (p_env->gen.data_source, img_offset, 
			      SEEK_SET); 
      if (ret!=0) return ret;
      ret = cdio_stream_read (p_env->gen.data_source, 
			      (M2RAW_SECTOR_SIZE == _map->blocksize)
			      ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE)
			      : buf,
			      _map->blocksize, 1); 
      if (ret==0) return ret;
      break;
    }
  }

  if (!node)
    cdio_warn ("reading into pre gap (lsn %lu)", (long unsigned int) lsn);

  if (b_form2)
    memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	    M2RAW_SECTOR_SIZE);
  else
    memcpy (data, buf + CDIO_CD_XA_SYNC_HEADER, CDIO_CD_FRAMESIZE);

  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static driver_return_code_t
_read_mode2_sectors_nrg (void *p_user_data, void *data, lsn_t lsn, 
			 bool b_form2, unsigned nblocks)
{
  _img_private_t *p_env = p_user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode2_sector_nrg (p_env, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*
  Free memory resources associated with NRG object.
*/
static void 
_free_nrg (void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;

  if (NULL == p_env) return;
  if (NULL != p_env->mapping)
    _cdio_list_free (p_env->mapping, true); 

  /* The remaining part of the image is like the other image drivers,
     so free that in the same way. */
  _free_image(p_user_data);
}

/*!
  Eject media -- there's nothing to do here except free resources.
  We always return 2.
 */
static driver_return_code_t
_eject_media_nrg(void *obj)
{
  _free_nrg (obj);
  return DRIVER_OP_UNSUPPORTED;
}

/*!
  Return an array of strings giving possible NRG disk images.
 */
char **
cdio_get_devices_nrg (void)
{
  char **drives = NULL;
  unsigned int num_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  glob("*.nrg", GLOB_DOOFFS, NULL, &globbuf);
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
cdio_get_default_device_nrg(void)
{
  char **drives = cdio_get_devices_nrg();
  char *drive = (drives[0] == NULL) ? NULL : strdup(drives[0]);
  cdio_free_device_list(drives);
  return drive;
}

static bool
get_hwinfo_nrg ( const CdIo *p_cdio, /*out*/ cdio_hwinfo_t *hw_info)
{
  strcpy(hw_info->psz_vendor, "libcdio");
  strcpy(hw_info->psz_model, "Nero");
  strcpy(hw_info->psz_revision, CDIO_VERSION);
  return true;
  
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_format_t
get_track_format_nrg(void *p_user_data, track_t track_num) 
{
  _img_private_t *p_env = p_user_data;
  
  if (track_num > p_env->gen.i_tracks || track_num == 0) 
    return TRACK_FORMAT_ERROR;

  if ( p_env->dtyp != DTYP_INVALID) {
    switch (p_env->dtyp) {
    case DTYP_MODE2_XA:
      return TRACK_FORMAT_XA;
    case DTYP_MODE1:
      return TRACK_FORMAT_DATA;
    default: ;
    }
  }
    
  /*if ( MTYP_AUDIO_CD == p_env->mtyp) return TRACK_FORMAT_AUDIO; */
  return p_env->tocent[track_num-1].track_format;
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_get_track_green_nrg(void *p_user_data, track_t track_num) 
{
  _img_private_t *p_env = p_user_data;
  
  if (track_num > p_env->gen.i_tracks || track_num == 0) 
    return false;

  if ( MTYP_AUDIO_CD == p_env->mtyp) return false;
  return p_env->tocent[track_num-1].track_green;
}

/*! 
  Check that a NRG file is valid. 

*/
/* Later we'll probably do better. For now though, this gets us 
   started for now.
*/
bool
cdio_is_nrg(const char *psz_nrg) 
{
  unsigned int i;
  
  if (psz_nrg == NULL) return false;

  i=strlen(psz_nrg)-strlen("nrg");
  
  if (i>0) {
    if (psz_nrg[i]=='n' && psz_nrg[i+1]=='r' && psz_nrg[i+2]=='g') {
      return true;
    } 
    else if (psz_nrg[i]=='N' && psz_nrg[i+1]=='R' && psz_nrg[i+2]=='G') {
      return true;
    }
  }
  return false;
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_nrg (const char *psz_source_name, const char *psz_access_mode)
{
  if (psz_access_mode != NULL && strcmp(psz_access_mode, "image"))
    cdio_warn ("there is only one access mode for nrg. Arg %s ignored",
	       psz_access_mode);
  return cdio_open_nrg(psz_source_name);
}


CdIo *
cdio_open_nrg (const char *psz_source)
{
  CdIo *ret;
  _img_private_t *_data;

  cdio_funcs_t _funcs;

  memset( &_funcs, 0, sizeof(_funcs) );

  _funcs.eject_media           = _eject_media_nrg;
  _funcs.free                  = _free_nrg;
  _funcs.get_arg               = _get_arg_image;
  _funcs.get_cdtext            = get_cdtext_generic;
  _funcs.get_devices           = cdio_get_devices_nrg;
  _funcs.get_default_device    = cdio_get_default_device_nrg;
  _funcs.get_disc_last_lsn     = get_disc_last_lsn_nrg;
  _funcs.get_discmode          = _get_discmode_image;
  _funcs.get_drive_cap         = _get_drive_cap_image;
  _funcs.get_first_track_num   = _get_first_track_num_image;
  _funcs.get_hwinfo            = get_hwinfo_nrg;
  _funcs.get_mcn               = _get_mcn_image;
  _funcs.get_num_tracks        = _get_num_tracks_image;
  _funcs.get_track_channels    = get_track_channels_generic,
  _funcs.get_track_copy_permit = get_track_copy_permit_image,
  _funcs.get_track_format      = get_track_format_nrg;
  _funcs.get_track_green       = _get_track_green_nrg;
  _funcs.get_track_lba         = NULL; /* Will use generic routine via msf */
  _funcs.get_track_msf         = _get_track_msf_image;
  _funcs.get_track_preemphasis = get_track_preemphasis_generic,
  _funcs.lseek                 = _lseek_nrg;
  _funcs.read                  = _read_nrg;
  _funcs.read_audio_sectors    = _read_audio_sectors_nrg;
  _funcs.read_mode1_sector     = _read_mode1_sector_nrg;
  _funcs.read_mode1_sectors    = _read_mode1_sectors_nrg;
  _funcs.read_mode2_sector     = _read_mode2_sector_nrg;
  _funcs.read_mode2_sectors    = _read_mode2_sectors_nrg;
  _funcs.set_arg               = _set_arg_image;

  _data                   = _cdio_malloc (sizeof (_img_private_t));
  _data->gen.init         = false;

  _data->gen.i_tracks     = 0;
  _data->mtyp             = 0; 
  _data->dtyp             = DTYP_INVALID; 
  _data->gen.i_first_track= 1;
  _data->is_dao           = false; 
  _data->is_cues          = false; /* FIXME: remove is_cues. */

  ret = cdio_new ((void *)_data, &_funcs);

  if (ret == NULL) {
    free(_data);
    return NULL;
  }

  ret->driver_id = DRIVER_NRG;
  _set_arg_image(_data, "source", (NULL == psz_source) 
	       ? DEFAULT_CDIO_DEVICE: psz_source);
  _set_arg_image (_data, "access-mode", "Nero");

  _data->psz_cue_name   = strdup(_get_arg_image(_data, "source"));

  if (!cdio_is_nrg(_data->psz_cue_name)) {
    cdio_debug ("source name %s is not recognized as a NRG image", 
		_data->psz_cue_name);
    _free_nrg(_data);
    return NULL;
  }

  _set_arg_image (_data, "cue", _data->psz_cue_name);

  if (_init_nrg(_data))
    return ret;
  else {
    _free_nrg(_data);
    free(ret);
    return NULL;
  }

}

bool
cdio_have_nrg (void)
{
  return true;
}
