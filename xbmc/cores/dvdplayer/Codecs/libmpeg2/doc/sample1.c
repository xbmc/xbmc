/*
 * sample1.c
 * Copyright (C) 2003      Regis Duchesne <hpreg@zoy.org>
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
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
 *
 * This program reads a MPEG-2 stream, and saves each of its frames as
 * an image file using the PGM format (black and white).
 *
 * It demonstrates how to use the following features of libmpeg2:
 * - Output buffers use the YUV 4:2:0 planar format.
 * - Output buffers are allocated and managed by the library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "mpeg2.h"

static void save_pgm (int width, int height,
		      int chroma_width, int chroma_height,
		      uint8_t * const * buf, int num)
{
    char filename[100];
    FILE * pgmfile;
    int i;
    static uint8_t black[16384] = { 0 };

    sprintf (filename, "%d.pgm", num);
    pgmfile = fopen (filename, "wb");
    if (!pgmfile) {
	fprintf (stderr, "Could not open file \"%s\".\n", filename);
	exit (1);
    }
    fprintf (pgmfile, "P5\n%d %d\n255\n",
	     2 * chroma_width, height + chroma_height);
    for (i = 0; i < height; i++) {
	fwrite (buf[0] + i * width, width, 1, pgmfile);
	fwrite (black, 2 * chroma_width - width, 1, pgmfile);
    }
    for (i = 0; i < chroma_height; i++) {
	fwrite (buf[1] + i * chroma_width, chroma_width, 1, pgmfile);
	fwrite (buf[2] + i * chroma_width, chroma_width, 1, pgmfile);
    }
    fclose (pgmfile);
}

static void sample1 (FILE * mpgfile)
{
#define BUFFER_SIZE 4096
    uint8_t buffer[BUFFER_SIZE];
    mpeg2dec_t * decoder;
    const mpeg2_info_t * info;
    const mpeg2_sequence_t * sequence;
    mpeg2_state_t state;
    size_t size;
    int framenum = 0;

    decoder = mpeg2_init ();
    if (decoder == NULL) {
	fprintf (stderr, "Could not allocate a decoder object.\n");
	exit (1);
    }
    info = mpeg2_info (decoder);

    size = (size_t)-1;
    do {
	state = mpeg2_parse (decoder);
	sequence = info->sequence;
	switch (state) {
	case STATE_BUFFER:
	    size = fread (buffer, 1, BUFFER_SIZE, mpgfile);
	    mpeg2_buffer (decoder, buffer, buffer + size);
	    break;
	case STATE_SLICE:
	case STATE_END:
	case STATE_INVALID_END:
	    if (info->display_fbuf)
		save_pgm (sequence->width, sequence->height,
			  sequence->chroma_width, sequence->chroma_height,
			  info->display_fbuf->buf, framenum++);
	    break;
	default:
	    break;
	}
    } while (size);

    mpeg2_close (decoder);
}

int main (int argc, char ** argv)
{
    FILE * mpgfile;

    if (argc > 1) {
	mpgfile = fopen (argv[1], "rb");
	if (!mpgfile) {
	    fprintf (stderr, "Could not open file \"%s\".\n", argv[1]);
	    exit (1);
	}
    } else
	mpgfile = stdin;

    sample1 (mpgfile);

    return 0;
}
