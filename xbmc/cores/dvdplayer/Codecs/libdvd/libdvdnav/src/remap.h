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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: remap.h 1135 2008-09-06 21:55:51Z rathann $
 */

#ifndef __REMAP__H
#define __REMAP__H
typedef struct block_s block_t;

typedef struct remap_s remap_t;

remap_t* remap_loadmap( char *title);

unsigned long remap_block(
	remap_t *map, int domain, int title, int program,
	unsigned long cblock, unsigned long offset);

#endif
