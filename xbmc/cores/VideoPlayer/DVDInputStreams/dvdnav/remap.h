/*
 *  This file is part of libdvdnav, a DVD navigation library.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

typedef struct block_s block_t;

typedef struct remap_s remap_t;

remap_t* remap_loadmap( char *title);

unsigned long remap_block(
	remap_t *map, int domain, int title, int program,
	unsigned long cblock, unsigned long offset);

