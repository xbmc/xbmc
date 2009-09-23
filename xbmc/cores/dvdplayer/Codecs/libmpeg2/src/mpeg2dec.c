/*
 * mpeg2dec.c
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
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#ifdef HAVE_IO_H
#include <fcntl.h>
#include <io.h>
#endif
#ifdef LIBVO_SDL
#include <SDL/SDL.h>
#endif
#include <inttypes.h>

#include "mpeg2.h"
#include "video_out.h"
#include "gettimeofday.h"

static int buffer_size = 4096;
static FILE * in_file;
static int demux_track = 0;
static int demux_pid = 0;
static int demux_pva = 0;
static mpeg2dec_t * mpeg2dec;
static vo_open_t * output_open = NULL;
static vo_instance_t * output;
static int sigint = 0;
static int total_offset = 0;
static int verbose = 0;

void dump_state (FILE * f, mpeg2_state_t state, const mpeg2_info_t * info,
		 int offset, int verbose);

#ifdef HAVE_GETTIMEOFDAY

static RETSIGTYPE signal_handler (int sig)
{
    sigint = 1;
    signal (sig, SIG_DFL);
    return (RETSIGTYPE)0;
}

static void print_fps (int final)
{
    static uint32_t frame_counter = 0;
    static struct timeval tv_beg, tv_start;
    static int total_elapsed;
    static int last_count = 0;
    struct timeval tv_end;
    double fps, tfps;
    int frames, elapsed;

    if (verbose)
	return;

    gettimeofday (&tv_end, NULL);

    if (!frame_counter) {
	tv_start = tv_beg = tv_end;
	signal (SIGINT, signal_handler);
    }

    elapsed = (tv_end.tv_sec - tv_beg.tv_sec) * 100 +
	(tv_end.tv_usec - tv_beg.tv_usec) / 10000;
    total_elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 100 +
	(tv_end.tv_usec - tv_start.tv_usec) / 10000;

    if (final) {
	if (total_elapsed)
	    tfps = frame_counter * 100.0 / total_elapsed;
	else
	    tfps = 0;

	fprintf (stderr,"\n%d frames decoded in %.2f seconds (%.2f fps)\n",
		 frame_counter, total_elapsed / 100.0, tfps);

	return;
    }

    frame_counter++;

    if (elapsed < 50)	/* only display every 0.50 seconds */
	return;

    tv_beg = tv_end;
    frames = frame_counter - last_count;

    fps = frames * 100.0 / elapsed;
    tfps = frame_counter * 100.0 / total_elapsed;

    fprintf (stderr, "%d frames in %.2f sec (%.2f fps), "
	     "%d last %.2f sec (%.2f fps)\033[K\r", frame_counter,
	     total_elapsed / 100.0, tfps, frames, elapsed / 100.0, fps);

    last_count = frame_counter;
}

#else /* !HAVE_GETTIMEOFDAY */

static void print_fps (int final)
{
}

#endif

static void print_usage (char ** argv)
{
    int i;
    vo_driver_t const * drivers;

    fprintf (stderr, "usage: "
	     "%s [-h] [-o <mode>] [-s [<track>]] [-t <pid>] [-p] [-c] \\\n"
	     "\t\t[-v] [-b <bufsize>] <file>\n"
	     "\t-h\tdisplay help and available video output modes\n"
	     "\t-s\tuse program stream demultiplexer, "
	     "track 0-15 or 0xe0-0xef\n"
	     "\t-t\tuse transport stream demultiplexer, pid 0x10-0x1ffe\n"
	     "\t-p\tuse pva demultiplexer\n"
	     "\t-c\tuse c implementation, disables all accelerations\n"
	     "\t-v\tverbose information about the MPEG stream\n"
	     "\t-b\tset input buffer size, default 4096 bytes\n"
	     "\t-o\tvideo output mode\n", argv[0]);

    drivers = vo_drivers ();
    for (i = 0; drivers[i].name; i++)
	fprintf (stderr, "\t\t\t%s\n", drivers[i].name);

    exit (1);
}

static void handle_args (int argc, char ** argv)
{
    int c;
    vo_driver_t const * drivers;
    int i;
    char * s;

    drivers = vo_drivers ();
    while ((c = getopt (argc, argv, "hs::t:pco:vb::")) != -1)
	switch (c) {
	case 'o':
	    for (i = 0; drivers[i].name != NULL; i++)
		if (strcmp (drivers[i].name, optarg) == 0)
		    output_open = drivers[i].open;
	    if (output_open == NULL) {
		fprintf (stderr, "Invalid video driver: %s\n", optarg);
		print_usage (argv);
	    }
	    break;

	case 's':
	    demux_track = 0xe0;
	    if (optarg != NULL) {
		demux_track = strtol (optarg, &s, 0);
		if (demux_track < 0xe0)
		    demux_track += 0xe0;
		if (demux_track < 0xe0 || demux_track > 0xef || *s) {
		    fprintf (stderr, "Invalid track number: %s\n", optarg);
		    print_usage (argv);
		}
	    }
	    break;

	case 't':
	    demux_pid = strtol (optarg, &s, 0);
	    if (demux_pid < 0x10 || demux_pid > 0x1ffe || *s) {
		fprintf (stderr, "Invalid pid: %s\n", optarg);
		print_usage (argv);
	    }
	    break;

	case 'p':
	    demux_pva = 1;
	    break;

	case 'c':
	    mpeg2_accel (0);
	    break;

	case 'v':
	    if (++verbose > 4)
		print_usage (argv);
	    break;

	case 'b':
	    buffer_size = 1;
	    if (optarg != NULL) {
		buffer_size = strtol (optarg, &s, 0);
		if (buffer_size < 1 || *s) {
		    fprintf (stderr, "Invalid buffer size: %s\n", optarg);
		    print_usage (argv);
		}
	    }
	    break;

	default:
	    print_usage (argv);
	}

    /* -o not specified, use a default driver */
    if (output_open == NULL)
	output_open = drivers[0].open;

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

static void * malloc_hook (unsigned size, mpeg2_alloc_t reason)
{
    void * buf;

    /*
     * Invalid streams can refer to fbufs that have not been
     * initialized yet. For example the stream could start with a
     * picture type onther than I. Or it could have a B picture before
     * it gets two reference frames. Or, some slices could be missing.
     *
     * Consequently, the output depends on the content 2 output
     * buffers have when the sequence begins. In release builds, this
     * does not matter (garbage in, garbage out), but in test code, we
     * always zero all our output buffers to:
     * - make our test produce deterministic outputs
     * - hint checkergcc that it is fine to read from all our output
     *   buffers at any time
     */
    if ((int)reason < 0) {
        return NULL;
    }
    buf = mpeg2_malloc (size, (mpeg2_alloc_t)-1);
    if (buf && (reason == MPEG2_ALLOC_YUV || reason == MPEG2_ALLOC_CONVERTED))
        memset (buf, 0, size);
    return buf;
}

static void decode_mpeg2 (uint8_t * current, uint8_t * end)
{
    const mpeg2_info_t * info;
    mpeg2_state_t state;
    vo_setup_result_t setup_result;

    mpeg2_buffer (mpeg2dec, current, end);
    total_offset += end - current;

    info = mpeg2_info (mpeg2dec);
    while (1) {
	state = mpeg2_parse (mpeg2dec);
	if (verbose)
	    dump_state (stderr, state, info,
			total_offset - mpeg2_getpos (mpeg2dec), verbose);
	switch (state) {
	case STATE_BUFFER:
	    return;
	case STATE_SEQUENCE:
	    /* might set nb fbuf, convert format, stride */
	    /* might set fbufs */
	    if (output->setup (output, info->sequence->width,
			       info->sequence->height,
			       info->sequence->chroma_width,
			       info->sequence->chroma_height, &setup_result)) {
		fprintf (stderr, "display setup failed\n");
		exit (1);
	    }
	    if (setup_result.convert &&
		mpeg2_convert (mpeg2dec, setup_result.convert, NULL)) {
		fprintf (stderr, "color conversion setup failed\n");
		exit (1);
	    }
	    if (output->set_fbuf) {
		uint8_t * buf[3];
		void * id;

		mpeg2_custom_fbuf (mpeg2dec, 1);
		output->set_fbuf (output, buf, &id);
		mpeg2_set_buf (mpeg2dec, buf, id);
		output->set_fbuf (output, buf, &id);
		mpeg2_set_buf (mpeg2dec, buf, id);
	    } else if (output->setup_fbuf) {
		uint8_t * buf[3];
		void * id;

		output->setup_fbuf (output, buf, &id);
		mpeg2_set_buf (mpeg2dec, buf, id);
		output->setup_fbuf (output, buf, &id);
		mpeg2_set_buf (mpeg2dec, buf, id);
		output->setup_fbuf (output, buf, &id);
		mpeg2_set_buf (mpeg2dec, buf, id);
	    }
	    mpeg2_skip (mpeg2dec, (output->draw == NULL));
	    break;
	case STATE_PICTURE:
	    /* might skip */
	    /* might set fbuf */
	    if (output->set_fbuf) {
		uint8_t * buf[3];
		void * id;

		output->set_fbuf (output, buf, &id);
		mpeg2_set_buf (mpeg2dec, buf, id);
	    }
	    if (output->start_fbuf)
		output->start_fbuf (output, info->current_fbuf->buf,
				    info->current_fbuf->id);
	    break;
	case STATE_SLICE:
	case STATE_END:
	case STATE_INVALID_END:
	    /* draw current picture */
	    /* might free frame buffer */
	    if (info->display_fbuf) {
		if (output->draw)
		    output->draw (output, info->display_fbuf->buf,
				  info->display_fbuf->id);
		print_fps (0);
	    }
	    if (output->discard && info->discard_fbuf)
		output->discard (output, info->discard_fbuf->buf,
				 info->discard_fbuf->id);
	    break;
	default:
	    break;
	}
    }
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
    static uint8_t head_buf[264];

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
	    decode_mpeg2 (buf, end);
	    state_bytes -= end - buf;
	    return 0;
	}
	decode_mpeg2 (buf, buf + state_bytes);
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
	if (demux_pid) {
	    if ((header[3] >= 0xe0) && (header[3] <= 0xef))
		goto pes;
	    fprintf (stderr, "bad stream id %x\n", header[3]);
	    exit (1);
	}
	switch (header[3]) {
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
	default:
	    if (header[3] == demux_track) {
	    pes:
		NEEDBYTES (7);
		if ((header[6] & 0xc0) == 0x80) {	/* mpeg2 */
		    NEEDBYTES (9);
		    len = 9 + header[8];
		    NEEDBYTES (len);
		    /* header points to the mpeg2 pes header */
		    if (header[7] & 0x80) {
			uint32_t pts, dts;

			pts = (((header[9] >> 1) << 30) |
			       (header[10] << 22) | ((header[11] >> 1) << 15) |
			       (header[12] << 7) | (header[13] >> 1));
			dts = (!(header[7] & 0x40) ? pts :
			       (uint32_t)(((header[14] >> 1) << 30) |
				(header[15] << 22) |
				((header[16] >> 1) << 15) |
				(header[17] << 7) | (header[18] >> 1)));
			mpeg2_tag_picture (mpeg2dec, pts, dts);
		    }
		} else {	/* mpeg1 */
		    int len_skip;
		    uint8_t * ptsbuf;

		    len = 7;
		    while (header[len - 1] == 0xff) {
			len++;
			NEEDBYTES (len);
			if (len > 23) {
			    fprintf (stderr, "too much stuffing\n");
			    break;
			}
		    }
		    if ((header[len - 1] & 0xc0) == 0x40) {
			len += 2;
			NEEDBYTES (len);
		    }
		    len_skip = len;
		    len += mpeg1_skip_table[header[len - 1] >> 4];
		    NEEDBYTES (len);
		    /* header points to the mpeg1 pes header */
		    ptsbuf = header + len_skip;
		    if ((ptsbuf[-1] & 0xe0) == 0x20) {
			uint32_t pts, dts;

			pts = (((ptsbuf[-1] >> 1) << 30) |
			       (ptsbuf[0] << 22) | ((ptsbuf[1] >> 1) << 15) |
			       (ptsbuf[2] << 7) | (ptsbuf[3] >> 1));
			dts = (((ptsbuf[-1] & 0xf0) != 0x30) ? pts :
			       (uint32_t)(((ptsbuf[4] >> 1) << 30) |
				(ptsbuf[5] << 22) | ((ptsbuf[6] >> 1) << 15) |
				(ptsbuf[7] << 7) | (ptsbuf[18] >> 1)));
			mpeg2_tag_picture (mpeg2dec, pts, dts);
		    }
		}
		DONEBYTES (len);
		bytes = 6 + (header[4] << 8) + header[5] - len;
		if (demux_pid || (bytes > end - buf)) {
		    decode_mpeg2 (buf, end);
		    state = DEMUX_DATA;
		    state_bytes = bytes - (end - buf);
		    return 0;
		} else if (bytes > 0) {
		    decode_mpeg2 (buf, buf + bytes);
		    buf += bytes;
		}
	    } else if (header[3] < 0xb9) {
		fprintf (stderr,
			 "looks like a video stream, not system stream\n");
		DONEBYTES (4);
	    } else {
		NEEDBYTES (6);
		DONEBYTES (6);
		bytes = (header[4] << 8) + header[5];
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
    uint8_t * buffer = (uint8_t *) malloc (buffer_size);
    uint8_t * end;

    if (buffer == NULL)
	exit (1);
    do {
	end = buffer + fread (buffer, 1, buffer_size, in_file);
	if (demux (buffer, end, 0))
	    break;	/* hit program_end_code */
    } while (end == buffer + buffer_size && !sigint);
    free (buffer);
}

static int pva_demux (uint8_t * buf, uint8_t * end)
{
    static int state = DEMUX_SKIP;
    static int state_bytes = 0;
    static uint8_t head_buf[15];

    uint8_t * header;
    int bytes;
    int len;

    switch (state) {
    case DEMUX_HEADER:
        if (state_bytes > 0) {
            header = head_buf;
            bytes = state_bytes;
            goto continue_header;
        }
        break;
    case DEMUX_DATA:
        if (state_bytes > end - buf) {
            decode_mpeg2 (buf, end);
            state_bytes -= end - buf;
            return 0;
        }
        decode_mpeg2 (buf, buf + state_bytes);
        buf += state_bytes;
        break;
    case DEMUX_SKIP:
        if (state_bytes > end - buf) {
            state_bytes -= end - buf;
            return 0;
        }
        buf += state_bytes;
        break;
    }

    while (1) {
    payload_start:
	header = buf;
	bytes = end - buf;
    continue_header:
	NEEDBYTES (2);
	if (header[0] != 0x41 || header[1] != 0x56) {
	    if (header != head_buf) {
		buf++;
		goto payload_start;
	    } else {
		header[0] = header[1];
		bytes = 1;
		goto continue_header;
	    }
	}
	NEEDBYTES (8);
	if (header[2] != 1) {
	    DONEBYTES (8);
	    bytes = (header[6] << 8) + header[7];
	    if (bytes > end - buf) {
		state = DEMUX_SKIP;
		state_bytes = bytes - (end - buf);
		return 0;
	    } 
	    buf += bytes; 
	} else {
	    len = 8;
	    if (header[5] & 0x10) {
		len = 12 + (header[5] & 3);
		NEEDBYTES (len);
		decode_mpeg2 (header + 12, header + len);
		mpeg2_tag_picture (mpeg2dec,
				   ((header[8] << 24) | (header[9] << 16) |
				    (header[10] << 8) | header[11]), 0);
	    }
	    DONEBYTES (len);
	    bytes = (header[6] << 8) + header[7] + 8 - len;
	    if (bytes > end - buf) {
		decode_mpeg2 (buf, end);
		state = DEMUX_DATA;
		state_bytes = bytes - (end - buf);
		return 0;
	    } else if (bytes > 0) {
		decode_mpeg2 (buf, buf + bytes);
		buf += bytes;
	    }
	}
    }
}

static void pva_loop (void)
{
    uint8_t * buffer = (uint8_t *) malloc (buffer_size);
    uint8_t * end;

    if (buffer == NULL)
	exit (1);
    do {
	end = buffer + fread (buffer, 1, buffer_size, in_file);
	pva_demux (buffer, end);
    } while (end == buffer + buffer_size && !sigint);
    free (buffer);
}

static void ts_loop (void)
{
    uint8_t * buffer = (uint8_t *) malloc (buffer_size);
    uint8_t * buf;
    uint8_t * nextbuf;
    uint8_t * data;
    uint8_t * end;
    int pid;

    if (buffer == NULL || buffer_size < 188)
	exit (1);
    buf = buffer;
    do {
	end = buf + fread (buf, 1, buffer + buffer_size - buf, in_file);
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
	if (end != buffer + buffer_size)
	    break;
	memcpy (buffer, buf, end - buf);
	buf = buffer + (end - buf);
    } while (!sigint);
    free (buffer);
}

static void es_loop (void)
{
    uint8_t * buffer = (uint8_t *) malloc (buffer_size);
    uint8_t * end;

    if (buffer == NULL)
	exit (1);
    do {
	end = buffer + fread (buffer, 1, buffer_size, in_file);
	decode_mpeg2 (buffer, end);
    } while (end == buffer + buffer_size && !sigint);
    free (buffer);
}

int main (int argc, char ** argv)
{
#ifdef HAVE_IO_H
    setmode (fileno (stdin), O_BINARY);
    setmode (fileno (stdout), O_BINARY);
#endif

    fprintf (stderr, PACKAGE"-"VERSION
	     " - by Michel Lespinasse <walken@zoy.org> and Aaron Holtzman\n");

    handle_args (argc, argv);

    output = output_open ();
    if (output == NULL) {
	fprintf (stderr, "Can not open output\n");
	return 1;
    }
    mpeg2dec = mpeg2_init ();
    if (mpeg2dec == NULL)
	exit (1);
    mpeg2_malloc_hooks (malloc_hook, NULL);

    if (demux_pva)
	pva_loop ();
    else if (demux_pid)
	ts_loop ();
    else if (demux_track)
	ps_loop ();
    else
	es_loop ();

    mpeg2_close (mpeg2dec);
    if (output->close)
	output->close (output);
    print_fps (1);
    fclose (in_file);
    return 0;
}
