/*
 * Copyright (C) 2008 Julian Scheel
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * nal.c: nal-structure utility functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nal.h"

#include "utils/fastmemcpy.h"

struct nal_buffer* create_nal_buffer(uint8_t max_size)
{
    struct nal_buffer *nal_buffer = (struct nal_buffer *)calloc(1, sizeof(struct nal_buffer));
    nal_buffer->max_size = max_size;

    return nal_buffer;
}

/**
 * destroys a nal buffer. all referenced nals are released
 */
void free_nal_buffer(struct nal_buffer *nal_buffer)
{
  struct nal_unit *nal = nal_buffer->first;

  if (nal) {
    while (nal) {
      struct nal_unit *del = nal;
      nal = nal->next;
      release_nal_unit(del);
    }
  }

  free(nal_buffer);
}

/**
 * appends a nal unit to the end of the buffer
 */
void nal_buffer_append(struct nal_buffer *nal_buffer, struct nal_unit *nal)
{
  if(nal_buffer->used == nal_buffer->max_size) {
    nal_buffer_remove(nal_buffer, nal_buffer->first);
  }

  if (nal_buffer->first == NULL) {
    nal_buffer->first = nal_buffer->last = nal;
    nal->prev = nal->next = NULL;

    lock_nal_unit(nal);
    nal_buffer->used++;
  } else if (nal_buffer->last != NULL) {
    nal_buffer->last->next = nal;
    nal->prev = nal_buffer->last;
    nal_buffer->last = nal;

    lock_nal_unit(nal);
    nal_buffer->used++;
  } else {
    printf("ERR: nal_buffer is in a broken state\n");
  }
}

void nal_buffer_remove(struct nal_buffer *nal_buffer, struct nal_unit *nal)
{
  if (nal == nal_buffer->first && nal == nal_buffer->last) {
    nal_buffer->first = nal_buffer->last = NULL;
  } else {
    if (nal == nal_buffer->first) {
      nal_buffer->first = nal->next;
      nal_buffer->first->prev = NULL;
    } else {
      nal->prev->next = nal->next;
    }

    if (nal == nal_buffer->last) {
      nal_buffer->last = nal->prev;
      nal_buffer->last->next = NULL;
    } else {
      nal->next->prev = nal->prev;
    }
  }

  nal->next = nal->prev = NULL;
  release_nal_unit(nal);

  nal_buffer->used--;
}

void nal_buffer_flush(struct nal_buffer *nal_buffer)
{
  while(nal_buffer->used > 0) {
    nal_buffer_remove(nal_buffer, nal_buffer->first);
  }
}

/**
 * returns the last element in the buffer
 */
struct nal_unit *nal_buffer_get_last(struct nal_buffer *nal_buffer)
{
  return nal_buffer->last;
}

/**
 * get a nal unit from a nal_buffer from it's
 * seq parameter_set_id
 */
struct nal_unit* nal_buffer_get_by_sps_id(struct nal_buffer *nal_buffer,
    uint32_t seq_parameter_set_id)
{
  struct nal_unit *nal = nal_buffer->last;

  if (nal != NULL) {
    do {
      if(nal->nal_unit_type == NAL_SPS) {
        if(nal->sps.seq_parameter_set_id == seq_parameter_set_id) {
          return nal;
        }
      }

      nal = nal->prev;
    } while(nal != NULL);
  }

  return NULL;
}

/**
 * get a nal unit from a nal_buffer from it's
 * pic parameter_set_id
 */
struct nal_unit* nal_buffer_get_by_pps_id(struct nal_buffer *nal_buffer,
    uint32_t pic_parameter_set_id)
{
  struct nal_unit *nal = nal_buffer->last;

  if (nal != NULL) {
    do {
      if(nal->nal_unit_type == NAL_PPS) {
        if(nal->pps.pic_parameter_set_id == pic_parameter_set_id) {
          return nal;
        }
      }

      nal = nal->prev;
    } while(nal != NULL);
  }

  return NULL;
}

/**
 * create a new nal unit, with a lock_counter of 1
 */
struct nal_unit* create_nal_unit()
{
  struct nal_unit *nal = (struct nal_unit *)calloc(1, sizeof(struct nal_unit));
  nal->lock_counter = 1;

  return nal;
}

void lock_nal_unit(struct nal_unit *nal)
{
  nal->lock_counter++;
}

void release_nal_unit(struct nal_unit *nal)
{
  if(!nal)
    return;

  nal->lock_counter--;

  if(nal->lock_counter <= 0) {
    free(nal);
  }
}

/**
 * creates a copy of a nal unit with a single lock
 */
void copy_nal_unit(struct nal_unit *dest, struct nal_unit *src)
{
  /* size without pps, sps and slc units: */
  int size = sizeof(struct nal_unit);

  fast_memcpy(dest, src, size);
  dest->lock_counter = 1;
  dest->prev = dest->next = NULL;
}
