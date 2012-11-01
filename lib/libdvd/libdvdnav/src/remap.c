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
 * $Id: remap.c 1135 2008-09-06 21:55:51Z rathann $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <sys/param.h>
#else
#ifndef MAXPATHLEN
#define MAXPATHLEN 255
#endif
#endif /* _MSC_VER */

#include <inttypes.h>
#include <limits.h>
#include <sys/time.h>
#include "dvd_types.h"
#include <dvdread/nav_types.h>
#include <dvdread/ifo_types.h>
#include "remap.h"
#include "vm/decoder.h"
#include "vm/vm.h"
#include "dvdnav.h"
#include "dvdnav_internal.h"

struct block_s {
    int domain;
    int title;
    int program;
    unsigned long start_block;
    unsigned long end_block;
};

struct remap_s {
    char *title;
    int maxblocks;
    int nblocks;
    int debug;
    struct block_s *blocks;
};

static remap_t* remap_new( char *title) {
    remap_t *map = malloc( sizeof(remap_t));
    map->title = strdup(title);
    map->maxblocks = 0;
    map->nblocks = 0;
    map->blocks = NULL;
    map->debug = 0;
    return map;
}

static int compare_block( block_t *a, block_t *b) {
    /* returns -1 if a precedes b, 1 if a follows b, and 0 if a and b overlap */
    if (a->domain < b->domain) {
	return -1;
    } else if (a->domain > b->domain) {
	return 1;
    }

    if (a->title < b->title) {
	return -1;
    } else if (a->title > b->title) {
	return 1;
    }

    if (a->program < b->program) {
	return -1;
    } else if (a->program > b->program) {
	return 1;
    }

    if (a->end_block < b->start_block) {
	return -1;
    } else if (a->start_block > b->end_block) {
	/*
	 * if a->start_block == b->end_block then the two regions
	 * aren't strictly overlapping, but they should be merged
	 * anyway since there are zero blocks between them
	 */
	return 1;
    }

    return 0;
}

static block_t *findblock( remap_t *map, block_t *key) {
    int lb = 0;
    int ub = map->nblocks - 1;
    int mid;
    int res;

    while (lb <= ub) {
	mid = lb + (ub - lb)/2;
	res = compare_block( key, &map->blocks[mid]);
	if (res < 0) {
	    ub = mid-1;
	} else if (res > 0) {
	    lb = mid+1;
	} else {
	    return &map->blocks[mid];
	}
    }
    return NULL;
}

static void mergeblock( block_t *b, block_t tmp) {
    if (tmp.start_block < b->start_block) b->start_block = tmp.start_block;
    if (tmp.end_block > b->end_block) b->end_block = tmp.end_block;
}

static void remap_add_node( remap_t *map, block_t block) {
    block_t *b;
    int n;
    b = findblock( map, &block);
    if (b) {
	/* overlaps an existing block */
	mergeblock( b, block);
    } else {
        /* new block */
	if (map->nblocks >= map->maxblocks) {
	    map->maxblocks += 20;
	    map->blocks = realloc( map->blocks, sizeof( block_t)*map->maxblocks);
	}
	n = map->nblocks++;
	while (n > 0 && compare_block( &block, &map->blocks[ n-1]) < 0) {
	    map->blocks[ n] = map->blocks[ n-1];
	    n--;
	}
	map->blocks[ n] = block;
    }
}

static int parseblock(char *buf, int *dom, int *tt, int *pg,
		      unsigned long *start, unsigned long *end) {
    long tmp;
    char *tok;
    char *epos;
    char *marker[]={"domain", "title", "program", "start", "end"};
    int st = 0;
    tok = strtok( buf, " ");
    while (st < 5) {
        if (strcmp(tok, marker[st])) return -st-1000;
        tok = strtok( NULL, " ");
        if (!tok) return -st-2000;
        tmp = strtol( tok, &epos, 0);
        if (*epos != 0 && *epos != ',') return -st-3000;
        switch (st) {
	    case 0:
		*dom = (int)tmp;
		break;
	    case 1:
		*tt = (int)tmp;
		break;
	    case 2:
		*pg = (int)tmp;
		break;
	    case 3:
		*start = tmp;
		break;
	    case 4:
		*end = tmp;
		break;
	}
	st++;
        tok = strtok( NULL, " ");
    }
    return st;
}

remap_t* remap_loadmap( char *title) {
    char buf[160];
    char fname[MAXPATHLEN];
    char *home;
    int res;
    FILE *fp;
    block_t tmp;
    remap_t *map;

    memset(&tmp, 0, sizeof(tmp));
    /* Build the map filename */
    home = getenv("HOME");
    if(!home) {
        fprintf(MSG_OUT, "libdvdnav: Unable to find home directory" );
        return NULL;
    }
    snprintf(fname, sizeof(fname), "%s/.dvdnav/%s.map", home, title);

    /* Open the map file */
    fp = fopen( fname, "r");
    if (!fp) {
	fprintf(MSG_OUT, "libdvdnav: Unable to find map file '%s'\n", fname);
	return NULL;
    }

    /* Load the map file */
    map = remap_new( title);
    while (fgets( buf, sizeof(buf), fp) != NULL) {
        if (buf[0] == '\n' || buf[0] == '#' || buf[0] == 0) continue;
        if (strncasecmp( buf, "debug", 5) == 0) {
	    map->debug = 1;
	} else {
	    res = parseblock( buf,
		&tmp.domain, &tmp.title, &tmp.program, &tmp.start_block, &tmp.end_block);
	    if (res != 5) {
		fprintf(MSG_OUT, "libdvdnav: Ignoring map line (%d): %s\n", res, buf);
		continue;
	    }
	    remap_add_node( map, tmp);
	}
    }
    fclose(fp);

    if (map->nblocks == 0 && map->debug == 0) {
        free(map);
        return NULL;
    }
    return map;
}

unsigned long remap_block(
	remap_t *map, int domain, int title, int program,
	unsigned long cblock, unsigned long offset)
{
    block_t key;
    block_t *b;

    if (map->debug) {
	fprintf(MSG_OUT, "libdvdnav: %s: domain %d, title %d, program %d, start %lx, next %lx\n",
	    map->title, domain, title, program, cblock, cblock+offset);
    }

    key.domain = domain;
    key.title = title;
    key.program = program;
    key.start_block = key.end_block = cblock + offset;
    b = findblock( map, &key);

    if (b) {
       if (map->debug) {
	   fprintf(MSG_OUT, "libdvdnav: Redirected to %lx\n", b->end_block);
       }
       return b->end_block - cblock;
    }
    return offset;
}
