/*
 * Copyright (C) 2000, 2001, 2002, 2003 Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "dvdread/bitreader.h"

int dvdread_getbits_init(getbits_state_t *state, uint8_t *start) {
  if ((state == NULL) || (start == NULL)) return 0;
  state->start = start;
  state->bit_position = 0;
  state->byte_position = 0;
  state->byte = start[0];
  return 1;
}

/* Non-optimized getbits. */
/* This can easily be optimized for particular platforms. */
uint32_t dvdread_getbits(getbits_state_t *state, uint32_t number_of_bits) {
  uint32_t result=0;
  uint8_t byte=0;
  if (number_of_bits > 32) {
    printf("Number of bits > 32 in getbits\n");
    abort();
  }

  if ((state->bit_position) > 0) {  /* Last getbits left us in the middle of a byte. */
    if (number_of_bits > (8-state->bit_position)) { /* this getbits will span 2 or more bytes. */
      byte = state->byte;
      byte = byte >> (state->bit_position);
      result = byte;
      number_of_bits -= (8-state->bit_position);
      state->bit_position = 0;
      state->byte_position++;
      state->byte = state->start[state->byte_position];
    } else {
      byte=state->byte;
      state->byte = state->byte << number_of_bits;
      byte = byte >> (8 - number_of_bits);
      result = byte;
      state->bit_position += number_of_bits; /* Here it is impossible for bit_position > 8 */
      if (state->bit_position == 8) {
        state->bit_position = 0;
        state->byte_position++;
        state->byte = state->start[state->byte_position];
      }
      number_of_bits = 0;
    }
  }
  if ((state->bit_position) == 0) {
    while (number_of_bits > 7) {
      result = (result << 8) + state->byte;
      state->byte_position++;
      state->byte = state->start[state->byte_position];
      number_of_bits -= 8;
    }
    if (number_of_bits > 0) { /* number_of_bits < 8 */
      byte = state->byte;
      state->byte = state->byte << number_of_bits;
      state->bit_position += number_of_bits; /* Here it is impossible for bit_position > 7 */
      byte = byte >> (8 - number_of_bits);
      result = (result << number_of_bits) + byte;
      number_of_bits = 0;
    }
  }

  return result;
}

#if 0  /* TODO: optimized versions not yet used */

/* WARNING: This function can only be used on a byte boundary.
            No checks are made that we are in fact on a byte boundary.
 */
uint16_t dvdread_get16bits(getbits_state_t *state) {
  uint16_t result;
  state->byte_position++;
  result = (state->byte << 8) + state->start[state->byte_position++];
  state->byte = state->start[state->byte_position];
  return result;
}

/* WARNING: This function can only be used on a byte boundary.
            No checks are made that we are in fact on a byte boundary.
 */
uint32_t dvdread_get32bits(getbits_state_t *state) {
  uint32_t result;
  state->byte_position++;
  result = (state->byte << 8) + state->start[state->byte_position++];
  result = (result << 8) + state->start[state->byte_position++];
  result = (result << 8) + state->start[state->byte_position++];
  state->byte = state->start[state->byte_position];
  return result;
}

#endif
