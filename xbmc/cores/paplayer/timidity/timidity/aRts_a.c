/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    aRts_a.c by Peter L Jones <peter@drealm.org.uk>
    based on esd_a.c

    Functions to play sound through aRts
*/

/* 2003.06.05  mput <root@mput.dip.jp>
 *	I and Masanao Izumo had different codes.  Mine was implemented
 *	by Peter L Jones <peter@drealm.org.uk>, which was posted to
 *	linux-audio-develpers ML.  Izumo's was written by Bernhard
 *	"Bero" Rosenkraenzer <bero@redhat.com>.  Both worked correctly,
 *	but differed a bit.  I have mergerd Bero's code into Peter's.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <artsc.h>

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "timer.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"

static arts_stream_t stream = 0;
static int server_buffer = 0;
static int output_count = 0;

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);
static int detect(void);

/* export the playback mode */

#define dpm arts_play_mode

PlayMode dpm = {
    /*rate*/         DEFAULT_RATE,
    /*encoding*/     PE_16BIT|PE_SIGNED,
    /*flag*/         PF_PCM_STREAM/*|PF_BUFF_FRAGM_OPT*//**/,
    /*fd*/           -1,
    /*extra_param*/  {0}, /* default: get all the buffer fragments you can */
    /*id*/           "aRts",
    /*id char*/      'R',
    /*name*/         "arts",
    open_output,
    close_output,
    output_data,
    acntl,
    detect
};

static int detect(void)
{
    if (arts_init() == 0) {
	arts_free();
	return 1; /* ok, found */
    }
    return 0;
}

/*************************************************************************/
/* We currently only honor the PE_MONO bit, and the sample rate. */

static int open_output(void)
{
    int i, include_enc, exclude_enc;
    int sample_width, channels;

    include_enc = 0;
    exclude_enc = PE_ULAW|PE_ALAW|PE_BYTESWAP; /* They can't mean these */
    if(dpm.encoding & PE_16BIT)
	include_enc |= PE_SIGNED;
    else
	exclude_enc |= PE_SIGNED;
    dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);
    sample_width = (dpm.encoding & PE_16BIT) ? 16 : 8;
    channels = (dpm.encoding & PE_MONO) ? 1 : 2;

    /* Open the audio device */
    if((i = arts_init()) != 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, arts_error_text(i));
	return -1;
    }
    stream = arts_play_stream(dpm.rate,
			      LE_LONG(sample_width),
			      channels,
			      "timidity");
    if(stream == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	return -1;
    }
    arts_stream_set(stream, ARTS_P_BLOCKING, 1);

    server_buffer = arts_stream_get(stream, ARTS_P_SERVER_LATENCY) * dpm.rate * (sample_width/8) * channels / 1000;
    output_count = 0;
    return 0;
    /* "this aRts function isnot yet implemented"
     *
    if (dpm.extra_param[0]) {
        i = arts_stream_set(stream,
            ARTS_P_PACKET_COUNT,
            dpm.extra_param[0]);
	if (i < 0) {
            ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
              dpm.name, arts_error_text(i));
	    return 1;
        }
    }
    return 0;
     *
     */
}

static int output_data(char *buf, int32 nbytes)
{
    int n;

    if (stream == 0) return -1;

    while(nbytes > 0)
    {
	if((n = arts_write(stream, buf, nbytes)) < 0)
	{
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "%s: %s", dpm.name, arts_error_text(n));
	    if(errno == EWOULDBLOCK)
	    {
		/* It is possible to come here because of bug of the
		 * sound driver.
		 */
		continue;
	    }
	    return -1;
	}
	output_count += n;
	buf += n;
	nbytes -= n;
    }

    return 0;
}

static void close_output(void)
{
    if(stream == 0)
	return;
    arts_close_stream(stream);
    arts_free();
    stream = 0;
}

static int acntl(int request, void *arg)
{
    int tmp, tmp1, samples;
    switch(request)
    {
      case PM_REQ_DISCARD: /* Discard stream */
	arts_close_stream(stream);
	arts_free();
	stream=NULL;
	return 0;
      case PM_REQ_RATE: /* Change sample rate */
        arts_close_stream(stream);
        tmp = (dpm.encoding & PE_16BIT) ? 16 : 8;
        tmp1 = (dpm.encoding & PE_MONO) ? 1 : 2;
        stream = arts_play_stream(*(int*)arg,
		    LE_LONG(tmp),
		    tmp1,
		    "timidity");
        server_buffer = arts_stream_get(stream, ARTS_P_SERVER_LATENCY) * dpm.rate * (tmp/8) * tmp1 / 1000;
	return 0;
      case PM_REQ_GETQSIZ: /* Get maximum queue size */
	*(int*)arg = arts_stream_get(stream, ARTS_P_BUFFER_SIZE);
	return 0;
      case PM_REQ_SETQSIZ: /* Set queue size */
	*(int*)arg = arts_stream_set(stream, ARTS_P_BUFFER_SIZE, *(int*)arg);
	return 0;
      case PM_REQ_GETFRAGSIZ: /* Get device fragment size */
	*(int*)arg = arts_stream_get(stream, ARTS_P_PACKET_SIZE);
	return 0;
      case PM_REQ_GETSAMPLES: /* Get current play sample */
	tmp = arts_stream_get(stream, ARTS_P_BUFFER_SIZE) -
		         arts_stream_get(stream, ARTS_P_BUFFER_SPACE) +
			 server_buffer;
	samples = output_count - tmp;
	if(samples < 0)
	  samples = 0;
	if(!(dpm.encoding & PE_MONO)) samples >>= 1;
	if(dpm.encoding & PE_16BIT) samples >>= 1;
	*(int*)arg = samples;
	return 0;
      case PM_REQ_GETFILLABLE: /* Get fillable device queue size */
	*(int*)arg = arts_stream_get(stream, ARTS_P_BUFFER_SPACE);
	return 0;
      case PM_REQ_GETFILLED: /* Get filled device queue size */
	*(int*)arg = arts_stream_get(stream, ARTS_P_BUFFER_SIZE) - arts_stream_get(stream, ARTS_P_BUFFER_SPACE);
	return 0;
      /* The following are not (yet) implemented: */
      case PM_REQ_FLUSH: /* Wait until playback is complete */
      case PM_REQ_MIDI: /* Send MIDI event */
      case PM_REQ_INST_NAME: /* Get instrument name */
      case PM_REQ_OUTPUT_FINISH: /* Sent after last output_data */
      case PM_REQ_PLAY_START: /* Called just before playing */
      case PM_REQ_PLAY_END: /* Called just after playing */
	return -1;
    }
    return -1;
}
