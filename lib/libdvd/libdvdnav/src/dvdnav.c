/*
 * Copyright (C) 2000 Rich Wareham <richwareham@users.sourceforge.net>
 *
 * This file is part of libdvdnav, a DVD navigation library.
 *
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdnav; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
#define LOG_DEBUG
*/

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include "dvdnav/dvdnav.h"
#include <dvdread/dvd_reader.h>
#include <dvdread/nav_types.h>
#include <dvdread/ifo_types.h> /* For vm_cmd_t */
#include "remap.h"
#include "vm/decoder.h"
#include "vm/vm.h"
#include "dvdnav_internal.h"
#include "read_cache.h"
#include <dvdread/nav_read.h>
#include "remap.h"

static dvdnav_status_t dvdnav_clear(dvdnav_t * this) {
  /* clear everything except file, vm, mutex, readahead */

  pthread_mutex_lock(&this->vm_lock);
  if (this->file) DVDCloseFile(this->file);
  this->file = NULL;

  memset(&this->position_current,0,sizeof(this->position_current));
  memset(&this->pci,0,sizeof(this->pci));
  memset(&this->dsi,0,sizeof(this->dsi));
  this->last_cmd_nav_lbn = SRI_END_OF_CELL;

  /* Set initial values of flags */
  this->skip_still = 0;
  this->sync_wait = 0;
  this->sync_wait_skip = 0;
  this->spu_clut_changed = 0;
  this->started = 0;
  this->cur_cell_time = 0;

  dvdnav_read_cache_clear(this->cache);
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_open(dvdnav_t** dest, const char *path) {
  dvdnav_t *this;
  struct timeval time;

  /* Create a new structure */
  fprintf(MSG_OUT, "libdvdnav: Using dvdnav version %s\n", VERSION);

  (*dest) = NULL;
  this = (dvdnav_t*)malloc(sizeof(dvdnav_t));
  if(!this)
    return DVDNAV_STATUS_ERR;
  memset(this, 0, (sizeof(dvdnav_t) ) ); /* Make sure this structure is clean */

  pthread_mutex_init(&this->vm_lock, NULL);
  /* Initialise the error string */
  printerr("");

  /* Initialise the VM */
  this->vm = vm_new_vm();
  if(!this->vm) {
    printerr("Error initialising the DVD VM.");
    pthread_mutex_destroy(&this->vm_lock);
    free(this);
    return DVDNAV_STATUS_ERR;
  }
  if(!vm_reset(this->vm, path)) {
    printerr("Error starting the VM / opening the DVD device.");
    pthread_mutex_destroy(&this->vm_lock);
    vm_free_vm(this->vm);
    free(this);
    return DVDNAV_STATUS_ERR;
  }

  /* Set the path. FIXME: Is a deep copy 'right' */
  strncpy(this->path, path, MAX_PATH_LEN - 1);
  this->path[MAX_PATH_LEN - 1] = '\0';

  /* Pre-open and close a file so that the CSS-keys are cached. */
  this->file = DVDOpenFile(vm_get_dvd_reader(this->vm), 0, DVD_READ_MENU_VOBS);

  /* Start the read-ahead cache. */
  this->cache = dvdnav_read_cache_new(this);

  /* Seed the random numbers. So that the DVD VM Command rand()
   * gives a different start value each time a DVD is played. */
  gettimeofday(&time, NULL);
  srand(time.tv_usec);

  dvdnav_clear(this);

  (*dest) = this;
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_close(dvdnav_t *this) {

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: close:called\n");
#endif

  if (this->file) {
    pthread_mutex_lock(&this->vm_lock);
    DVDCloseFile(this->file);
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: close:file closing\n");
#endif
    this->file = NULL;
    pthread_mutex_unlock(&this->vm_lock);
  }

  /* Free the VM */
  if(this->vm)
    vm_free_vm(this->vm);

  pthread_mutex_destroy(&this->vm_lock);

  /* We leave the final freeing of the entire structure to the cache,
   * because we don't know, if there are still buffers out in the wild,
   * that must return first. */
  if(this->cache)
    dvdnav_read_cache_free(this->cache);
  else
    free(this);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_reset(dvdnav_t *this) {
  dvdnav_status_t result;

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: reset:called\n");
#endif

  pthread_mutex_lock(&this->vm_lock);

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: reseting vm\n");
#endif
  if(!vm_reset(this->vm, NULL)) {
    printerr("Error restarting the VM.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: clearing dvdnav\n");
#endif
  pthread_mutex_unlock(&this->vm_lock);
  result = dvdnav_clear(this);

  return result;
}

dvdnav_status_t dvdnav_path(dvdnav_t *this, const char** path) {
  (*path) = this->path;

  return DVDNAV_STATUS_OK;
}

const char* dvdnav_err_to_string(dvdnav_t *this) {

  if(!this)
    return "Hey! You gave me a NULL pointer you naughty person!";

  return this->err_str;
}

/* converts a dvd_time_t to PTS ticks */
int64_t dvdnav_convert_time(dvd_time_t *time) {
  int64_t result;
  int64_t frames;

  result  = (time->hour    >> 4  ) * 10 * 60 * 60 * 90000ull;
  result += (time->hour    & 0x0f)      * 60 * 60 * 90000;
  result += (time->minute  >> 4  )      * 10 * 60 * 90000;
  result += (time->minute  & 0x0f)           * 60 * 90000;
  result += (time->second  >> 4  )           * 10 * 90000;
  result += (time->second  & 0x0f)                * 90000;

  frames  = ((time->frame_u & 0x30) >> 4) * 10;
  frames += ((time->frame_u & 0x0f)     )     ;

  if (time->frame_u & 0x80)
    result += frames * 3000;
  else
    result += frames * 3600;

  return result;
}

/*
 * Returns 1 if block contains NAV packet, 0 otherwise.
 * Processes said NAV packet if present.
 *
 * Most of the code in here is copied from xine's MPEG demuxer
 * so any bugs which are found in that should be corrected here also.
 */
static int32_t dvdnav_decode_packet(dvdnav_t *this, uint8_t *p, dsi_t *nav_dsi, pci_t *nav_pci) {
  int32_t        bMpeg1 = 0;
  uint32_t       nHeaderLen;
  uint32_t       nPacketLen;
  uint32_t       nStreamID;

  if (p[3] == 0xBA) { /* program stream pack header */
    int32_t nStuffingBytes;

    bMpeg1 = (p[4] & 0x40) == 0;

    if (bMpeg1) {
      p += 12;
    } else { /* mpeg2 */
      nStuffingBytes = p[0xD] & 0x07;
      p += 14 + nStuffingBytes;
    }
  }

  if (p[3] == 0xbb) { /* program stream system header */
    nHeaderLen = (p[4] << 8) | p[5];
    p += 6 + nHeaderLen;
  }

  /* we should now have a PES packet here */
  if (p[0] || p[1] || (p[2] != 1)) {
    fprintf(MSG_OUT, "libdvdnav: demux error! %02x %02x %02x (should be 0x000001) \n",p[0],p[1],p[2]);
    return 0;
  }

  nPacketLen = p[4] << 8 | p[5];
  nStreamID  = p[3];

  nHeaderLen = 6;
  p += nHeaderLen;

  if (nStreamID == 0xbf) { /* Private stream 2 */
#if 0
    int32_t i;
    fprintf(MSG_OUT, "libdvdnav: nav packet=%u\n",p-p_start-6);
    for(i=0;i<80;i++)
      fprintf(MSG_OUT, "%02x ",p[i-6]);
    fprintf(MSG_OUT, "\n");
#endif

    if(p[0] == 0x00) {
      navRead_PCI(nav_pci, p+1);
    }

    p += nPacketLen;

    /* We should now have a DSI packet. */
    if(p[6] == 0x01) {
      nPacketLen = p[4] << 8 | p[5];
      p += 6;
      navRead_DSI(nav_dsi, p+1);
    }
    return 1;
  }
  return 0;
}

/* DSI is used for most angle stuff.
 * PCI is used for only non-seemless angle stuff
 */
static int32_t dvdnav_get_vobu(dvdnav_t *this, dsi_t *nav_dsi, pci_t *nav_pci, dvdnav_vobu_t *vobu) {
  uint32_t next;
  int32_t angle, num_angle;

  vobu->vobu_start = nav_dsi->dsi_gi.nv_pck_lbn; /* Absolute offset from start of disk */
  vobu->vobu_length = nav_dsi->dsi_gi.vobu_ea; /* Relative offset from vobu_start */

  /*
   * If we're not at the end of this cell, we can determine the next
   * VOBU to display using the VOBU_SRI information section of the
   * DSI.  Using this value correctly follows the current angle,
   * avoiding the doubled scenes in The Matrix, and makes our life
   * really happy.
   *
   * vobu_next is an offset value, 0x3fffffff = SRI_END_OF_CELL
   * DVDs are about 6 Gigs, which is only up to 0x300000 blocks
   * Should really assert if bit 31 != 1
   */

#if 0
  /* Old code -- may still be useful one day */
  if(nav_dsi->vobu_sri.next_vobu != SRI_END_OF_CELL ) {
    vobu->vobu_next = ( nav_dsi->vobu_sri.next_vobu & 0x3fffffff );
  } else {
    vobu->vobu_next = vobu->vobu_length;
  }
#else
  /* Relative offset from vobu_start */
  vobu->vobu_next = ( nav_dsi->vobu_sri.next_vobu & 0x3fffffff );
#endif

  vm_get_angle_info(this->vm, &angle, &num_angle);

  /* FIMXE: The angle reset doesn't work for some reason for the moment */
#if 0
  if((num_angle < angle) && (angle != 1)) {
    fprintf(MSG_OUT, "libdvdnav: angle ends!\n");

    /* This is to switch back to angle one when we
     * finish with angles. */
    dvdnav_angle_change(this, 1);
  }
#endif
  /* only use ILVU information if we are at the last vobunit in ILVU */
  /* otherwise we will miss nav packets from vobunits inbetween */
  if(num_angle != 0 && (nav_dsi->sml_pbi.category & DSI_ILVU_MASK) == (DSI_ILVU_BLOCK | DSI_ILVU_LAST)) {

    if((next = nav_pci->nsml_agli.nsml_agl_dsta[angle-1]) != 0) {
      if((next & 0x3fffffff) != 0) {
	if(next & 0x80000000)
	  vobu->vobu_next = - (int32_t)(next & 0x3fffffff);
	else
	  vobu->vobu_next = + (int32_t)(next & 0x3fffffff);
      }
    } else if((next = nav_dsi->sml_agli.data[angle-1].address) != 0) {
      vobu->vobu_length = nav_dsi->sml_pbi.ilvu_ea;

      if((next & 0x80000000) && (next != 0x7fffffff))
	vobu->vobu_next =  - (int32_t)(next & 0x3fffffff);
      else
	vobu->vobu_next =  + (int32_t)(next & 0x3fffffff);
    }
  }

  return 1;
}

/*
 * These are the main get_next_block function which actually get the media stream video and audio etc.
 *
 * There are two versions: The second one is using the zero-copy read ahead cache and therefore
 * hands out pointers targetting directly into the cache.
 * The first one uses a memcopy to fill this cache block into the application provided memory.
 * The benefit of this first one is that no special memory management is needed. The application is
 * the only one responsible of allocating and freeing the memory associated with the pointer.
 * The drawback is the additional memcopy.
 */

dvdnav_status_t dvdnav_get_next_block(dvdnav_t *this, uint8_t *buf,
				      int32_t *event, int32_t *len) {
  unsigned char *block;
  dvdnav_status_t status;

  block = buf;
  status = dvdnav_get_next_cache_block(this, &block, event, len);
  if (status == DVDNAV_STATUS_OK && block != buf) {
    /* we received a block from the cache, copy it, so we can give it back */
    memcpy(buf, block, DVD_VIDEO_LB_LEN);
    dvdnav_free_cache_block(this, block);
  }
  return status;
}

int64_t dvdnav_get_current_time(dvdnav_t *this) {
  int i;
  int64_t tm=0;
  dvd_state_t *state = &this->vm->state;

  for(i=0; i<state->cellN-1; i++) {
    if(!
        (state->pgc->cell_playback[i].block_type == BLOCK_TYPE_ANGLE_BLOCK &&
         state->pgc->cell_playback[i].block_mode != BLOCK_MODE_FIRST_CELL)
    )
      tm += dvdnav_convert_time(&state->pgc->cell_playback[i].playback_time);
  }
  tm += this->cur_cell_time;

  return tm;
}

dvdnav_status_t dvdnav_get_next_cache_block(dvdnav_t *this, uint8_t **buf,
					    int32_t *event, int32_t *len) {
  dvd_state_t *state;
  int32_t result;

  pthread_mutex_lock(&this->vm_lock);

  if(!this->started) {
    /* Start the VM */
    if (!vm_start(this->vm)) {
      printerr("Encrypted or faulty DVD");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    this->started = 1;
  }

  state = &(this->vm->state);
  (*event) = DVDNAV_NOP;
  (*len) = 0;

  /* Check the STOP flag */
  if(this->vm->stopped) {
    vm_stop(this->vm);
    (*event) = DVDNAV_STOP;
    this->started = 0;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  vm_position_get(this->vm, &this->position_next);

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: POS-NEXT ");
  vm_position_print(this->vm, &this->position_next);
  fprintf(MSG_OUT, "libdvdnav: POS-CUR  ");
  vm_position_print(this->vm, &this->position_current);
#endif

  /* did we hop? */
  if(this->position_current.hop_channel != this->position_next.hop_channel) {
    (*event) = DVDNAV_HOP_CHANNEL;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: HOP_CHANNEL\n");
#endif
    if (this->position_next.hop_channel - this->position_current.hop_channel >= HOP_SEEK) {
      int32_t num_angles = 0, current;

      /* we seeked -> check for multiple angles */
      vm_get_angle_info(this->vm, &current, &num_angles);
      if (num_angles > 1) {
        int32_t result, block;
	/* we have to skip the first VOBU when seeking in a multiangle feature,
	 * because it might belong to the wrong angle */
	block = this->position_next.cell_start + this->position_next.block;
	result = dvdnav_read_cache_block(this->cache, block, 1, buf);
	if(result <= 0) {
	  printerr("Error reading NAV packet.");
	  pthread_mutex_unlock(&this->vm_lock);
	  return DVDNAV_STATUS_ERR;
	}
	/* Decode nav into pci and dsi. Then get next VOBU info. */
	if(!dvdnav_decode_packet(this, *buf, &this->dsi, &this->pci)) {
	  printerr("Expected NAV packet but none found.");
#ifdef _XBMC
    /* skip this cell as we won't recover from this*/
    vm_get_next_cell(this->vm);
#endif
	  pthread_mutex_unlock(&this->vm_lock);
	  return DVDNAV_STATUS_ERR;
	}
	dvdnav_get_vobu(this, &this->dsi, &this->pci, &this->vobu);
	/* skip to next, if there is a next */
	if (this->vobu.vobu_next != SRI_END_OF_CELL) {
	  this->vobu.vobu_start += this->vobu.vobu_next;
	  this->vobu.vobu_next   = 0;
	}
	/* update VM state */
	this->vm->state.blockN = this->vobu.vobu_start - this->position_next.cell_start;
      }
    }
    this->position_current.hop_channel = this->position_next.hop_channel;
    /* update VOBU info */
    this->vobu.vobu_start  = this->position_next.cell_start + this->position_next.block;
    this->vobu.vobu_next   = 0;
    /* Make blockN == vobu_length to do expected_nav */
    this->vobu.vobu_length = 0;
    this->vobu.blockN      = 0;
    this->sync_wait        = 0;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* Check the HIGHLIGHT flag */
  if(this->position_current.button != this->position_next.button) {
    dvdnav_highlight_event_t *hevent = (dvdnav_highlight_event_t *)*buf;

    (*event) = DVDNAV_HIGHLIGHT;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: HIGHLIGHT\n");
#endif
    (*len) = sizeof(dvdnav_highlight_event_t);
    hevent->display = 1;
    hevent->buttonN = this->position_next.button;
    this->position_current.button = this->position_next.button;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* Check the WAIT flag */
  if(this->sync_wait) {
    (*event) = DVDNAV_WAIT;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: WAIT\n");
#endif
    (*len) = 0;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* Check to see if we need to change the currently opened VOB */
  if((this->position_current.vts != this->position_next.vts) ||
     (this->position_current.domain != this->position_next.domain)) {
    dvd_read_domain_t domain;
    int32_t vtsN;
    dvdnav_vts_change_event_t *vts_event = (dvdnav_vts_change_event_t *)*buf;

    if(this->file) {
      DVDCloseFile(this->file);
      this->file = NULL;
    }

    vts_event->old_vtsN = this->position_current.vts;
    vts_event->old_domain = this->position_current.domain;

    /* Use the DOMAIN to find whether to open menu or title VOBs */
    switch(this->position_next.domain) {
    case FP_DOMAIN:
    case VMGM_DOMAIN:
      domain = DVD_READ_MENU_VOBS;
      vtsN = 0;
      break;
    case VTSM_DOMAIN:
      domain = DVD_READ_MENU_VOBS;
      vtsN = this->position_next.vts;
      break;
    case VTS_DOMAIN:
      domain = DVD_READ_TITLE_VOBS;
      vtsN = this->position_next.vts;
      break;
    default:
      printerr("Unknown domain when changing VTS.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }

    this->position_current.vts = this->position_next.vts;
    this->position_current.domain = this->position_next.domain;
    dvdnav_read_cache_clear(this->cache);
    this->file = DVDOpenFile(vm_get_dvd_reader(this->vm), vtsN, domain);
    vts_event->new_vtsN = this->position_next.vts;
    vts_event->new_domain = this->position_next.domain;

    /* If couldn't open the file for some reason, moan */
    if(this->file == NULL) {
      printerrf("Error opening vtsN=%i, domain=%i.", vtsN, domain);
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }

    /* File opened successfully so return a VTS change event */
    (*event) = DVDNAV_VTS_CHANGE;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: VTS_CHANGE\n");
#endif
    (*len) = sizeof(dvdnav_vts_change_event_t);

    this->spu_clut_changed = 1;
    this->position_current.cell = -1; /* Force an update */
    this->position_current.spu_channel = -1; /* Force an update */
    this->position_current.audio_channel = -1; /* Force an update */;

    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* Check if the cell changed */
  if( (this->position_current.cell != this->position_next.cell) ||
      (this->position_current.cell_restart != this->position_next.cell_restart) ||
      (this->position_current.cell_start != this->position_next.cell_start) ) {
    dvdnav_cell_change_event_t *cell_event = (dvdnav_cell_change_event_t *)*buf;
    int32_t first_cell_nr, last_cell_nr, i;
    dvd_state_t *state = &this->vm->state;

    this->cur_cell_time = 0;
    (*event) = DVDNAV_CELL_CHANGE;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: CELL_CHANGE\n");
#endif
    (*len) = sizeof(dvdnav_cell_change_event_t);

    cell_event->cellN = state->cellN;
    cell_event->pgN   = state->pgN;
    cell_event->cell_length =
      dvdnav_convert_time(&state->pgc->cell_playback[state->cellN-1].playback_time);

    cell_event->pg_length = 0;
    /* Find start cell of program. */
    first_cell_nr = state->pgc->program_map[state->pgN-1];
    /* Find end cell of program */
    if(state->pgN < state->pgc->nr_of_programs)
      last_cell_nr = state->pgc->program_map[state->pgN] - 1;
    else
      last_cell_nr = state->pgc->nr_of_cells;
    for (i = first_cell_nr; i <= last_cell_nr; i++)
      cell_event->pg_length +=
        dvdnav_convert_time(&state->pgc->cell_playback[i - 1].playback_time);
    cell_event->pgc_length = dvdnav_convert_time(&state->pgc->playback_time);

    cell_event->cell_start = 0;
    for (i = 0; i < state->cellN; i++)
    {
      /* only count the first angle cell */
      if(  state->pgc->cell_playback[i].block_type == BLOCK_TYPE_ANGLE_BLOCK 
        && state->pgc->cell_playback[i].block_mode != BLOCK_MODE_FIRST_CELL )
        continue;

        cell_event->cell_start +=
          dvdnav_convert_time(&state->pgc->cell_playback[i].playback_time);
    }    
    cell_event->cell_start-= dvdnav_convert_time(&state->pgc->cell_playback[state->cellN-1].playback_time);

    cell_event->pg_start = 0;
    for (i = 1; i < state->pgc->program_map[state->pgN-1]; i++)
      cell_event->pg_start +=
        dvdnav_convert_time(&state->pgc->cell_playback[i - 1].playback_time);

    this->position_current.cell         = this->position_next.cell;
    this->position_current.cell_restart = this->position_next.cell_restart;
    this->position_current.cell_start   = this->position_next.cell_start;
    this->position_current.block        = this->position_next.block;

    /* vobu info is used for mid cell resumes */
    this->vobu.vobu_start               = this->position_next.cell_start + this->position_next.block;
    this->vobu.vobu_next                = 0;
    /* Make blockN == vobu_length to do expected_nav */
    this->vobu.vobu_length = 0;
    this->vobu.blockN      = 0;

    /* update the spu palette at least on PGC changes */
    this->spu_clut_changed = 1;
    this->position_current.spu_channel = -1; /* Force an update */
    this->position_current.audio_channel = -1; /* Force an update */

    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* has the CLUT changed? */
  if(this->spu_clut_changed) {
    (*event) = DVDNAV_SPU_CLUT_CHANGE;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: SPU_CLUT_CHANGE\n");
#endif
    (*len) = 16 * sizeof(uint32_t);
    memcpy(*buf, &(state->pgc->palette), 16 * sizeof(uint32_t));
    this->spu_clut_changed = 0;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* has the SPU channel changed? */
  if(this->position_current.spu_channel != this->position_next.spu_channel) {
    dvdnav_spu_stream_change_event_t *stream_change = (dvdnav_spu_stream_change_event_t *)*buf;

    (*event) = DVDNAV_SPU_STREAM_CHANGE;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: SPU_STREAM_CHANGE\n");
#endif
    (*len) = sizeof(dvdnav_spu_stream_change_event_t);
    stream_change->physical_wide      = vm_get_subp_active_stream(this->vm, 0);
    stream_change->physical_letterbox = vm_get_subp_active_stream(this->vm, 1);
    stream_change->physical_pan_scan  = vm_get_subp_active_stream(this->vm, 2);
    this->position_current.spu_channel = this->position_next.spu_channel;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: SPU_STREAM_CHANGE stream_id_wide=%d\n",stream_change->physical_wide);
    fprintf(MSG_OUT, "libdvdnav: SPU_STREAM_CHANGE stream_id_letterbox=%d\n",stream_change->physical_letterbox);
    fprintf(MSG_OUT, "libdvdnav: SPU_STREAM_CHANGE stream_id_pan_scan=%d\n",stream_change->physical_pan_scan);
    fprintf(MSG_OUT, "libdvdnav: SPU_STREAM_CHANGE returning DVDNAV_STATUS_OK\n");
#endif
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* has the audio channel changed? */
  if(this->position_current.audio_channel != this->position_next.audio_channel) {
    dvdnav_audio_stream_change_event_t *stream_change = (dvdnav_audio_stream_change_event_t *)*buf;

    (*event) = DVDNAV_AUDIO_STREAM_CHANGE;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: AUDIO_STREAM_CHANGE\n");
#endif
    (*len) = sizeof(dvdnav_audio_stream_change_event_t);
    stream_change->physical = vm_get_audio_active_stream( this->vm );
    stream_change->logical = this->position_next.audio_channel;
    this->position_current.audio_channel = this->position_next.audio_channel;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: AUDIO_STREAM_CHANGE stream_id=%d returning DVDNAV_STATUS_OK\n",stream_change->physical);
#endif
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* Check the STILLFRAME flag */
  if(this->position_current.still != 0) {
    dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)*buf;

    (*event) = DVDNAV_STILL_FRAME;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: STILL_FRAME\n");
#endif
    (*len) = sizeof(dvdnav_still_event_t);
    still_event->length = this->position_current.still;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* Have we reached the end of a VOBU? */
  if (this->vobu.blockN >= this->vobu.vobu_length) {

    /* Have we reached the end of a cell? */
    if(this->vobu.vobu_next == SRI_END_OF_CELL) {
      /* End of Cell from NAV DSI info */
#ifdef LOG_DEBUG
      fprintf(MSG_OUT, "libdvdnav: Still set to %x\n", this->position_next.still);
#endif
      this->position_current.still = this->position_next.still;

      /* we are about to leave a cell, so a lot of state changes could occur;
       * under certain conditions, the application should get in sync with us before this,
       * otherwise it might show stills or menus too shortly */
      if ((this->position_current.still || this->pci.hli.hl_gi.hli_ss) && !this->sync_wait_skip) {
        this->sync_wait = 1;
      } else {
	if( this->position_current.still == 0 || this->skip_still ) {
	  /* no active cell still -> get us to the next cell */
	  vm_get_next_cell(this->vm);
	  this->position_current.still = 0; /* still gets activated at end of cell */
	  this->skip_still = 0;
	  this->sync_wait_skip = 0;
	}
      }
      /* handle related state changes in next iteration */
      (*event) = DVDNAV_NOP;
      (*len) = 0;
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_OK;
    }

    /* Perform remapping jump if necessary (this is always a
     * VOBU boundary). */
    if (this->vm->map) {
      this->vobu.vobu_next = remap_block( this->vm->map,
        this->vm->state.domain, this->vm->state.TTN_REG,
        this->vm->state.pgN,
        this->vobu.vobu_start, this->vobu.vobu_next);
    }

    /* at the start of the next VOBU -> expecting NAV packet */
    result = dvdnav_read_cache_block(this->cache, this->vobu.vobu_start + this->vobu.vobu_next, 1, buf);

    if(result <= 0) {
      printerr("Error reading NAV packet.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    /* Decode nav into pci and dsi. Then get next VOBU info. */
    if(!dvdnav_decode_packet(this, *buf, &this->dsi, &this->pci)) {
      printerr("Expected NAV packet but none found.");
#ifdef _XBMC
      /* skip this cell as we won't recover from this */
      vm_get_next_cell(this->vm);
#endif
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    /* We need to update the vm state->blockN with which VOBU we are in.
     * This is so RSM resumes to the VOBU level and not just the CELL level.
     */
    this->vm->state.blockN = this->vobu.vobu_start - this->position_current.cell_start;

    dvdnav_get_vobu(this, &this->dsi, &this->pci, &this->vobu);
    this->vobu.blockN = 0;
    /* Give the cache a hint about the size of next VOBU.
     * This improves pre-caching, because the VOBU will almost certainly be read entirely.
     */
    dvdnav_pre_cache_blocks(this->cache, this->vobu.vobu_start+1, this->vobu.vobu_length+1);

    /* release NAV menu filter, when we reach the same NAV packet again */
    if (this->last_cmd_nav_lbn == this->pci.pci_gi.nv_pck_lbn)
      this->last_cmd_nav_lbn = SRI_END_OF_CELL;

    /* Successfully got a NAV packet */
    (*event) = DVDNAV_NAV_PACKET;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: NAV_PACKET\n");
#endif
    (*len) = 2048;
    this->cur_cell_time = dvdnav_convert_time(&this->dsi.dsi_gi.c_eltm);
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  }

  /* If we've got here, it must just be a normal block. */
  if(!this->file) {
    printerr("Attempting to read without opening file.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  this->vobu.blockN++;
  result = dvdnav_read_cache_block(this->cache, this->vobu.vobu_start + this->vobu.blockN, 1, buf);
  if(result <= 0) {
    printerr("Error reading from DVD.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  (*event) = DVDNAV_BLOCK_OK;
  (*len) = 2048;

  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_title_string(dvdnav_t *this, const char **title_str) {
  (*title_str) = this->vm->dvd_name;
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_serial_string(dvdnav_t *this, const char **serial_str) {
  (*serial_str) = this->vm->dvd_serial;
  return DVDNAV_STATUS_OK;
}

uint8_t dvdnav_get_video_aspect(dvdnav_t *this) {
  uint8_t         retval;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  retval = (uint8_t)vm_get_video_aspect(this->vm);
  pthread_mutex_unlock(&this->vm_lock);

  return retval;
}
int dvdnav_get_video_resolution(dvdnav_t *this, uint32_t *width, uint32_t *height) {
  int w, h;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  vm_get_video_res(this->vm, &w, &h);
  pthread_mutex_unlock(&this->vm_lock);

  *width  = w;
  *height = h;
  return 0;
}

uint8_t dvdnav_get_video_scale_permission(dvdnav_t *this) {
  uint8_t         retval;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  retval = (uint8_t)vm_get_video_scale_permission(this->vm);
  pthread_mutex_unlock(&this->vm_lock);

  return retval;
}

uint16_t dvdnav_audio_stream_to_lang(dvdnav_t *this, uint8_t stream) {
  audio_attr_t  attr;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  attr = vm_get_audio_attr(this->vm, stream);
  pthread_mutex_unlock(&this->vm_lock);

  if(attr.lang_type != 1)
    return 0xffff;

  return attr.lang_code;
}

uint16_t dvdnav_audio_stream_format(dvdnav_t *this, uint8_t stream) {
  audio_attr_t  attr;
  uint16_t format;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1; /* 0xffff */
  }

  pthread_mutex_lock(&this->vm_lock);
  attr = vm_get_audio_attr(this->vm, stream);
  pthread_mutex_unlock(&this->vm_lock);

  if (attr.audio_format >=0 && attr.audio_format <= 7)
    format = attr.audio_format;
  else
    format = 0xffff;

  return format;
}

uint16_t dvdnav_audio_stream_channels(dvdnav_t *this, uint8_t stream) {
  audio_attr_t  attr;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1; /* 0xffff */
  }

  pthread_mutex_lock(&this->vm_lock);
  attr = vm_get_audio_attr(this->vm, stream);
  pthread_mutex_unlock(&this->vm_lock);

  return attr.channels + 1;
}

uint16_t dvdnav_spu_stream_to_lang(dvdnav_t *this, uint8_t stream) {
  subp_attr_t  attr;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  attr = vm_get_subp_attr(this->vm, stream);
  pthread_mutex_unlock(&this->vm_lock);

  if(attr.type != 1)
    return 0xffff;

  return attr.lang_code;
}

int8_t dvdnav_get_audio_logical_stream(dvdnav_t *this, uint8_t audio_num) {
  int8_t       retval;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return -1;
  }
  retval = vm_get_audio_stream(this->vm, audio_num);
  pthread_mutex_unlock(&this->vm_lock);

  return retval;
}

dvdnav_status_t dvdnav_get_audio_attr(dvdnav_t *this, uint8_t audio_num, audio_attr_t *audio_attr) {
  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }
  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return -1;
  }
  *audio_attr=vm_get_audio_attr(this->vm, audio_num);
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

int8_t dvdnav_get_spu_logical_stream(dvdnav_t *this, uint8_t subp_num) {
  int8_t       retval;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return -1;
  }
  retval = vm_get_subp_stream(this->vm, subp_num, 0);
  pthread_mutex_unlock(&this->vm_lock);

  return retval;
}

dvdnav_status_t dvdnav_get_spu_attr(dvdnav_t *this, uint8_t audio_num, subp_attr_t *subp_attr) {
  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }
  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return -1;
  }
  *subp_attr=vm_get_subp_attr(this->vm, audio_num);
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}

int8_t dvdnav_get_active_audio_stream(dvdnav_t *this) {
  int8_t        retval;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return -1;
  }
  retval = vm_get_audio_active_stream(this->vm);
  pthread_mutex_unlock(&this->vm_lock);

  return retval;
}

int8_t dvdnav_get_active_spu_stream(dvdnav_t *this) {
  int8_t        retval;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return -1;
  }
  retval = vm_get_subp_active_stream(this->vm, 0);
  pthread_mutex_unlock(&this->vm_lock);

  return retval;
}

static int8_t dvdnav_is_domain(dvdnav_t *this, domain_t domain) {
  int8_t        retval;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return -1;
  }

  pthread_mutex_lock(&this->vm_lock);
  retval = (this->vm->state.domain == domain);
  pthread_mutex_unlock(&this->vm_lock);

  return retval;
}

/* First Play domain. (Menu) */
int8_t dvdnav_is_domain_fp(dvdnav_t *this) {
  return dvdnav_is_domain(this, FP_DOMAIN);
}
/* Video management Menu domain. (Menu) */
int8_t dvdnav_is_domain_vmgm(dvdnav_t *this) {
  return dvdnav_is_domain(this, VMGM_DOMAIN);
}
/* Video Title Menu domain (Menu) */
int8_t dvdnav_is_domain_vtsm(dvdnav_t *this) {
  return dvdnav_is_domain(this, VTSM_DOMAIN);
}
/* Video Title domain (playing movie). */
int8_t dvdnav_is_domain_vts(dvdnav_t *this) {
  return dvdnav_is_domain(this, VTS_DOMAIN);
}

/* Generally delegate angle information handling to VM */
dvdnav_status_t dvdnav_angle_change(dvdnav_t *this, int32_t angle) {
  int32_t num, current;

  pthread_mutex_lock(&this->vm_lock);
  vm_get_angle_info(this->vm, &current, &num);
  /* Set angle SPRM if valid */
  if((angle > 0) && (angle <= num)) {
    this->vm->state.AGL_REG = angle;
  } else {
    printerr("Passed an invalid angle number.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_angle_info(dvdnav_t *this, int32_t *current_angle,
				      int32_t *number_of_angles) {
  pthread_mutex_lock(&this->vm_lock);
  vm_get_angle_info(this->vm, current_angle, number_of_angles);
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

pci_t* dvdnav_get_current_nav_pci(dvdnav_t *this) {
  if(!this) return 0;
  return &this->pci;
}

dsi_t* dvdnav_get_current_nav_dsi(dvdnav_t *this) {
  if(!this) return 0;
  return &this->dsi;
}

uint32_t dvdnav_get_next_still_flag(dvdnav_t *this) {
  if(!this) return -1;
  return this->position_next.still;
}

user_ops_t dvdnav_get_restrictions(dvdnav_t* this) {
  /*
   * user_ops_t is a structure of 32 bits.  We want to compute
   * the union of two of those bitfields so to make this quicker
   * than performing 32 ORs, we will access them as 32bits words.
   */
  union {
    user_ops_t ops_struct;
    uint32_t   ops_int;
  } ops, tmp;

  ops.ops_int = 0;

  if(!this) {
    printerr("Passed a NULL pointer.");
    return ops.ops_struct;
  }

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return ops.ops_struct;
  }

  pthread_mutex_lock(&this->vm_lock); 
  ops.ops_struct = this->pci.pci_gi.vobu_uop_ctl;

  if(this->vm && this->vm->state.pgc) {
    tmp.ops_struct = this->vm->state.pgc->prohibited_ops;
    ops.ops_int |= tmp.ops_int;
  }
  pthread_mutex_unlock(&this->vm_lock);

  return ops.ops_struct;
}

#ifdef _XBMC

vm_t* dvdnav_get_vm(dvdnav_t *this) {
  if(!this) return 0;
  return this->vm;
}

/* return the alpha and color for the current active button
 * color, alpha [0][] = selection
 * color, alpha = color
 *
 * argsize = [2][4]
 */
int dvdnav_get_button_info(dvdnav_t* this, int alpha[2][4], int color[2][4])
{
  int current_button, current_button_color, i;
  pci_t* pci;
  
  if (!this) return -1;
  
  pci = dvdnav_get_current_nav_pci(this);
  if (!pci) return -1;
  
  dvdnav_get_current_highlight(this, &current_button);
  current_button_color = pci->hli.btnit[current_button - 1].btn_coln;
  
  for (i = 0; i < 2; i++)
  {
    alpha[i][0] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 0 & 0xf;
    alpha[i][1] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 4 & 0xf;
    alpha[i][2] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 8 & 0xf;
    alpha[i][3] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 12 & 0xf;
    
    color[i][0] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 16 & 0xf;
    color[i][1] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 20 & 0xf;
    color[i][2] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 24 & 0xf;
    color[i][3] = pci->hli.btn_colit.btn_coli[current_button_color - 1][i] >> 28 & 0xf;
  }
  
  return 0;
}

void dvdnav_free(void* pdata)
{
  free(pdata);
}

#undef printerr
#define printerr(str) strncpy(self->err_str, str, MAX_ERR_LEN);

#endif // _XBMC
