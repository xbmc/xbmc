/*
 * Copyright (C) 2000, 2001, 2002 Håkan Hjort <d95hjort@dtek.chalmers.se>.
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

#ifndef LIBDVDREAD_BITREADER_H
#define LIBDVDREAD_BITREADER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t *start;
  uint32_t byte_position;
  uint32_t bit_position;
  uint8_t byte;
} getbits_state_t;

int dvdread_getbits_init(getbits_state_t *state, uint8_t *start);
uint32_t dvdread_getbits(getbits_state_t *state, uint32_t number_of_bits);

#ifdef __cplusplus
};
#endif
#endif /* LIBDVDREAD_BITREADER_H */
