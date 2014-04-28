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

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "dvdnav/dvdnav.h"
#include <dvdread/nav_types.h>
#include <dvdread/ifo_types.h>
#include "remap.h"
#include "vm/decoder.h"
#include "vm/vm.h"
#include "dvdnav_internal.h"
#include <dvdread/ifo_read.h>

/*
#define LOG_DEBUG
*/

/* Searching API calls */

/* Scan the ADMAP for a particular block number. */
/* Return placed in vobu. */
/* Returns error status */
/* FIXME: Maybe need to handle seeking outside current cell. */
static dvdnav_status_t dvdnav_scan_admap(dvdnav_t *this, int32_t domain, uint32_t seekto_block, int next, uint32_t *vobu) {
  vobu_admap_t *admap = NULL;

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: Seeking to target %u ...\n", seekto_block);
#endif
  *vobu = -1;

  /* Search through the VOBU_ADMAP for the nearest VOBU
   * to the target block */
  switch(domain) {
  case FP_DOMAIN:
  case VMGM_DOMAIN:
    admap = this->vm->vmgi->menu_vobu_admap;
    break;
  case VTSM_DOMAIN:
    admap = this->vm->vtsi->menu_vobu_admap;
    break;
  case VTS_DOMAIN:
    admap = this->vm->vtsi->vts_vobu_admap;
    break;
  default:
    fprintf(MSG_OUT, "libdvdnav: Error: Unknown domain for seeking.\n");
  }
  if(admap) {
    uint32_t address = 0;
    uint32_t vobu_start, next_vobu;
    int admap_entries = (admap->last_byte + 1 - VOBU_ADMAP_SIZE)/VOBU_ADMAP_SIZE;

    /* Search through ADMAP for best sector */
    vobu_start = SRI_END_OF_CELL;
    /* FIXME: Implement a faster search algorithm */
    while(address < admap_entries) {
      next_vobu = admap->vobu_start_sectors[address];

      /* fprintf(MSG_OUT, "libdvdnav: Found block %u\n", next_vobu); */

      if(vobu_start <= seekto_block && next_vobu > seekto_block)
        break;
      vobu_start = next_vobu;
      address++;
    }
    *vobu = next ? next_vobu : vobu_start;
    return DVDNAV_STATUS_OK;
  }
  fprintf(MSG_OUT, "libdvdnav: admap not located\n");
  return DVDNAV_STATUS_ERR;
}

/* FIXME: right now, this function does not use the time tables but interpolates
   only the cell times */
dvdnav_status_t dvdnav_time_search(dvdnav_t *this,
				   uint64_t time) {

  uint64_t target = time;
  uint64_t length = 0;
  uint32_t first_cell_nr, last_cell_nr, cell_nr;
  int32_t found;
  cell_playback_t *cell;
  dvd_state_t *state;

  if(this->position_current.still != 0) {
    printerr("Cannot seek in a still frame.");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);
  state = &(this->vm->state);
  if(!state->pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  if((state->pgc->prohibited_ops.title_or_time_play == 1) ||
      (this->pci.pci_gi.vobu_uop_ctl.title_or_time_play == 1 )){
    printerr("operation forbidden.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  this->cur_cell_time = 0;
  if (this->pgc_based) {
    first_cell_nr = 1;
    last_cell_nr = state->pgc->nr_of_cells;
  } else {
    /* Find start cell of program. */
    first_cell_nr = state->pgc->program_map[state->pgN-1];
    /* Find end cell of program */
    if(state->pgN < state->pgc->nr_of_programs)
      last_cell_nr = state->pgc->program_map[state->pgN] - 1;
    else
      last_cell_nr = state->pgc->nr_of_cells;
  }

  /* FIXME: using time map is not going to work unless we are pgc_based */
  /*        we'd need to recalculate the time to be relative to full pgc first*/
  if(!this->pgc_based)
  {
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - not pgc based\n");
#endif
    goto timemapdone;
  }

  if(!this->vm->vtsi->vts_tmapt){
    /* no time map for this program chain */
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - no time map for this program chain\n");
#endif
    goto timemapdone;
  }

  if(this->vm->vtsi->vts_tmapt->nr_of_tmaps < state->pgcN){
    /* to few time maps for this program chain */
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - to few time maps for this program chain\n");
#endif
    goto timemapdone;
  }

  /* get the tmpat corresponding to the pgc */
  vts_tmap_t *tmap = &(this->vm->vtsi->vts_tmapt->tmap[state->pgcN-1]);

  if(tmap->tmu == 0){
    /* no time unit for this time map */
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: time_search - no time unit for this time map\n");
#endif
    goto timemapdone;
  }

  /* time is in pts (90khz clock), get the number of tmu's that represent */
  /* first entry defines at time tmu not time zero */
  int entry = time / tmap->tmu / 90000 - 1;
  if(entry > tmap->nr_of_entries)
    entry = tmap->nr_of_entries -1;

  if(entry > 0)
  {
    /* get the table entry, disregarding marking of discontinuity */
    target = tmap->map_ent[entry] & 0x7fffffff;
  }
  else
  {
    /* start from first vobunit */
    target = state->pgc->cell_playback[first_cell_nr-1].first_sector;;
  }

  /* if we have an additional entry we can interpolate next position */
  /* allowed only if next entry isn't discontinious */

  if( entry < tmap->nr_of_entries - 1)
  {
    const uint32_t target2 = tmap->map_ent[entry+1];
    const uint64_t timeunit = tmap->tmu*90000;
    if( !( target2 & 0x80000000) )
    {
      length = target2 - target;
      target += (uint32_t) (length * ( time - (entry+1)*timeunit ) / timeunit);
    }
  }
  found = 1;

timemapdone:

  found = 0;
  for(cell_nr = first_cell_nr; cell_nr <= last_cell_nr; cell_nr ++) {
    cell =  &(state->pgc->cell_playback[cell_nr-1]);
    if(cell->block_type == BLOCK_TYPE_ANGLE_BLOCK && cell->block_mode != BLOCK_MODE_FIRST_CELL)
      continue;

    if(found) {

      if (target >= cell->first_sector
      &&  target <= cell->last_sector)
         break;

    } else {

      length = dvdnav_convert_time(&cell->playback_time);
      if (time >= length) {
        time -= length;
      } else {
        /* FIXME: there must be a better way than interpolation */
        target = time * (cell->last_sector - cell->first_sector + 1) / length;
        target += cell->first_sector;

  #ifdef LOG_DEBUG
        if( cell->first_sector > target || target > cell->last_sector )
          fprintf(MSG_OUT, "libdvdnav: time_search - sector is not within cell min:%u, max:%u, cur:%u\n", cell->first_sector, cell->last_sector, target);
  #endif

        found = 1;
        break;
      }
    }
  }
  if(found) {
    uint32_t vobu;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: Seeking to cell %i from choice of %i to %i\n",
	    cell_nr, first_cell_nr, last_cell_nr);
#endif
    if (dvdnav_scan_admap(this, state->domain, target, 0, &vobu) == DVDNAV_STATUS_OK) {
      uint32_t start = state->pgc->cell_playback[cell_nr-1].first_sector;

      if (vm_jump_cell_block(this->vm, cell_nr, vobu - start)) {
#ifdef LOG_DEBUG
        fprintf(MSG_OUT, "libdvdnav: After cellN=%u blockN=%u target=%x vobu=%x start=%x\n" ,
          state->cellN, state->blockN, target, vobu, start);
#endif
        this->vm->hop_channel += HOP_SEEK;
        pthread_mutex_unlock(&this->vm_lock);
        return DVDNAV_STATUS_OK;
      }
    }
  }

  fprintf(MSG_OUT, "libdvdnav: Error when seeking\n");
  printerr("Error when seeking.");
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_sector_search(dvdnav_t *this,
				     uint64_t offset, int32_t origin) {
  uint32_t target = 0;
  uint32_t current_pos;
  uint32_t cur_sector;
  uint32_t cur_cell_nr;
  uint32_t length = 0;
  uint32_t first_cell_nr, last_cell_nr, cell_nr;
  int32_t found;
  int forward = 0;
  cell_playback_t *cell;
  dvd_state_t *state;
  dvdnav_status_t result;

  if(this->position_current.still != 0) {
    printerr("Cannot seek in a still frame.");
    return DVDNAV_STATUS_ERR;
  }

  result = dvdnav_get_position(this, &target, &length);
  if(!result) {
    printerr("Cannot get current position");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);
  state = &(this->vm->state);
  if(!state->pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: seeking to offset=%llu pos=%u length=%u\n", offset, target, length);
  fprintf(MSG_OUT, "libdvdnav: Before cellN=%u blockN=%u\n", state->cellN, state->blockN);
#endif

  current_pos = target;
  cur_sector = this->vobu.vobu_start + this->vobu.blockN;
  cur_cell_nr = state->cellN;

  switch(origin) {
   case SEEK_SET:
    if(offset >= length) {
      printerr("Request to seek behind end.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    target = offset;
    break;
   case SEEK_CUR:
    if(target + offset >= length) {
      printerr("Request to seek behind end.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    target += offset;
    break;
   case SEEK_END:
    if(length < offset) {
      printerr("Request to seek before start.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
    target = length - offset;
    break;
   default:
    /* Error occured */
    printerr("Illegal seek mode.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  forward = target > current_pos;

  this->cur_cell_time = 0;
  if (this->pgc_based) {
    first_cell_nr = 1;
    last_cell_nr = state->pgc->nr_of_cells;
  } else {
    /* Find start cell of program. */
    first_cell_nr = state->pgc->program_map[state->pgN-1];
    /* Find end cell of program */
    if(state->pgN < state->pgc->nr_of_programs)
      last_cell_nr = state->pgc->program_map[state->pgN] - 1;
    else
      last_cell_nr = state->pgc->nr_of_cells;
  }

  found = 0;
  for(cell_nr = first_cell_nr; (cell_nr <= last_cell_nr) && !found; cell_nr ++) {
    cell =  &(state->pgc->cell_playback[cell_nr-1]);
    if(cell->block_type == BLOCK_TYPE_ANGLE_BLOCK && cell->block_mode != BLOCK_MODE_FIRST_CELL)
      continue;
    length = cell->last_sector - cell->first_sector + 1;
    if (target >= length) {
      target -= length;
    } else {
      /* convert the target sector from Cell-relative to absolute physical sector */
      target += cell->first_sector;
      if (forward && (cell_nr == cur_cell_nr)) {
        uint32_t vobu;
        /* if we are seeking forward from the current position, make sure
         * we move to a new position that is after our current position.
         * simply truncating to the vobu will go backwards */
        if (dvdnav_scan_admap(this, state->domain, target, 0, &vobu) != DVDNAV_STATUS_OK)
          break;
        if (vobu <= cur_sector) {
          if (dvdnav_scan_admap(this, state->domain, target, 1, &vobu) != DVDNAV_STATUS_OK)
            break;
          if (vobu > cell->last_sector) {
            if (cell_nr == last_cell_nr)
              break;
            cell_nr++;
            cell =  &(state->pgc->cell_playback[cell_nr-1]);
            target = cell->first_sector;
          } else {
            target = vobu;
          }
        }
      }
      found = 1;
      break;
    }
  }

  if(found) {
    uint32_t vobu;
#ifdef LOG_DEBUG
    fprintf(MSG_OUT, "libdvdnav: Seeking to cell %i from choice of %i to %i\n",
	    cell_nr, first_cell_nr, last_cell_nr);
#endif
    if (dvdnav_scan_admap(this, state->domain, target, 0, &vobu) == DVDNAV_STATUS_OK) {
      int32_t start = state->pgc->cell_playback[cell_nr-1].first_sector;

      if (vm_jump_cell_block(this->vm, cell_nr, vobu - start)) {
#ifdef LOG_DEBUG
        fprintf(MSG_OUT, "libdvdnav: After cellN=%u blockN=%u target=%x vobu=%x start=%x\n" ,
          state->cellN, state->blockN, target, vobu, start);
#endif
        this->vm->hop_channel += HOP_SEEK;
        pthread_mutex_unlock(&this->vm_lock);
        return DVDNAV_STATUS_OK;
      }
    }
  }

  fprintf(MSG_OUT, "libdvdnav: Error when seeking\n");
  fprintf(MSG_OUT, "libdvdnav: FIXME: Implement seeking to location %u\n", target);
  printerr("Error when seeking.");
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_part_search(dvdnav_t *this, int32_t part) {
  int32_t title, old_part;

  if (dvdnav_current_title_info(this, &title, &old_part) == DVDNAV_STATUS_OK)
    return dvdnav_part_play(this, title, part);
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_prev_pg_search(dvdnav_t *this) {
  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: previous chapter\n");
#endif
  if (!vm_jump_prev_pg(this->vm)) {
    fprintf(MSG_OUT, "libdvdnav: previous chapter failed.\n");
    printerr("Skip to previous chapter failed.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  this->cur_cell_time = 0;
  this->position_current.still = 0;
  this->vm->hop_channel++;
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: previous chapter done\n");
#endif
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_top_pg_search(dvdnav_t *this) {
  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: top chapter\n");
#endif
  if (!vm_jump_top_pg(this->vm)) {
    fprintf(MSG_OUT, "libdvdnav: top chapter failed.\n");
    printerr("Skip to top chapter failed.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  this->cur_cell_time = 0;
  this->position_current.still = 0;
  this->vm->hop_channel++;
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: top chapter done\n");
#endif
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_next_pg_search(dvdnav_t *this) {
  vm_t *try_vm;

  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: next chapter\n");
#endif
  /* make a copy of current VM and try to navigate the copy to the next PG */
  try_vm = vm_new_copy(this->vm);
  if (!vm_jump_next_pg(try_vm) || try_vm->stopped) {
    vm_free_copy(try_vm);
    /* next_pg failed, try to jump at least to the next cell */
    try_vm = vm_new_copy(this->vm);
    vm_get_next_cell(try_vm);
    if (try_vm->stopped) {
      vm_free_copy(try_vm);
      fprintf(MSG_OUT, "libdvdnav: next chapter failed.\n");
      printerr("Skip to next chapter failed.");
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_ERR;
    }
  }
  this->cur_cell_time = 0;
  /* merge changes on success */
  vm_merge(this->vm, try_vm);
  vm_free_copy(try_vm);
  this->position_current.still = 0;
  this->vm->hop_channel++;
#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: next chapter done\n");
#endif
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_menu_call(dvdnav_t *this, DVDMenuID_t menu) {
  vm_t *try_vm;

  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  this->cur_cell_time = 0;
  /* make a copy of current VM and try to navigate the copy to the menu */
  try_vm = vm_new_copy(this->vm);
  if ( (menu == DVD_MENU_Escape) && (this->vm->state.domain != VTS_DOMAIN)) {
    /* Try resume */
    if (vm_jump_resume(try_vm) && !try_vm->stopped) {
        /* merge changes on success */
        vm_merge(this->vm, try_vm);
        vm_free_copy(try_vm);
        this->position_current.still = 0;
        this->vm->hop_channel++;
        pthread_mutex_unlock(&this->vm_lock);
        return DVDNAV_STATUS_OK;
    }
  }
  if (menu == DVD_MENU_Escape) menu = DVD_MENU_Root;

  if (vm_jump_menu(try_vm, menu) && !try_vm->stopped) {
    /* merge changes on success */
    vm_merge(this->vm, try_vm);
    vm_free_copy(try_vm);
    this->position_current.still = 0;
    this->vm->hop_channel++;
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_OK;
  } else {
    vm_free_copy(try_vm);
    printerr("No such menu or menu not reachable.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
}

dvdnav_status_t dvdnav_get_position(dvdnav_t *this, uint32_t *pos,
				    uint32_t *len) {
  uint32_t cur_sector;
  int32_t cell_nr, first_cell_nr, last_cell_nr;
  cell_playback_t *cell;
  dvd_state_t *state;

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);
  state = &(this->vm->state);
  if(!state->pgc || this->vm->stopped) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  if (this->position_current.hop_channel  != this->vm->hop_channel ||
      this->position_current.domain       != state->domain         ||
      this->position_current.vts          != state->vtsN           ||
      this->position_current.cell_restart != state->cell_restart) {
    printerr("New position not yet determined.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  /* Get current sector */
  cur_sector = this->vobu.vobu_start + this->vobu.blockN;

  if (this->pgc_based) {
    first_cell_nr = 1;
    last_cell_nr = state->pgc->nr_of_cells;
  } else {
    /* Find start cell of program. */
    first_cell_nr = state->pgc->program_map[state->pgN-1];
    /* Find end cell of program */
    if(state->pgN < state->pgc->nr_of_programs)
      last_cell_nr = state->pgc->program_map[state->pgN] - 1;
    else
      last_cell_nr = state->pgc->nr_of_cells;
  }

  *pos = -1;
  *len = 0;
  for (cell_nr = first_cell_nr; cell_nr <= last_cell_nr; cell_nr++) {
    cell = &(state->pgc->cell_playback[cell_nr-1]);
    if (cell_nr == state->cellN) {
      /* the current sector is in this cell,
       * pos is length of PG up to here + sector's offset in this cell */
      *pos = *len + cur_sector - cell->first_sector;
    }
    *len += cell->last_sector - cell->first_sector + 1;
  }

  assert((signed)*pos != -1);

  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_position_in_title(dvdnav_t *this,
					     uint32_t *pos,
					     uint32_t *len) {
  uint32_t cur_sector;
  uint32_t first_cell_nr;
  uint32_t last_cell_nr;
  cell_playback_t *first_cell;
  cell_playback_t *last_cell;
  dvd_state_t *state;

  state = &(this->vm->state);
  if(!state->pgc) {
    printerr("No current PGC.");
    return DVDNAV_STATUS_ERR;
  }

  /* Get current sector */
  cur_sector = this->vobu.vobu_start + this->vobu.blockN;

  /* Now find first and last cells in title. */
  first_cell_nr = state->pgc->program_map[0];
  first_cell = &(state->pgc->cell_playback[first_cell_nr-1]);
  last_cell_nr = state->pgc->nr_of_cells;
  last_cell = &(state->pgc->cell_playback[last_cell_nr-1]);

  *pos = cur_sector - first_cell->first_sector;
  *len = last_cell->last_sector - first_cell->first_sector;

  return DVDNAV_STATUS_OK;
}

uint32_t dvdnav_describe_title_chapters(dvdnav_t *this, int32_t title, uint64_t **times, uint64_t *duration) {
  int32_t retval=0;
  uint16_t parts, i;
  title_info_t *ptitle = NULL;
  ptt_info_t *ptt = NULL;
  ifo_handle_t *ifo = NULL;
  pgc_t *pgc;
  cell_playback_t *cell;
  uint64_t length, *tmp=NULL;

  *times = NULL;
  *duration = 0;
  pthread_mutex_lock(&this->vm_lock);
  if(!this->vm->vmgi) {
    printerr("Bad VM state or missing VTSI.");
    goto fail;
  }
  if(!this->started) {
    /* don't report an error but be nice */
    vm_start(this->vm);
    this->started = 1;
  }
  ifo = vm_get_title_ifo(this->vm, title);
  if(!ifo || !ifo->vts_pgcit) {
    printerr("Couldn't open IFO for chosen title, exit.");
    retval = 0;
    goto fail;
  }

  ptitle = &this->vm->vmgi->tt_srpt->title[title-1];
  parts = ptitle->nr_of_ptts;
  ptt = ifo->vts_ptt_srpt->title[ptitle->vts_ttn-1].ptt;

  tmp = calloc(1, sizeof(uint64_t)*parts);
  if(!tmp)
    goto fail;

  length = 0;
  for(i=0; i<parts; i++) {
    uint32_t cellnr, endcellnr;
    if (ptt[i].pgcn == 0 || ptt[i].pgcn > ifo->vts_pgcit->nr_of_pgci_srp) {
      printerr("PGCN out of bounds.");
      continue;
    }
    if (ifo->vts_pgcit->pgci_srp[ptt[i].pgcn-1].pgc_start_byte >= ifo->vts_pgcit->last_byte) {
      printerr("PGC start out of bounds");
      continue;
    }
    pgc = ifo->vts_pgcit->pgci_srp[ptt[i].pgcn-1].pgc;
    if (pgc == NULL) {
      printerr("PGC missing.");
      continue;
    }
    if (pgc->program_map == NULL) {
      printerr("Program map missing.");
      continue;
    }
    if(ptt[i].pgn == 0 || ptt[i].pgn > pgc->nr_of_programs) {
      printerr("WRONG part number.");
      goto fail;
    }

    if (pgc->nr_of_cells == 0) {
      printerr("Number of cells cannot be 0");
      continue;
    }
    if ((cellnr = pgc->program_map[ptt[i].pgn-1]) == 0) {
      printerr("Cell new row cannot be 0");
      continue;
    }
    if (pgc->cell_playback == NULL) {
      printerr("Cell missing");
      continue;
    }

    if(ptt[i].pgn < pgc->nr_of_programs)
      endcellnr = pgc->program_map[ptt[i].pgn];
    else
      endcellnr = 0;

    do {
      cell = &pgc->cell_playback[cellnr-1];
      if(!(cell->block_type == BLOCK_TYPE_ANGLE_BLOCK &&
           cell->block_mode != BLOCK_MODE_FIRST_CELL
      ))
      {
        tmp[i] = length + dvdnav_convert_time(&cell->playback_time);
        length = tmp[i];
      }
      cellnr++;
    } while(cellnr < endcellnr);
  }
  *duration = length;
  vm_ifo_close(ifo);
  ifo = NULL;
  retval = parts;
  *times = tmp;

fail:
  pthread_mutex_unlock(&this->vm_lock);
  if(!retval && ifo)
    vm_ifo_close(ifo);
  if(!retval && tmp)
    free(tmp);
  return retval;
}

dvdnav_status_t dvdnav_get_state(dvdnav_t *this, dvd_state_t *save_state)
{
  if(!this || !this->vm) return DVDNAV_STATUS_ERR;

  pthread_mutex_lock(&this->vm_lock);
  
  if( !vm_get_state(this->vm, save_state) )
  {
    printerr("Failed to get vm state.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_set_state(dvdnav_t *this, dvd_state_t *save_state)
{
  if(!this || !this->vm)
  {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }

  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);

  /* reset the dvdnav state */
  memset(&this->pci,0,sizeof(this->pci));
  memset(&this->dsi,0,sizeof(this->dsi));
  this->last_cmd_nav_lbn = SRI_END_OF_CELL;

  /* Set initial values of flags */  
  this->position_current.still = 0;
  this->skip_still = 0;
  this->sync_wait = 0;
  this->sync_wait_skip = 0;
  this->spu_clut_changed = 0;


  /* set the state. this will also start the vm on that state */
  /* means the next read block should be comming from that new */
  /* state */
  if( !vm_set_state(this->vm, save_state) )
  {
    printerr("Failed to set vm state.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  } 

  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}



/* Get an admap and admap_len */
static vobu_admap_t* dvdnav_admap_get(dvdnav_t *this, dvd_state_t *state,
            int32_t *admap_len) {
  vobu_admap_t *admap = NULL;
  switch(state->domain) {
  case FP_DOMAIN:
  case VMGM_DOMAIN:
    admap = this->vm->vmgi->menu_vobu_admap;
    break;
  case VTSM_DOMAIN:
    admap = this->vm->vtsi->menu_vobu_admap;
    break;
  case VTS_DOMAIN:
    admap = this->vm->vtsi->vts_vobu_admap;
    break;
  default: {
    fprintf(MSG_OUT, "Unknown domain");
    return NULL;
  }
  }
  if (admap == NULL) return NULL;

  *admap_len = (admap->last_byte + 1 - VOBU_ADMAP_SIZE) / VOBU_ADMAP_SIZE;
  if (*admap_len <= 0) {
    fprintf(MSG_OUT, "admap_len <= 0");
    return NULL;
  }
  return admap;
}

/* Get a tmap, tmap_len and tmap_interval */
static vts_tmap_t* dvdnav_tmap_get(dvdnav_t *this, dvd_state_t *state,
            int32_t *tmap_len, int32_t *tmap_interval) {
  int32_t vts_idx = 0;
  domain_t domain;
  ifo_handle_t *ifo = NULL;
  vts_tmapt_t *tmapt = NULL;
  uint16_t tmap_count = 0;
  int32_t pgcN = 0;
  vts_tmap_t *tmap = NULL;
  int32_t result = 0;

  vts_idx = state->vtsN;
  domain = state->domain;
  switch(domain) {
  case FP_DOMAIN:
  case VTSM_DOMAIN:
  case VMGM_DOMAIN: {
    ifo = this->vm->vmgi;
    break;
  }
  case VTS_DOMAIN: {
    ifo = this->vm->vtsi;
    break;
  }
  default: {
    fprintf(MSG_OUT, "unknown domain for tmap");
    return NULL;
  }
  }
  if (ifo == NULL) return NULL;
  tmapt = ifo->vts_tmapt;
  /* HACK: ifo->vts_tmapt is NULL b/c ifo_read.c never loads it
   * load ifo->vts_tmapt directly*/
  if (tmapt == NULL) {
    result = ifoRead_VTS_TMAPT(ifo);
    if (!result) {
      return NULL;
    }
    tmapt = ifo->vts_tmapt;
    if (tmapt == NULL) return NULL;
  }

  tmap_count = tmapt->nr_of_tmaps;
  pgcN = state->pgcN - 1; /* -1 b/c pgcN is base1 */
  if (pgcN < 0) {
    fprintf(MSG_OUT, "pgcN < 0");
    return NULL;
  }

  /* get tmap */
  switch(domain) {
  case FP_DOMAIN:
  case VMGM_DOMAIN:
  case VTSM_DOMAIN: {
    if (tmap_count == 0) {
      fprintf(MSG_OUT, "tmap_count == 0");
      return NULL;
    }
    tmap = &tmapt->tmap[0]; /* ASSUME: vmgi only has one time map */
    break;
  }
  case VTS_DOMAIN: {
    if (pgcN >= tmap_count) {
      fprintf(MSG_OUT, "pgcN >= tmap_count; pgcN=%i tmap_count=%i",
          pgcN, tmap_count);
      return NULL;
    }
    tmap = &tmapt->tmap[pgcN];
    break;
  }
  }
  if (tmap == NULL) return NULL;

  /* tmap->tmu is in seconds; convert to millisecs */
  *tmap_interval = tmap->tmu * 1000;
  if (*tmap_interval == 0) {
    fprintf(MSG_OUT, "tmap_interval == 0");
    return NULL;
  }
  *tmap_len = tmap->nr_of_entries;
  if (*tmap_len == 0) {
    fprintf(MSG_OUT, "tmap_len == 0");
    return NULL;
  }
  return tmap;
}

/* Get a sector from a tmap */
static int32_t dvdnav_tmap_get_entry(vts_tmap_t *tmap, uint16_t tmap_len,
            int32_t idx, uint32_t *sector) {
  /* tmaps start at idx 0 which represents a sector at time 1 * tmap_interval
   * this creates a "fake" tmap index at idx -1 for sector 0 */
  if (idx == TMAP_IDX_EDGE_BGN) {
    *sector = 0;
    return 1;
  }
  if (idx < TMAP_IDX_EDGE_BGN || idx >= tmap_len) {
    fprintf(MSG_OUT, "idx out of bounds idx=%i %i", idx, tmap_len);
    return 0;
  }
  /* 0x7fffffff unsets discontinuity bit if present */
  *sector = tmap->map_ent[idx] & 0x7fffffff;
  return 1;
}

/* Do a binary search for earlier admap index near find_sector */
static int32_t dvdnav_admap_search(vobu_admap_t *admap, uint32_t admap_len,
            uint32_t find_sector, uint32_t *vobu) {
  int32_t adj = 1;
  int32_t prv_pos = 0;
  int32_t prv_len = admap_len;
  int32_t cur_len = 0;
  int32_t cur_idx = 0;
  uint32_t cur_sector = 0;
  while (1) {
    cur_len = prv_len / 2;
    /* need to add 1 when prv_len == 3 (cur_len shoud go to 2, not 1) */
    if (prv_len % 2 == 1) ++cur_len;
    cur_idx = prv_pos + (cur_len * adj);
    if       (cur_idx < 0)           cur_idx = 0;
    else if  (cur_idx >= admap_len)  cur_idx = admap_len - 1;

    cur_sector = admap->vobu_start_sectors[cur_idx];
    if      (find_sector <  cur_sector) adj = -1;
    else if (find_sector >  cur_sector) adj =  1;
    else if (find_sector == cur_sector) {
      *vobu = cur_idx;
      return 1;
    }
    if (cur_len == 1) {/* no smaller intervals left */
      if (adj == -1) {/* last comparison was greater; take lesser */
          cur_idx -= 1;
          cur_sector = admap->vobu_start_sectors[cur_idx];
      }
      *vobu = cur_idx;
      return 1;
    }
    prv_len = cur_len;
    prv_pos = cur_idx;
  }
}

/* Do a binary search for the earlier tmap entry near find_sector */
static int32_t dvdnav_tmap_search(vts_tmap_t *tmap, uint32_t tmap_len,
            uint32_t find_sector, int32_t *tmap_idx, uint32_t *sector) {
  int32_t adj = 1;
  int32_t prv_pos = 0;
  int32_t prv_len = tmap_len;
  int32_t result = 0;
  int32_t cur_len = 0;
  int32_t cur_idx = 0;
  uint32_t cur_sector = 0;
  while (1) {
    cur_len = prv_len / 2;
    /* need to add 1 when prv_len == 3 (cur_len shoud go to 2, not 1) */
    if (prv_len % 2 == 1) ++cur_len;
    cur_idx = prv_pos + (cur_len * adj);
    if      (cur_idx < 0)         cur_idx = 0;
    else if (cur_idx >= tmap_len) cur_idx = tmap_len - 1;
    cur_sector = 0;
    result = dvdnav_tmap_get_entry(tmap, tmap_len, cur_idx, &cur_sector);
    if (!result) return 0;
    if      (find_sector <  cur_sector) adj = -1;
    else if (find_sector >  cur_sector) adj =  1;
    else if (find_sector == cur_sector) {
      *tmap_idx = cur_idx;
      *sector = cur_sector;
      return 1;
    }
    if (cur_len == 1) {/* no smaller intervals left */
      if (adj == -1) {/* last comparison was greater; take lesser */
        if (cur_idx == 0) { /* fake tmap index for sector 0 */
          cur_idx = TMAP_IDX_EDGE_BGN;
          cur_sector = 0;
        }
        else {
          cur_idx -= 1;
          result = dvdnav_tmap_get_entry(tmap, tmap_len, cur_idx, &cur_sector);
          if (!result) return 0;
        }
      }
      *tmap_idx = cur_idx;
      *sector = cur_sector;
      return 1;
    }
    prv_len = cur_len;
    prv_pos = cur_idx;
  }
}

/* Find the cell for a given time */
static int32_t dvdnav_cell_find(dvdnav_t *this, dvd_state_t *state,
            uint64_t find_val, dvdnav_cell_data_t *cell_data) {
  uint32_t cells_len = 0;
  uint32_t cells_bgn = 0;
  uint32_t cells_end = 0;
  uint32_t cell_idx = 0;
  pgc_t *pgc = NULL;
  int pgN = 0;
  cell_playback_t *cell = NULL;
  int found = 0;

  pgc = state->pgc;
  if (pgc == NULL) return 0;
  cells_len = pgc->nr_of_cells;
  if (cells_len == 0) {
    fprintf(MSG_OUT, "cells_len == 0");
    return 0;
  }

  /* get cells_bgn, cells_end */
  if (this->pgc_based) {
    cells_bgn = 1;
    cells_end = cells_len;
  }
  else {
    pgN = state->pgN;
    cells_bgn = pgc->program_map[pgN - 1]; /* -1 b/c pgN is 1 based? */
    if (pgN < pgc->nr_of_programs) {
      cells_end = pgc->program_map[pgN] - 1;
    }
    else {
      cells_end = cells_len;
    }
  }

  /* search cells */
  for (cell_idx = cells_bgn; cell_idx <= cells_end; cell_idx++) {
    cell = &(pgc->cell_playback[cell_idx - 1]); /* -1 b/c cell is base1 */
    /* if angle block, only consider first angleBlock
     * (others are "redundant" for purpose of search) */
    if ( cell->block_type == BLOCK_TYPE_ANGLE_BLOCK
      && cell->block_mode != BLOCK_MODE_FIRST_CELL) {
      continue;
    }
    cell_data->bgn->sector = cell->first_sector;
    cell_data->end->sector = cell->last_sector;

    /* 90 pts to ms */
    cell_data->end->time += (dvdnav_convert_time(&cell->playback_time) / 90);
    if (  find_val >= cell_data->bgn->time
       && find_val <= cell_data->end->time) {
      found = 1;
      break;
    }
    cell_data->bgn->time = cell_data->end->time;
  }

  /* found cell: set var */
  if (found) {
    cell_data->idx = cell_idx;
  }
  else
    fprintf(MSG_OUT, "cell not found; find=%"PRId64"", find_val);
  return found;
}

/* Given two sectors and a fraction, calc the corresponding vobu */
static int32_t dvdnav_admap_interpolate_vobu(dvdnav_jump_args_t *args,
            dvdnav_pos_data_t *bgn, dvdnav_pos_data_t *end, uint32_t fraction,
            uint32_t *jump_sector) {
  int32_t result = 0;
  uint32_t vobu_len = 0;
  uint32_t vobu_adj = 0;
  uint32_t vobu_idx = 0;

  /* get bgn->vobu_idx */
  result = dvdnav_admap_search(args->admap, args->admap_len,
      bgn->sector, &bgn->vobu_idx);
  if (!result) {
    fprintf(MSG_OUT, "admap_interpolate: could not find sector_bgn");
    return 0;
  }

  /* get end->vobu_idx */
  result = dvdnav_admap_search(args->admap, args->admap_len,
      end->sector, &end->vobu_idx);
  if (!result) {
    fprintf(MSG_OUT, "admap_interpolate: could not find sector_end");
    return 0;
  }

  vobu_len = end->vobu_idx - bgn->vobu_idx;
  /* +500 to round up else 74% of a 4 sec interval = 2 sec */
  vobu_adj = ((fraction * vobu_len) + 500) / 1000;
  /* HACK: need to add +1, or else will land too soon (not sure why) */
  vobu_adj++;
  vobu_idx = bgn->vobu_idx + vobu_adj;
  if (vobu_idx >= args->admap_len) {
    fprintf(MSG_OUT, "admap_interpolate: vobu_idx >= admap_len");
    return 0;
  }
  *jump_sector = args->admap->vobu_start_sectors[vobu_idx];
  return 1;
}

/* Given two tmap entries and a time, calc the time for the lo tmap entry */
static int32_t dvdnav_tmap_calc_time_for_tmap_entry(dvdnav_jump_args_t *args,
            dvdnav_pos_data_t *lo, dvdnav_pos_data_t *hi,
            dvdnav_pos_data_t *pos, uint64_t *out_time) {
  int32_t result = 0;
  uint32_t vobu_pct = 0;
  uint64_t time_adj = 0;

  if (lo->sector == hi->sector) {
    fprintf(MSG_OUT, "lo->sector == hi->sector: %i", lo->sector);
    return 0;
  }

  /* get vobus corresponding to lo, hi, pos */
  result = dvdnav_admap_search(args->admap, args->admap_len,
      lo->sector, &lo->vobu_idx);
  if (!result) {
    fprintf(MSG_OUT, "lo->vobu: lo->sector=%i", lo->sector);
    return 0;
  }
  result = dvdnav_admap_search(args->admap, args->admap_len,
      hi->sector, &hi->vobu_idx);
  if (!result) {
    fprintf(MSG_OUT, "hi->vobu: hi->sector=%i", hi->sector);
    return 0;
  }
  result = dvdnav_admap_search(args->admap, args->admap_len,
      pos->sector, &pos->vobu_idx);
  if (!result) {
    fprintf(MSG_OUT, "pos->vobu: pos->sector=%i", pos->sector);
    return 0;
  }

  /* calc position of cell relative to lo */
  vobu_pct = ((pos->vobu_idx - lo->vobu_idx) * 1000)
            / ( hi->vobu_idx - lo->vobu_idx);
  if (vobu_pct < 0 || vobu_pct > 1000) {
    fprintf(MSG_OUT, "vobu_pct must be between 0 and 1000");
    return 0;
  }

  /* calc time of lo */
  time_adj = (uint64_t)((args->tmap_interval * vobu_pct) / 1000);
  *out_time = pos->time - time_adj;
  return 1;
}

/* Find the tmap entries on either side of a given sector */
static int32_t dvdnav_tmap_get_entries_for_sector(dvdnav_t *this,
            dvd_state_t *state, dvdnav_jump_args_t *args,
            dvdnav_cell_data_t *cell_data, uint32_t find_sector,
            dvdnav_pos_data_t *lo, dvdnav_pos_data_t *hi) {
  int32_t result = 0;

  result = dvdnav_tmap_search(args->tmap, args->tmap_len, find_sector,
      &lo->tmap_idx, &lo->sector);
  if (!result) {
    fprintf(MSG_OUT, "could not find lo idx: %i", find_sector);
    return 0;
  }

  /* HACK: Most DVDs have a tmap that starts at sector 0
   * However, some have initial dummy cells that are not seekable
   * (restricted = y).
   * These cells will throw off the tmap calcs when in the first playable cell.
   * For now, assume that lo->sector is equal to the cell->bgn->sector
   * Note that for most DVDs this will be 0
   * (Since they will have no dummy cells and cell 1 will start at sector 0)
   */
  if (lo->tmap_idx == TMAP_IDX_EDGE_BGN) {
    lo->sector = cell_data->bgn->sector;
  }

  if (lo->tmap_idx == args->tmap_len - 1) {
    /* lo is last tmap entry; "fake" entry for one beyond
     * and mark it with cell_end_sector */
    hi->tmap_idx = TMAP_IDX_EDGE_END;
    hi->sector = cell_data->end->sector;
  }
  else {
    hi->tmap_idx = lo->tmap_idx + 1;
    result = dvdnav_tmap_get_entry(args->tmap, args->tmap_len,
        hi->tmap_idx, &hi->sector);
    if (!result) {
      fprintf(MSG_OUT, "could not find hi idx: %i", find_sector);
      return 0;
    }
  }
  return 1;
}

/* Find the nearest vobu by using the tmap */
static int32_t dvdnav_find_vobu_by_tmap(dvdnav_t *this, dvd_state_t *state,
            dvdnav_jump_args_t *args, dvdnav_cell_data_t *cell_data,
            dvdnav_pos_data_t *jump) {
  uint64_t seek_offset = 0;
  uint32_t seek_idx = 0;
  int32_t result = 0;
  dvdnav_pos_data_t *cell_bgn_lo = NULL;
  dvdnav_pos_data_t *cell_bgn_hi = NULL;
  dvdnav_pos_data_t *jump_lo = NULL;
  dvdnav_pos_data_t *jump_hi = NULL;

  /* get tmap, tmap_len, tmap_interval */
  args->tmap = dvdnav_tmap_get(this, state,
      &args->tmap_len, &args->tmap_interval);
  if (args->tmap == NULL) return 0;

  /* get tmap entries on either side of cell_bgn */
  cell_bgn_lo = &(dvdnav_pos_data_t){0};
  cell_bgn_hi = &(dvdnav_pos_data_t){0};
  result = dvdnav_tmap_get_entries_for_sector(this, state, args, cell_data,
      cell_data->bgn->sector, cell_bgn_lo, cell_bgn_hi);
  if (!result) return 0;

  /* calc time of cell_bgn_lo */
  result = dvdnav_tmap_calc_time_for_tmap_entry(args, cell_bgn_lo, cell_bgn_hi,
      cell_data->bgn, &cell_bgn_lo->time);
  if (!result) return 0;

  /* calc time of jump_time relative to cell_bgn_lo */
  seek_offset = jump->time - cell_bgn_lo->time;
  seek_idx = (uint32_t)(seek_offset / args->tmap_interval);
  uint32_t seek_remainder = seek_offset - (seek_idx * args->tmap_interval);
  uint32_t seek_pct = (seek_remainder * 1000) / args->tmap_interval;

  /* get tmap entries on either side of jump_time */
  jump_lo = &(dvdnav_pos_data_t){0};
  jump_hi = &(dvdnav_pos_data_t){0};

  /* if seek_idx == 0, then tmap_indexes are the same, do not re-get
   * also, note cell_bgn_lo will already have sector if TMAP_IDX_EDGE_BGN */
  if (seek_idx == 0) {
    jump_lo = cell_bgn_lo;
    jump_hi = cell_bgn_hi;
  }
  else {
    jump_lo->tmap_idx = (uint32_t)(cell_bgn_lo->tmap_idx + seek_idx);
    result = dvdnav_tmap_get_entry(args->tmap, args->tmap_len,
        jump_lo->tmap_idx, &jump_lo->sector);
    if (!result) return 0;

    /* +1 handled by dvdnav_tmap_get_entry */
    jump_hi->tmap_idx = jump_lo->tmap_idx + 1;
    result = dvdnav_tmap_get_entry(args->tmap, args->tmap_len,
        jump_hi->tmap_idx, &jump_hi->sector);
    if (!result) return 0;
  }

  /* interpolate sector */
  result = dvdnav_admap_interpolate_vobu(args, jump_lo, jump_hi,
      seek_pct, &jump->sector);

  return result;
}

/* Find the nearest vobu by using the cell boundaries */
static int32_t dvdnav_find_vobu_by_cell_boundaries(dvdnav_t *this,
            dvdnav_jump_args_t *args, dvdnav_cell_data_t *cell_data,
            dvdnav_pos_data_t *jump) {
  uint64_t jump_offset = 0;
  uint64_t cell_len = 0;
  uint32_t jump_pct = 0;
  int32_t result = 0;

  /* get jump_offset */
  jump_offset = jump->time - cell_data->bgn->time;
  if (jump_offset < 0) {
    fprintf(MSG_OUT, "jump_offset < 0");
    return 0;
  }
  cell_len = cell_data->end->time - cell_data->bgn->time;
  if (cell_len < 0) {
    fprintf(MSG_OUT, "cell_len < 0");
    return 0;
  }
  jump_pct = (jump_offset * 1000) / cell_len;

  /* get sector */
  /* NOTE: end cell sector in VTS_PGC is last sector of cell
   * this last sector is not the start of a VOBU
   * +1 to get sector that is the start of a VOBU
   * start of a VOBU is needed in order to index into admap */
  cell_data->end->sector += 1;
  result = dvdnav_admap_interpolate_vobu(args,
      cell_data->bgn, cell_data->end, jump_pct, &jump->sector);
  if (!result) {
    fprintf(MSG_OUT, "find_by_admap.interpolate");
    return 0;
  }
  return 1;
}

/* Jump to sector by time */
/* NOTE: Mode is currently unimplemented. Only 0 should be passed. */
/* 1 and -1 are for future implementation */
/*  0: Default. Jump to a time which may be either <> time_in_pts_ticks */
/*  1: After. Always jump to a time that is > time_in_pts_ticks */
/* -1: Before. Always jump to a time that is < time_in_pts_ticks */
dvdnav_status_t dvdnav_jump_to_sector_by_time(dvdnav_t *this,
            uint64_t time_in_pts_ticks, int32_t mode) {
  if (mode != JUMP_MODE_TIME_DEFAULT) return DVDNAV_STATUS_ERR;
  int32_t result = DVDNAV_STATUS_ERR;
  dvd_state_t *state = NULL;
  uint32_t sector_off = 0;
  dvdnav_pos_data_t *jump = NULL;
  dvdnav_cell_data_t *cell_data = NULL;
  dvdnav_jump_args_t *args = NULL;

  jump = &(dvdnav_pos_data_t){0};
  /* convert time to milliseconds */
  jump->time = time_in_pts_ticks / 90;

  /* get variables that will be used across both functions */
  state = &(this->vm->state);
  if (state == NULL) goto exit;

  /* get cell info */
  cell_data = &(dvdnav_cell_data_t){0};
  cell_data->bgn = &(dvdnav_pos_data_t){0};
  cell_data->end = &(dvdnav_pos_data_t){0};
  result = dvdnav_cell_find(this, state, jump->time, cell_data);
  if (!result) goto exit;

  /* get admap */
  args = &(dvdnav_jump_args_t){0};
  args->admap = dvdnav_admap_get(this, state, &args->admap_len);
  if (args->admap == NULL) goto exit;

  /* find sector */
  result = dvdnav_find_vobu_by_tmap(this, state, args, cell_data, jump);
  if (!result) {/* bad tmap; interpolate over cell */
    result = dvdnav_find_vobu_by_cell_boundaries(this, args, cell_data, jump);
    if (!result) {
      goto exit;
    }
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: seeking to time=%lu\n", jump->time);
  fprintf(MSG_OUT, "libdvdnav: Before cellN=%u blockN=%u\n", state->cellN, state->blockN);
#endif

  /* jump to sector */
  sector_off = jump->sector - cell_data->bgn->sector;
  this->cur_cell_time = 0;
  if (vm_jump_cell_block(this->vm, cell_data->idx, sector_off)) {
    pthread_mutex_lock(&this->vm_lock);
    this->vm->hop_channel += HOP_SEEK;
    pthread_mutex_unlock(&this->vm_lock);
    result = DVDNAV_STATUS_OK;
  }

#ifdef LOG_DEBUG
  fprintf(MSG_OUT, "libdvdnav: After cellN=%u blockN=%u\n", state->cellN, state->blockN);
#endif

exit:
  return result;
}
