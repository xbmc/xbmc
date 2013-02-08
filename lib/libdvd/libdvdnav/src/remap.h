/*
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

#ifndef LIBDVDNAV_REMAP_H
#define LIBDVDNAV_REMAP_H
typedef struct block_s block_t;

typedef struct remap_s remap_t;

remap_t* remap_loadmap( char *title);

unsigned long remap_block(
	remap_t *map, int domain, int title, int program,
	unsigned long cblock, unsigned long offset);

#endif /* LIBDVDNAV_REMAP_H */
