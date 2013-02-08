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

#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include "dvdnav/dvdnav.h"
#include <dvdread/nav_types.h>
#include <dvdread/ifo_types.h>
#include "remap.h"
#include "vm/decoder.h"
#include "vm/vm.h"
#include "dvdnav_internal.h"

/* Characteristics/setting API calls */

dvdnav_status_t dvdnav_get_region_mask(dvdnav_t *this, int32_t *region) {
  (*region) = this->vm->state.registers.SPRM[20];
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_set_region_mask(dvdnav_t *this, int32_t mask) {
  pthread_mutex_lock(&this->vm_lock);
  this->vm->state.registers.SPRM[20] = (mask & 0xff);
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_set_readahead_flag(dvdnav_t *this, int32_t use_readahead) {
  this->use_read_ahead = use_readahead;
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_readahead_flag(dvdnav_t *this, int32_t *flag) {
  (*flag) = this->use_read_ahead;
  return DVDNAV_STATUS_OK;
}

static dvdnav_status_t set_language_register(dvdnav_t *this, char *code, int reg) {
  if(!code[0] || !code[1]) {
    printerr("Passed illegal language code.");
    return DVDNAV_STATUS_ERR;
  }

  pthread_mutex_lock(&this->vm_lock);
  this->vm->state.registers.SPRM[reg] = (code[0] << 8) | code[1];
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_menu_language_select(dvdnav_t *this, char *code) {
  return set_language_register(this, code, 0);
}

dvdnav_status_t dvdnav_audio_language_select(dvdnav_t *this, char *code) {
  return set_language_register(this, code, 16);
}

dvdnav_status_t dvdnav_spu_language_select(dvdnav_t *this, char *code) {
  return set_language_register(this, code, 18);
}

dvdnav_status_t dvdnav_set_PGC_positioning_flag(dvdnav_t *this, int32_t pgc) {
  this->pgc_based = pgc;
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_PGC_positioning_flag(dvdnav_t *this, int32_t *flag) {
  (*flag) = this->pgc_based;
  return DVDNAV_STATUS_OK;
}
