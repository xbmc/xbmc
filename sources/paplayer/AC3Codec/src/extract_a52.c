/*
 * extract_a52.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
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
#include <string.h>
#include <errno.h>
#include <getopt.h>
#ifdef HAVE_IO_H
#include <fcntl.h>
#include <io.h>
#endif
#include <inttypes.h>

#define BUFFER_SIZE 4096
static uint8_t buffer[BUFFER_SIZE];
static FILE * in_file;
static int demux_track = 0x80;
static int demux_pid = 0;
static int demux_pes = 0;

static void print_usage (char ** argv)
{
    fprintf (stderr, "usage: %s [-h] [-s <track>] [-t <pid>] <file>\n"
	     "\t-h\tdisplay help\n"
	     "\t-s\tset track number (0-7 or 0x80-0x87)\n"
	     "\t-t\tuse transport stream demultiplexer, pid 0x10-0x1ffe\n"
	     "\t-T\tuse transport stream PES demultiplexer\n",
	     argv[0]);

    exit (1);
}

static void handle_args (int argc, char ** argv)
{
    int c;
    char * s;

    while ((c = getopt (argc, argv, "hs:t:T")) != -1)
	switch (c) {
	case 's':
	    demux_track = strtol (optarg, &s, 0);
	    if (demux_track < 0x80)
		demux_track += 0x80;
	    if (demux_track < 0x80 || demux_track > 0x87 || *s) {
		fprintf (stderr, "Invalid track number: %s\n", optarg);
		print_usage (argv);
	    }
	    break;

	case 't':
	    demux_pid = strtol (optarg, &s, 0);
	    if (demux_pid < 0x10 || demux_pid > 0x1ffe || *s) {
		fprintf (stderr, "Invalid pid: %s\n", optarg);
		print_usage (argv);
	    }
	    break;

	case 'T':
	    demux_pes = 1;
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
}

#define DEMUX_PAYLOAD_START 1
static int demux (uint8_t * buf, uint8_t * end, int flags)
{
    static int mpeg1_skip_table[16] = {
	0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /*
     * the demuxer keeps some state between calls:
     * if "state" = DEMUX_HEADER, then "head_buf" contains the first
     *     "bytes" bytes from some header.
     * if "state" == DEMUX_DATA, then we need to copy "bytes" bytes
     *     of ES data before the next header.
     * if "state" == DEMUX_SKIP, then we need to skip "bytes" bytes
     *     of data before the next header.
     *
     * NEEDBYTES makes sure we have the requested number of bytes for a
     * header. If we dont, it copies what we have into head_buf and returns,
     * so that when we come back with more data we finish decoding this header.
     *
     * DONEBYTES updates "buf" to point after the header we just parsed.
     */

#define DEMUX_HEADER 0
#define DEMUX_DATA 1
#define DEMUX_SKIP 2
    static int state = DEMUX_SKIP;
    static int state_bytes = 0;
    static uint8_t head_buf[268];

    uint8_t * header;
    int bytes;
    int len;

#define NEEDBYTES(x)						\
    do {							\
	int missing;						\
								\
	missing = (x) - bytes;					\
	if (missing > 0) {					\
	    if (header == head_buf) {				\
		if (missing <= end - buf) {			\
		    memcpy (header + bytes, buf, missing);	\
		    buf += missing;				\
		    bytes = (x);				\
		} else {					\
		    memcpy (header + bytes, buf, end - buf);	\
		    state_bytes = bytes + end - buf;		\
		    return 0;					\
		}						\
	    } else {						\
		memcpy (head_buf, header, bytes);		\
		state = DEMUX_HEADER;				\
		state_bytes = bytes;				\
		return 0;					\
	    }							\
	}							\
    } while (0)

#define DONEBYTES(x)		\
    do {			\
	if (header != head_buf)	\
	    buf = header + (x);	\
    } while (0)

    if (flags & DEMUX_PAYLOAD_START)
	goto payload_start;
    switch (state) {
    case DEMUX_HEADER:
	if (state_bytes > 0) {
	    header = head_buf;
	    bytes = state_bytes;
	    goto continue_header;
	}
	break;
    case DEMUX_DATA:
	if (demux_pid || (state_bytes > end - buf)) {
	    fwrite (buf, end - buf, 1, stdout);
	    state_bytes -= end - buf;
	    return 0;
	}
	fwrite (buf, state_bytes, 1, stdout);
	buf += state_bytes;
	break;
    case DEMUX_SKIP:
	if (demux_pid || (state_bytes > end - buf)) {
	    state_bytes -= end - buf;
	    return 0;
	}
	buf += state_bytes;
	break;
    }

    while (1) {
	if (demux_pid) {
	    state = DEMUX_SKIP;
	    return 0;
	}
    payload_start:
	header = buf;
	bytes = end - buf;
    continue_header:
	NEEDBYTES (4);
	if (header[0] || header[1] || (header[2] != 1)) {
	    if (demux_pid) {
		state = DEMUX_SKIP;
		return 0;
	    } else if (header != head_buf) {
		buf++;
		goto payload_start;
	    } else {
		header[0] = header[1];
		header[1] = header[2];
		header[2] = header[3];
		bytes = 3;
		goto continue_header;
	    }
	}
	if (demux_pid || demux_pes) {
	    if (header[3] != 0xbd) {
		fprintf (stderr, "bad stream id %x\n", header[3]);
		exit (1);
	    }
	    NEEDBYTES (9);
	    if ((header[6] & 0xc0) != 0x80) {	/* not mpeg2 */
		fprintf (stderr, "bad multiplex - not mpeg2\n");
		exit (1);
	    }
	    len = 9 + header[8];
	    NEEDBYTES (len);
	    DONEBYTES (len);
	    bytes = 6 + (header[4] << 8) + header[5] - len;
	    if (bytes > end - buf) {
		fwrite (buf, end - buf, 1, stdout);
		state = DEMUX_DATA;
		state_bytes = bytes - (end - buf);
		return 0;
	    } else if (bytes > 0) {
		fwrite (buf, bytes, 1, stdout);
		buf += bytes;
	    }
	} else switch (header[3]) {
	case 0xb9:	/* program end code */
	    /* DONEBYTES (4); */
	    /* break;         */
	    return 1;
	case 0xba:	/* pack header */
	    NEEDBYTES (5);
	    if ((header[4] & 0xc0) == 0x40) {	/* mpeg2 */
		NEEDBYTES (14);
		len = 14 + (header[13] & 7);
		NEEDBYTES (len);
		DONEBYTES (len);
		/* header points to the mpeg2 pack header */
	    } else if ((header[4] & 0xf0) == 0x20) {	/* mpeg1 */
		NEEDBYTES (12);
		DONEBYTES (12);
		/* header points to the mpeg1 pack header */
	    } else {
		fprintf (stderr, "weird pack header\n");
		DONEBYTES (5);
	    }
	    break;
	case 0xbd:	/* private stream 1 */
	    NEEDBYTES (7);
	    if ((header[6] & 0xc0) == 0x80) {	/* mpeg2 */
		NEEDBYTES (9);
		len = 10 + header[8];
		NEEDBYTES (len);
		/* header points to the mpeg2 pes header */
	    } else {	/* mpeg1 */
		len = 7;
		while ((header-1)[len] == 0xff) {
		    len++;
		    NEEDBYTES (len);
		    if (len == 23) {
			fprintf (stderr, "too much stuffing\n");
			break;
		    }
		}
		if (((header-1)[len] & 0xc0) == 0x40) {
		    len += 2;
		    NEEDBYTES (len);
		}
		len += mpeg1_skip_table[(header - 1)[len] >> 4] + 1;
		NEEDBYTES (len);
		/* header points to the mpeg1 pes header */
	    }
	    if ((header-1)[len] != demux_track) {
		DONEBYTES (len);
		bytes = 6 + (header[4] << 8) + header[5] - len;
		if (bytes <= 0)
		    continue;
		goto skip;
	    }
	    len += 3;
	    NEEDBYTES (len);
	    DONEBYTES (len);
	    bytes = 6 + (header[4] << 8) + header[5] - len;
	    if (bytes > end - buf) {
		fwrite (buf, end - buf, 1, stdout);
		state = DEMUX_DATA;
		state_bytes = bytes - (end - buf);
		return 0;
	    } else if (bytes > 0) {
		fwrite (buf, bytes, 1, stdout);
		buf += bytes;
	    }
	    break;
	default:
	    if (header[3] < 0xb9) {
		fprintf (stderr,
			 "looks like a video stream, not system stream\n");
		exit (1);
	    } else {
		NEEDBYTES (6);
		DONEBYTES (6);
		bytes = (header[4] << 8) + header[5];
	    skip:
		if (bytes > end - buf) {
		    state = DEMUX_SKIP;
		    state_bytes = bytes - (end - buf);
		    return 0;
		}
		buf += bytes;
	    }
	}
    }
}

static void ps_loop (void)
{
    uint8_t * end;

    do {
	end = buffer + fread (buffer, 1, BUFFER_SIZE, in_file);
	if (demux (buffer, end, 0))
	    break;	/* hit program_end_code */
    } while (end == buffer + BUFFER_SIZE);
}

static void ts_loop (void)
{
    uint8_t * buf;
    uint8_t * nextbuf;
    uint8_t * data;
    uint8_t * end;
    int pid;

    buf = buffer;
    while (1) {
	end = buf + fread (buf, 1, buffer + BUFFER_SIZE - buf, in_file);
	buf = buffer;
	for (; (nextbuf = buf + 188) <= end; buf = nextbuf) {
	    if (*buf != 0x47) {
		fprintf (stderr, "bad sync byte\n");
		nextbuf = buf + 1;
		continue;
	    }
	    pid = ((buf[1] << 8) + buf[2]) & 0x1fff;
	    if (pid != demux_pid)
		continue;
	    data = buf + 4;
	    if (buf[3] & 0x20) {	/* buf contains an adaptation field */
		data = buf + 5 + buf[4];
		if (data > nextbuf)
		    continue;
	    }
	    if (buf[3] & 0x10)
		demux (data, nextbuf,
		       (buf[1] & 0x40) ? DEMUX_PAYLOAD_START : 0);
	}
	if (end != buffer + BUFFER_SIZE)
	    break;
	memcpy (buffer, buf, end - buf);
	buf = buffer + (end - buf);
    }
}

int main (int argc, char ** argv)
{
#ifdef HAVE_IO_H
    setmode (fileno (stdin), O_BINARY);
    setmode (fileno (stdout), O_BINARY);
#endif

    handle_args (argc, argv);

    if (demux_pid)
	ts_loop ();
    else
	ps_loop ();

    return 0;
}
