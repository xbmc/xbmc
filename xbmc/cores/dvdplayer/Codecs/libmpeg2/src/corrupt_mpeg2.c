/*
 * corrupt_mpeg2.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#ifdef HAVE_IO_H
#include <fcntl.h>
#include <io.h>
#endif
#include <inttypes.h>

static FILE * in_file;
static FILE * seed_file;
static int seed_loaded = 0;
static uint32_t rsl[55];
static int rsl_i = -1;

typedef struct {
    uint32_t p, q[8], r;
} randbyte_t;

#define CORRUPT_RANDOM 0
#define CORRUPT_VALUE 1

typedef struct corrupt_s {
    int type;
    int chunk_start, chunk_stop;
    int bit_start, bit_stop;
    union {
	randbyte_t prob;
    } u;
    struct corrupt_s * next;
    uint8_t mask;
} corrupt_t;

#define CORRUPT_LIST_SIZE 10
static corrupt_t corrupt_list[10];
static int corrupt_list_index = 0;
static corrupt_t * corrupt_head = NULL;
static int current_chunk = -1, current_bit = 0, target_bit = 0x7fffffff;

static inline uint32_t fastrand (void)
{
    if (++rsl_i == 55) rsl_i = 0;
    return rsl[rsl_i] += rsl[(rsl_i < 31) ? rsl_i + 24 : rsl_i - 31];
}

static uint32_t clip (double p)
{
    return (p < 0) ? 0 : ((p >= 1) ? 0xffffffff : (uint32_t)(p*4294967296.0));
}

static void randbyte_init (double p, randbyte_t * rnd)
{
    double q, r;
    int i;

    rnd->p = clip (p);
    r = 1;
    for (i = 0; i < 8; i++) {
	r *= 1 - p;
	q = p / (1 - r);
	rnd->q[i] = clip (q);
    }
    rnd->r = clip (1 - r);
}

static inline uint8_t randbyte (const randbyte_t * const rnd)
{
    int i, j;

    if (fastrand () > rnd->r || rnd->r == 0)
	return 0;

    i = 7; j = 0;
    do
	if (fastrand () <= (j ? rnd->p : rnd->q[i]))
	    j |= 1 << i;
    while (i--);
    return j;
}

static void print_usage (char ** argv)
{
    fprintf (stderr, "usage: %s [-h] [-l <seed>] [-s <seedfile>] \\\n"
	     "\t\t[-r prob[,restrict] [-v prob[,restrict]] <file>\n"
	     "\t-h\tdisplay help\n"
	     "\t-l load seed\n"
	     "\t-s save seed file\n"
	     "\t-r random corruption\n"
	     "\t-v random value\n"
	     "restrict: chunk[-endchunk][,bit[-endbit]]\n", argv[0]);

    exit (1);
}

static void corrupt_arg (corrupt_t * corrupt, int type, char * s, char ** argv)
{
    corrupt->type = type;
    if (! *s)
	s = (char *)",0-0xff,0-";
    else if (*s != ',' || !isdigit (s[1]))
	print_usage (argv);
    corrupt->chunk_start = strtol (s + 1, &s, 0);
    if (*s != '-')
	corrupt->chunk_stop = corrupt->chunk_start;
    else if (isdigit (* ++s))
	corrupt->chunk_stop = strtol (s, &s, 0);
    else
	print_usage (argv);
    if (! *s)
	s = (char *)",32-";
    else if (*s != ',' || !isdigit (s[1]))
	print_usage (argv);
    corrupt->bit_start = strtol (s + 1, &s, 0);
    if (*s != '-')
	corrupt->bit_stop = corrupt->bit_start;
    else if (isdigit (* ++s))
	corrupt->bit_stop = strtol (s, &s, 0);
    else
	corrupt->bit_stop = 0x7ffffffe;
    if (corrupt->chunk_start < 0 ||
	corrupt->chunk_start > corrupt->chunk_stop ||
	corrupt->chunk_stop >= 0x1000 ||
	corrupt->bit_start < 0 || corrupt->bit_start > corrupt->bit_stop || *s)
	print_usage (argv);
    if (corrupt->chunk_stop < 0x100) {
	corrupt->chunk_start <<= 4;
	corrupt->chunk_stop = (corrupt->chunk_stop << 4) | 0xf;
    }
}

static void handle_args (int argc, char ** argv)
{
    int c;
    double prob;
    char * s;
    corrupt_t * corrupt;

    while ((c = getopt (argc, argv, "hl:s:r:v:")) != -1)
	switch (c) {
	case 'l':
	    if (seed_file || seed_loaded)
		print_usage (argv);
	    if (sscanf (optarg, "%08x%08x%08x%08x",
			rsl, rsl+1, rsl+2, rsl+3) != 4)
		print_usage (argv);
	    seed_loaded = 1;
	    break;

	case 's':
	    if (seed_file || seed_loaded)
		print_usage (argv);
	    seed_file = fopen (optarg, "wt");
	    if (!seed_file)
		print_usage (argv);
	    break;

	case 'r':
	    prob = strtod (optarg, &s);
	    if (prob < 0 || prob > 1 || 
		corrupt_list_index == CORRUPT_LIST_SIZE)
		print_usage (argv);
	    corrupt = corrupt_list + corrupt_list_index++;
	    corrupt_arg (corrupt, CORRUPT_RANDOM, s, argv);
	    randbyte_init (prob, &corrupt->u.prob);
	    break;

	case 'v':
	    prob = strtod (optarg, &s);
	    if (prob < 0 || prob > 1 || 
		corrupt_list_index == CORRUPT_LIST_SIZE)
		print_usage (argv);
	    corrupt = corrupt_list + corrupt_list_index++;
	    corrupt_arg (corrupt, CORRUPT_VALUE, s, argv);
	    randbyte_init (prob, &corrupt->u.prob);
	    break;

	default:
	    print_usage (argv);
	}

    if (optind < argc) {
	in_file = fopen (argv[optind], "rb");
	if (!in_file) {
	    fprintf (stderr, "%s - could not open file %s\n", strerror (errno),
		     argv[optind]);
	    exit (1);
	}
    } else
	in_file = stdin;

    if (!seed_file && !seed_loaded)
	seed_file = fopen ("seed", "wt");
}

static void update_corrupt_list (void)
{
    corrupt_t * corrupt;
    corrupt_t ** corrupt_link;

    corrupt_link = &corrupt_head;
    target_bit = 0x7fffffff;
    for (corrupt = corrupt_list;
	 corrupt < corrupt_list + corrupt_list_index; corrupt++)
	if (corrupt->chunk_start <= current_chunk &&
	    corrupt->chunk_stop >= current_chunk &&
	    corrupt->bit_stop >= current_bit) {
	    if (corrupt->bit_start >= current_bit + 8) {
		if (corrupt->bit_start <= target_bit)
		    target_bit = corrupt->bit_start & ~7;
	    } else {
		*corrupt_link = corrupt;
		corrupt_link = &corrupt->next;
		if (corrupt->bit_stop >= current_bit + 7) {
		    corrupt->mask = 0xff;
		    if (corrupt->bit_stop <= target_bit)
			target_bit = (corrupt->bit_stop + 1) & ~7;
		} else {
		    corrupt->mask = -1 << (7 - (corrupt->bit_stop & 7));
		    target_bit = current_bit + 8;
		}
		if (corrupt->bit_start > current_bit) {
		    corrupt->mask &= 0xff >> (corrupt->bit_start & 7);
		    target_bit = current_bit + 8;
		}
	    }
	}
    *corrupt_link = NULL;
}

static void corrupt (uint8_t * ptr)
{
    corrupt_t * corrupt_ptr;

    if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1) {
	current_chunk = (ptr[3] << 4) | (ptr[4] >> 4);
	current_bit = 0;
	update_corrupt_list ();
    } else if (current_bit == target_bit)
	update_corrupt_list ();

    current_bit += 8;

    for (corrupt_ptr = corrupt_head; corrupt_ptr; corrupt_ptr = corrupt_ptr->next)
	switch (corrupt_ptr->type) {
	case CORRUPT_RANDOM:
	    *ptr ^= randbyte (&corrupt_ptr->u.prob) & corrupt_ptr->mask;
	    break;
	case CORRUPT_VALUE:
	    *ptr = ((*ptr & ~corrupt_ptr->mask) |
		    (randbyte (&corrupt_ptr->u.prob) & corrupt_ptr->mask));
	    break;
	}
}

static void corrupt_loop (void)
{
#define BUFFER_SIZE 4096
    static uint8_t buffer1[BUFFER_SIZE + 4];
    static uint8_t buffer2[BUFFER_SIZE + 4];
    static uint8_t terminator[4] = {0xff, 0xff, 0xff, 0xff};

    uint8_t * buf;
    uint8_t * end;
    uint8_t * current;

    buf = buffer1;
    end = buf + fread (buf, 1, BUFFER_SIZE, in_file);

    while (end == buf + BUFFER_SIZE) {
	uint8_t * lastbuf;
	uint8_t * lastbuf_end;

	lastbuf = buf;	lastbuf_end = buf + BUFFER_SIZE;
	buf = (buf == buffer1) ? buffer2 : buffer1;
	memcpy (buf, terminator, 4);
	end = buf + fread (buf, 1, BUFFER_SIZE, in_file);
	memcpy (lastbuf_end, buf, 4);
	for (current = lastbuf; current < lastbuf_end; current++)
	    corrupt (current);
	fwrite (lastbuf, BUFFER_SIZE, 1, stdout);
    }

    memcpy (end, terminator, 4);
    for (current = buf; current < end; current++)
	corrupt (current);
    fwrite (buf, end - buf, 1, stdout);
}

int main (int argc, char ** argv)
{
    int i;

#ifdef HAVE_IO_H
    setmode (fileno (stdin), O_BINARY);
    setmode (fileno (stdout), O_BINARY);
#endif

    handle_args (argc, argv);

    if (!seed_loaded) {
	srand (time (NULL));
	for (i = 0; i < 4; i++)
	    rsl[i] = rand ();
	fprintf (seed_file, "%08x%08x%08x%08x\n",
		 rsl[0], rsl[1], rsl[2], rsl[3]);
	fclose (seed_file);
    }

    for (i = 4; i < 55; i++)
	rsl[i] = 0;
    for (i = 0; i < 1000; i++)
	fastrand ();

    corrupt_loop ();

    return 0;
}
