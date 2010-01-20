/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    jack_a.c - Copyright (C) 2003 Takashi Iwai <tiwai@suse.de>

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

    jack_a.c

    Functions to play sound on the JACK system


    Since the model of JACK (so-called "pull" model) doesn't match
    with the current model of TiMidity ("push" model), this audio
    driver is implemented with another intermediate ring buffer.

    The audio data from timidity (S16 or U8 PCM) is converted to
    JACK audio stream(s) in float and stored once on the ring buffer,
    which is read eventually in the client's process callback and
    copied to the JACK buffers.  Note that this buffer is independent
    from the audio-queue.  (Yes, it's re-invention of wheels, but we
    cannot handle dynamic memory allocation in JACK's process thread.)

    The buffer size options (-B) specify the size of the ring buffer.
    To get the best perfomance, you'll need to set up them to match
    with the parameter of JACK server.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef NO_STRING_H /* for memmove */
#include <string.h>
#else
#include <strings.h>
#endif

#include <jack/jack.h>

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "timer.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"

/*
 */
static int open_jack(void);
static void close_jack(void);
static int write_jack(char *buf, int32 nbytes);
static int actl_jack(int request, void *arg);
static int detect(void);


/*
 */

#define dpm jack_play_mode

PlayMode dpm = {
	DEFAULT_RATE,
	PE_16BIT|PE_SIGNED,
	PF_PCM_STREAM|PF_CAN_TRACE|PF_BUFF_FRAGM_OPT,
	-1,
	{0},
	"JACK device", 'j',
	NULL,
	open_jack,
	close_jack,
	write_jack,
	actl_jack,
	detect
};

/*
 * simple ring-buffer
 */

struct tm_ringbuf {
	long rdptr, wrptr;	/* read, write pointers (not bound in ringbuffer size!) */
	int size;		/* ring buffer size */
	jack_default_audio_sample_t *buf[2];	/* left, right buffers */
};

static void ringbuf_init(struct tm_ringbuf *rbuf, int size, int channels)
{
	int i;

	memset(rbuf, 0, sizeof(*rbuf));
	rbuf->size = size;
	for (i = 0; i < channels; i++)
		rbuf->buf[i] = (jack_default_audio_sample_t *)safe_malloc(sizeof(jack_default_audio_sample_t) * size);
	rbuf->rdptr = rbuf->wrptr = 0;
}

static void ringbuf_destroy(struct tm_ringbuf *rbuf)
{
	int i;
	for (i = 0; i < 2; i++) {
		free(rbuf->buf[i]);
		rbuf->buf[i] = NULL;
	}
	rbuf->rdptr = rbuf->wrptr = 0;
}

/* get avaialble samples for read */
inline static int ringbuf_get_available(struct tm_ringbuf *rbuf)
{
	return rbuf->wrptr - rbuf->rdptr;
}

/* get empty sample spaces for write */
inline static int ringbuf_get_empty(struct tm_ringbuf *rbuf)
{
	return rbuf->size - ringbuf_get_available(rbuf);
}

static int ringbuf_bound_readable(struct tm_ringbuf *rbuf, int size)
{
	int rdptr = rbuf->rdptr % rbuf->size;
	if (rdptr + size >= rbuf->size)
		size = rbuf->size - rdptr;
	return size;
}

inline static jack_default_audio_sample_t *
ringbuf_get_readbuf(struct tm_ringbuf *rbuf, int c)
{
	return rbuf->buf[c] + (rbuf->rdptr % rbuf->size);
}

inline static void ringbuf_read_advance(struct tm_ringbuf *rbuf, int size)
{
	rbuf->rdptr += size;
}

static int ringbuf_bound_writable(struct tm_ringbuf *rbuf, int size)
{
	int wrptr = rbuf->wrptr % rbuf->size;
	if (wrptr + size >= rbuf->size)
		size = rbuf->size - wrptr;
	return size;
}

inline static jack_default_audio_sample_t *
ringbuf_get_writebuf(struct tm_ringbuf *rbuf, int c)
{
	return rbuf->buf[c] + (rbuf->wrptr % rbuf->size);
}

inline static void ringbuf_write_advance(struct tm_ringbuf *rbuf, int size)
{
	rbuf->wrptr += size;
}

inline static void ringbuf_clear(struct tm_ringbuf *rbuf)
{
	rbuf->wrptr = rbuf->rdptr = 0;
}


/*
 * jack control context
 */
struct tm_jack {
	jack_client_t *client;
	jack_port_t *ports[2];

	int channels;		/* number of channels */
	int sample_16bit;	/* 16bit sample */
	int shift;		/* sample bit shift */
	int frag_size;		/* buffer fragment size (in samples) */
	int frags;		/* buffer fragments */

	pthread_cond_t cond;
	pthread_mutex_t lock;
	int running;
	int shutdown;

	struct tm_ringbuf rbuf;
};


/*
 * jack process callback.
 * here we only copy the stream data from the ring buffer.
 */

static int transfer_callback(jack_nframes_t nframes, void *arg)
{
	struct tm_jack *ctx = (struct tm_jack *)arg;
	int i;
	int size;
	jack_default_audio_sample_t *outbuf[2];

	for (i = 0; i < ctx->channels; i++)
		outbuf[i] = (jack_default_audio_sample_t *)jack_port_get_buffer(ctx->ports[i], nframes);

	if (! ctx->running) {
		/* not running yet, set silence and quit */
		for (i = 0; i < ctx->channels; i++)
			memset(outbuf[i], 0, sizeof(jack_default_audio_sample_t) * nframes);
		return 0;
	}

	size = ringbuf_get_available(&ctx->rbuf);
	if (size > nframes)
		size = nframes;

	while (size > 0) {
		int size1 = ringbuf_bound_readable(&ctx->rbuf, size);
		for (i = 0; i < ctx->channels; i++) {
			memcpy(outbuf[i], ringbuf_get_readbuf(&ctx->rbuf, i),
			       size1 * sizeof(jack_default_audio_sample_t));
			outbuf[i] += size1;
		}
		ringbuf_read_advance(&ctx->rbuf, size1);
		size -= size1;
	}
	/* wake up the main thread */
	pthread_cond_signal(&ctx->cond);
	return 0;
}

static void shutdown_callback(void *arg)
{
	struct tm_jack *ctx = (struct tm_jack *)arg;
	if (! ctx->shutdown)
		safe_exit(1);
}


/*
 */

static struct tm_jack jack_ctx;

#define TIMIDITY_JACK_CLIENT_NAME	"TiMidity"
#define TIMIDITY_JACK_PORT_NAME		"port_%d"

static int detect(void)
{
	jack_client_t *client;
	client = jack_client_new(TIMIDITY_JACK_CLIENT_NAME);
	if (! client)
		return 0;
	jack_client_close(client);
	return 1; /* found */
}

static int open_jack(void)
{
	int i, rate;
	int ret_val = 0;
	struct tm_jack *ctx = &jack_ctx;
	const char **dst_ports;

	memset(ctx, 0, sizeof(*ctx));

	ctx->client = jack_client_new(TIMIDITY_JACK_CLIENT_NAME);
	if (! ctx->client)
		return -1;

	jack_set_process_callback(ctx->client, transfer_callback, ctx);
	jack_on_shutdown(ctx->client, shutdown_callback, ctx);

	dpm.encoding &= ~(PE_ULAW|PE_ALAW|PE_BYTESWAP);
	/* check channels */
	if (dpm.encoding & PE_MONO)
		ctx->channels = 1;
	else
		ctx->channels = 2;

	/* sample bit check */
	ctx->sample_16bit = (dpm.encoding & PE_16BIT) ? 1 : 0;
	if (ctx->sample_16bit)
		dpm.encoding |= PE_SIGNED; /* S16 only */
	else
		dpm.encoding &= ~PE_SIGNED; /* U8 only */

	/* check sample bit shift */
	ctx->shift = 0;
	if (ctx->channels > 1)
		ctx->shift++;
	if (ctx->sample_16bit)
		ctx->shift++;

	for (i = 0; i < ctx->channels; i++) {
		char name[32];
		sprintf(name, TIMIDITY_JACK_PORT_NAME, i + 1);
		ctx->ports[i] = jack_port_register(ctx->client, name,
						   JACK_DEFAULT_AUDIO_TYPE,
						   JackPortIsOutput, 0);
		if (! ctx->ports[i]) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "Cannot register a JACK port '%s'", name);
			jack_client_close(ctx->client);
			return -1;
		}
	}

	/* check the sample rate.
	 * if not match, correct the rate
	 */
	rate = jack_get_sample_rate(ctx->client);
	if (rate != dpm.rate) {
		dpm.rate = rate;
		ret_val = 1;
	}

	/* get the buffer sizes */
	if (dpm.extra_param[1] != 0)
		ctx->frag_size = dpm.extra_param[1];
	else
		ctx->frag_size = audio_buffer_size;
	if (dpm.extra_param[0] == 0)
		ctx->frags = 2;
	else
		ctx->frags = dpm.extra_param[0];

	/* initialize rest stuffs */
	pthread_cond_init(&ctx->cond, NULL);
	pthread_mutex_init(&ctx->lock, NULL);
	ringbuf_init(&ctx->rbuf, ctx->frag_size * ctx->frags, ctx->channels);
	ctx->running = 0;

	/* rock it baby */
	if (jack_activate(ctx->client) < 0) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Cannot activate JACK engine");
		jack_client_close(ctx->client);
		ctx->client = NULL;
		return -1;
	}

	/* 
	 * it seems the JACK port connection must be done after
	 * activating jack client...
	 */
	/* detect destination ports */
	dst_ports = jack_get_ports(ctx->client, NULL, NULL,
				   JackPortIsInput|JackPortIsPhysical);
	if (dst_ports) {
		/* connect them */
		for (i = 0; dst_ports[i] && i < ctx->channels; i++) {
			if (jack_connect(ctx->client, jack_port_name(ctx->ports[i]), dst_ports[i]))
				break;
		}
		free(dst_ports);
	}

	return ret_val;
}

/*
 * close callback
 */
static void close_jack(void)
{
	struct tm_jack *ctx = &jack_ctx;
	if (ctx->client) {
		ctx->shutdown = 1;
		ctx->running = 0;
		jack_deactivate(ctx->client);
		sleep(2);
		jack_client_close(ctx->client);
		sleep(2);
		ringbuf_destroy(&ctx->rbuf);
		pthread_cond_destroy(&ctx->cond);
	}
}

/*
 * convert 16bit PCM to JACK float [-1,1]
 */
static void convert_stream_16(struct tm_jack *ctx, int c, int size, short *buf)
{
	int i;
	jack_default_audio_sample_t *inbuf = ringbuf_get_writebuf(&ctx->rbuf, c);
	for (i = 0; i < size; i++, buf += ctx->channels, inbuf++) {
		/* well, we can use ftol() in C99 but here let's leave
		 * the optimization for the compiler...
		 */
		jack_default_audio_sample_t val;
		val = (jack_default_audio_sample_t)*buf / 32768.0;
		*inbuf = val;
	}
}

/*
 * convert 8bit PCM to JACK float [-1,1]
 */
static void convert_stream_8(struct tm_jack *ctx, int c, int size, char *buf)
{
	int i;
	jack_default_audio_sample_t *inbuf = ringbuf_get_writebuf(&ctx->rbuf, c);
	for (i = 0; i < size; i++, buf += ctx->channels, inbuf++) {
		signed char cval = *buf ^ 0x80; /* to signed char */
		*inbuf = (jack_default_audio_sample_t)cval / 128.0;
	}
}

/*
 * write callback
 */
static int write_jack(char *buf, int32 nbytes)
{
	struct tm_jack *ctx = &jack_ctx;
	int nframes;
	int i;

	nframes = nbytes >> ctx->shift;
	if (nframes <= 0)
		return 0;

	for (;;) {
		int size = ringbuf_get_empty(&ctx->rbuf);
		if (size > nframes)
			size = nframes;
		while (size > 0) {
			int size1 = ringbuf_bound_writable(&ctx->rbuf, size);
			if (ctx->sample_16bit) {
				short *sbuf = (short *)buf;
				for (i = 0; i < ctx->channels; i++, sbuf++)
					convert_stream_16(ctx, i, size1, sbuf);
			} else {
				char *sbuf = (char *)buf;
				for (i = 0; i < ctx->channels; i++, sbuf++)
					convert_stream_8(ctx, i, size1, sbuf);
			}
			ringbuf_write_advance(&ctx->rbuf, size1);
			/* set running flag */
			ctx->running = 1;
			nframes -= size1;
			if (! nframes)
				return 0;
			buf += size1 << ctx->shift;
			size -= size1;
		}

		/* blocking behavior: sleep until the process thread
		 * wakes up...
		 */
		pthread_cond_wait(&ctx->cond, &ctx->lock);
	}
}


/*
 * audio control callback
 */
static int actl_jack(int request, void *arg)
{
	struct tm_jack *ctx = &jack_ctx;

	switch (request) {
	case PM_REQ_GETFRAGSIZ:
		if (ctx->frag_size == 0)
			return -1;
		*((int *)arg) = ctx->frag_size;
		return 0;

	case PM_REQ_GETQSIZ:
		if (ctx->frag_size == -1)
			return -1;
		*((int *)arg) = ctx->frag_size * ctx->frags;
		return 0;

	case PM_REQ_GETFILLABLE:
		*((int *)arg) = ringbuf_get_empty(&ctx->rbuf);
		return 0;
    
	case PM_REQ_GETFILLED:
		*((int *)arg) = ringbuf_get_available(&ctx->rbuf);
		return 0;

	case PM_REQ_GETSAMPLES:
		*((int *)arg) = ctx->rbuf.rdptr;
		return 0;

	case PM_REQ_FLUSH:
		if (ctx->running) {
			while (ringbuf_get_available(&ctx->rbuf) > 0)
				pthread_cond_wait(&ctx->cond, &ctx->lock);
		}
		/* fallthrough */
	case PM_REQ_DISCARD:
		ctx->running = 0;
		ringbuf_clear(&ctx->rbuf);
		return 0;

	}
	return -1;
}

