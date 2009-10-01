/* findsep.c
 * library routines for find silent points in mp3 data
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Portions are adapted from minimad.c, included with the 
 * libmad library, distributed under the GNU General Public License.
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "mad.h"
#include "findsep.h"
#include "srtypes.h"
#include "debug.h"
#include "list.h"

#define MIN_RMS_SILENCE		100
#define MAX_RMS_SILENCE		32767 //max short
#define NUM_SILTRACKERS		30
#define READSIZE	2000
// #define READSIZE	1000

/* Uncomment to dump an mp3 of the search window. */
//   #define MAKE_DUMP_MP3 1

typedef struct FRAME_LIST_struct FRAME_LIST;
struct FRAME_LIST_struct
{
    const unsigned char* m_framepos;
    long m_samples;
    long m_pcmpos;
    LIST m_list;
};

typedef struct SILENCETRACKERst
{
    long insilencecount;
    double silencevol;
    unsigned long silstart_samp;
    BOOL foundsil;
} SILENCETRACKER;

typedef struct DECODE_STRUCTst
{
    unsigned char* mpgbuf;
    long  mpgsize;
    long  mpgpos;
    long len_to_sw_ms;
    long searchwindow_ms;
    long  silence_ms;
    long  silence_samples;
    unsigned long len_to_sw_start_samp;
    unsigned long len_to_sw_end_samp;
    unsigned long  pcmpos;
    long  samplerate;
    SILENCETRACKER siltrackers[NUM_SILTRACKERS];
    LIST frame_list;
} DECODE_STRUCT;

typedef struct GET_BITRATE_STRUCTst
{
    unsigned long bitrate;
    unsigned char* mpgbuf;
    long mpgsize;
} GET_BITRATE_STRUCT;

/*****************************************************************************
 * Public functions
 *****************************************************************************/

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void init_siltrackers(SILENCETRACKER* siltrackers);
static void apply_padding (DECODE_STRUCT* ds, unsigned long silstart,
			   long padding1, long padding2,
			   u_long* pos1, u_long* pos2);
static void free_frame_list (DECODE_STRUCT* ds);
static enum mad_flow input(void *data, struct mad_stream *ms);
static void search_for_silence(DECODE_STRUCT *ds, double vol);
static signed int scale(mad_fixed_t sample);
static enum mad_flow output(void *data, struct mad_header const *header,
			    struct mad_pcm *pcm);
static enum mad_flow filter (void *data, struct mad_stream const *ms,
			     struct mad_frame *frame);
static enum mad_flow error(void *data, struct mad_stream *ms, 
			   struct mad_frame *frame);
static enum mad_flow header(void *data, struct mad_header const *pheader);
static enum mad_flow input_get_bitrate (void *data, struct mad_stream *stream);
static enum mad_flow header_get_bitrate (void *data, 
					 struct mad_header const *pheader);

/*****************************************************************************
 * Private Vars
 *****************************************************************************/

/*****************************************************************************
 * Functions
 *****************************************************************************/
error_code
findsep_silence (const u_char* mpgbuf,
		 long mpgsize,
		 long len_to_sw,
		 long searchwindow,
		 long silence_length,
		 long padding1,
		 long padding2,
		 u_long* pos1,
		 u_long* pos2
		 )
{
    DECODE_STRUCT ds;
    struct mad_decoder decoder;
    int result;
    unsigned long silstart;
    int i;
    
    ds.mpgbuf = (unsigned char*)mpgbuf;
    ds.mpgsize = mpgsize;
    ds.pcmpos = 0;
    ds.mpgpos= 0;
    ds.samplerate = 0;
    ds.len_to_sw_ms = len_to_sw;
    ds.searchwindow_ms = searchwindow;
    ds.silence_ms = silence_length;
    INIT_LIST_HEAD (&ds.frame_list);

    debug_printf ("FINDSEP: %p -> %p (%d)\n", mpgbuf, mpgbuf+mpgsize, mpgsize);

    init_siltrackers(ds.siltrackers);

#if defined (MAKE_DUMP_MP3)
    {
	FILE* fp = fopen("dump.mp3", "wb");
	fwrite(mpgbuf, mpgsize, 1, fp);
	fclose(fp);
    }
#endif

    /* Run decoder */
    mad_decoder_init(&decoder, &ds,
		     input /* input */,
		     header/* header */,
		     filter /* filter */,
		     output /* output */,
		     error /* error */,
		     NULL /* message */);
    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
    mad_decoder_finish(&decoder);

    debug_printf ("total length:    %d\n", ds.pcmpos);
    debug_printf ("silence_length:  %d ms\n", ds.silence_ms);
    debug_printf ("silence_samples: %d\n", ds.silence_samples);

    /* Search through siltrackers to find minimum volume point */
    assert(ds.mpgsize != 0);
    silstart = ds.pcmpos/2;
    for(i = 0; i < NUM_SILTRACKERS; i++) {
	debug_printf("SILT: %2d/%8g, pcm=%4d, found=%d, insil=%d\n", 
	    i,
	    ds.siltrackers[i].silencevol,
	    ds.siltrackers[i].silstart_samp,
	    ds.siltrackers[i].foundsil,
	    ds.siltrackers[i].insilencecount
	    );
	if (ds.siltrackers[i].foundsil) {
	    debug_printf("found!\n");
	    silstart = ds.siltrackers[i].silstart_samp;
	    break;
	}
    }

    if (i == NUM_SILTRACKERS) {
	debug_printf("warning: no silence found between tracks\n");
    }

    /* Now that we have the start of the silence, let's add the padding */
    apply_padding (&ds, silstart, padding1, padding2, pos1, pos2);

    /* Free the list of frame info */
    free_frame_list (&ds);

    return SR_SUCCESS;
}

void init_siltrackers(SILENCETRACKER* siltrackers)
{
    int i;
    long stepsize = (MAX_RMS_SILENCE - MIN_RMS_SILENCE) / (NUM_SILTRACKERS-1);
    long rms = MIN_RMS_SILENCE;
    for (i = 0; i < NUM_SILTRACKERS; i++, rms += stepsize) {
	siltrackers[i].foundsil = 0;
	siltrackers[i].silstart_samp = 0;
	siltrackers[i].insilencecount = 0;
	siltrackers[i].silencevol = rms;
    }
}

static void
apply_padding (DECODE_STRUCT* ds,
	       unsigned long silstart,
	       long padding1,
	       long padding2,
	       u_long* pos1, 
	       u_long* pos2
	       )
{
    /* Compute positions in samples */
    FRAME_LIST *pos;
    long pos1s, pos2s;

    pos1s = silstart 
	    + (ds->silence_samples/2) 
	    + padding1 * (ds->samplerate/1000);
    pos2s = silstart 
	    + (ds->silence_samples/2) 
	    - padding2 * (ds->samplerate/1000);

    debug_printf ("Applying padding: p1,p2 = (%d,%d), pos1s,pos2s = (%d,%d)\n", padding1, padding2, pos1s, pos2s);

    /* GCS FIX: Need to check for pos == null */
    /* GCS FIX: Watch out for -1, might have mem error! */
    pos = list_entry (ds->frame_list.next, FRAME_LIST, m_list);
    if (pos1s < pos->m_pcmpos) {
	*pos1 = pos->m_framepos - ds->mpgbuf - 1;
    }
    if (pos2s < pos->m_pcmpos) {
	*pos2 = pos->m_framepos - ds->mpgbuf;
    }
    list_for_each_entry (pos, FRAME_LIST, &(ds->frame_list), m_list) {
	if (pos1s >= pos->m_pcmpos) {
	    *pos1 = pos->m_framepos - ds->mpgbuf - 1;
	}
	if (pos2s >= pos->m_pcmpos) {
	    *pos2 = pos->m_framepos - ds->mpgbuf;
	}
    }
    debug_printf ("pos1, pos2 = %d,%d (%d) (%02x%02x)\n",
		  *pos1, *pos2, 
		  *pos1 - *pos2, 
		  ds->mpgbuf[*pos2], 
		  ds->mpgbuf[*pos2+1]);
}

static void 
free_frame_list (DECODE_STRUCT* ds)
{
    FRAME_LIST *pos, *n;
    /* GCS: This seems to be the best way to go through a list.
       Note no compiler warnings. */
    list_for_each_entry_safe (pos, FRAME_LIST, n, &(ds->frame_list), m_list) {
	list_del (&(pos->m_list));
	free (pos);
    }
}

enum mad_flow
input(void *data, struct mad_stream *ms)
{
    DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
    long frameoffset = 0;
    long espnextpos = ds->mpgpos+READSIZE;

    /* GCS FIX: This trims the last READSIZE from consideration */
    if (espnextpos > ds->mpgsize) {
	return MAD_FLOW_STOP;
    }

    if (ms->next_frame) {
	frameoffset = &(ds->mpgbuf[ds->mpgpos]) - ms->next_frame;
        /* GCS July 8, 2004
	       This is the famous frameoffset != READSIZE bug.
	       What appears to be happening is libmad is not syncing 
	       properly on the broken initial frame.  Therefore, 
	       if there is no header yet (hence no ds->samplerate),
	       we'll nudge along the buffer to try to resync.
         */
	if (frameoffset == READSIZE) {
	    if (!ds->samplerate) {
		frameoffset--;
	    } else {
		FILE* fp;
		debug_printf ("%p | %p | %p | %p | %d\n",
		    ds->mpgbuf, ds->mpgpos, &(ds->mpgbuf[ds->mpgpos]), 
		    ms->next_frame, frameoffset);
    		fprintf (stderr, "ERROR: frameoffset != READSIZE\n");
		debug_printf ("ERROR: frameoffset != READSIZE\n");
		fp = fopen ("gcs1.txt","w");
		fwrite(ds->mpgbuf,1,ds->mpgsize,fp);
		fclose(fp);
		exit (-1);
	    }
	}
    }
    debug_printf ("%p | %p | %p |< %p | %p >| %d\n",
		  ds->mpgbuf, 
		  ds->mpgpos, 
		  &(ds->mpgbuf[ds->mpgpos]), 
		  ms->this_frame, 
		  ms->next_frame, 
		  frameoffset);

    mad_stream_buffer (ms, (const unsigned char*)
		       (ds->mpgbuf+ds->mpgpos)-frameoffset, 
		       READSIZE);
    ds->mpgpos += READSIZE - frameoffset;

    return MAD_FLOW_CONTINUE;
}

void
search_for_silence (DECODE_STRUCT *ds, double vol)
{
    int i;
    for(i = 0; i < NUM_SILTRACKERS; i++) {
	SILENCETRACKER *pstracker = &ds->siltrackers[i];

	if (pstracker->foundsil)
	    continue;

	if (vol < pstracker->silencevol) {
	    if (pstracker->insilencecount == 0) {
		pstracker->silstart_samp = ds->pcmpos;
	    }
	    pstracker->insilencecount++;
	} else {
	    pstracker->insilencecount = 0;
	}

	if (pstracker->insilencecount > ds->silence_samples) {
	    pstracker->foundsil = TRUE;
	}
    }
}

signed int
scale(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - 16));

    /* clip */
    if (sample >= MAD_F_ONE)
	sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
	sample = -MAD_F_ONE;

    /* quantize */
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static enum mad_flow 
filter (void *data, struct mad_stream const *ms, struct mad_frame *frame)
{
    DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
    FRAME_LIST* fl;

    fl = (FRAME_LIST*) malloc (sizeof(FRAME_LIST));
    fl->m_framepos = ms->this_frame;
    fl->m_samples = 0;
    fl->m_pcmpos = 0;
    list_add_tail (&(fl->m_list), &(ds->frame_list));

#if defined (commentout)
    debug_printf ("FILTER: %p (%02x%02x) | %p\n", 
		  ms->this_frame, 
		  ms->this_frame[0], 
		  ms->this_frame[1], 
		  ms->next_frame);
#endif

    return MAD_FLOW_CONTINUE;
}

enum mad_flow
output (void *data, struct mad_header const *header,
	struct mad_pcm *pcm)
{
    DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
    FRAME_LIST *fl;
    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    static short lastSample = 0;
    static signed int sample;
    double v;

    nchannels = pcm->channels;
    nsamples  = pcm->length;
    left_ch   = pcm->samples[0];
    right_ch  = pcm->samples[1];

    /* Get frame entry */
    fl = list_entry (ds->frame_list.prev, FRAME_LIST, m_list);
    fl->m_samples = nsamples;
    fl->m_pcmpos = ds->pcmpos;

    if (ds->pcmpos > ds->len_to_sw_start_samp
	&& ds->pcmpos < ds->len_to_sw_end_samp) {
	debug_printf ("* %d\n", ds->pcmpos);
    } else {
	debug_printf ("- %d\n", ds->pcmpos);
    }
#if defined (commentout)
#endif

    while(nsamples--) {
	/* output sample(s) in 16-bit signed little-endian PCM */
	/* GCS FIX: Does this work on big endian machines??? */
	lastSample = sample;
	sample = (short) scale (*left_ch++);
	//	fwrite(&sample, sizeof(short), 1, fp);

	if (nchannels == 2) {
	    // make mono
	    sample = (sample+scale(*right_ch++))/2;
	}

	// get the instantanous volume
	v = (lastSample*lastSample)+(sample*sample);
	v = sqrt(v / 2);
	if (ds->pcmpos > ds->len_to_sw_start_samp
	    && ds->pcmpos < ds->len_to_sw_end_samp)
	{
	    search_for_silence(ds, v);
	}
	ds->pcmpos++;
    }
    
    return MAD_FLOW_CONTINUE;
}

static enum 
mad_flow header(void *data, struct mad_header const *pheader)
{
    DECODE_STRUCT *ds = (DECODE_STRUCT *)data;
    if (!ds->samplerate) {
	ds->samplerate = pheader->samplerate;
	ds->silence_samples = ds->silence_ms * (ds->samplerate/1000);
	ds->len_to_sw_start_samp = ds->len_to_sw_ms * (ds->samplerate/1000);
	ds->len_to_sw_end_samp = (ds->len_to_sw_ms + ds->searchwindow_ms) 
		* (ds->samplerate/1000);
	debug_printf ("Setting samplerate: %ld\n",ds->samplerate);
    }
    return MAD_FLOW_CONTINUE;
}

enum mad_flow
error(void *data, struct mad_stream *ms, struct mad_frame *frame)
{
    if (MAD_RECOVERABLE(ms->error)) {
	debug_printf("mad error 0x%04x\n", ms->error);
	return MAD_FLOW_CONTINUE;
    }

    debug_printf("unrecoverable mad error 0x%04x\n", ms->error);
    return MAD_FLOW_BREAK;
}

/* The following routines have nothing to do with finding a separation 
 * point. Instead, they have to do with finding the bitrate.  However, 
 * they are included here because they are "mad" related.
 */
error_code
find_bitrate(unsigned long* bitrate, const u_char* mpgbuf, long mpgsize)
{
    struct mad_decoder decoder;
    GET_BITRATE_STRUCT gbs;
    int result;
    
    /* initialize and start decoder */
    gbs.mpgbuf = (unsigned char*) mpgbuf;
    gbs.mpgsize = mpgsize;
    gbs.bitrate = 0;
    mad_decoder_init(&decoder,
		     &gbs,
		     input_get_bitrate /* input */,
		     header_get_bitrate /* header */,
		     NULL /* filter */,
		     NULL /* output */,
		     NULL /* error */,
		     NULL /* message */);
    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
    mad_decoder_finish(&decoder);
    *bitrate = gbs.bitrate;
    return SR_SUCCESS;
}

static enum mad_flow
input_get_bitrate (void *data, struct mad_stream *stream)
{
    GET_BITRATE_STRUCT* gbs = (GET_BITRATE_STRUCT*) data;

    if (!gbs->mpgsize)
	return MAD_FLOW_STOP;

    mad_stream_buffer(stream, gbs->mpgbuf, gbs->mpgsize);
    gbs->mpgsize = 0;
    return MAD_FLOW_CONTINUE;
}

static enum mad_flow
header_get_bitrate (void *data, struct mad_header const *pheader)
{
    GET_BITRATE_STRUCT* gbs = (GET_BITRATE_STRUCT*) data;

    gbs->bitrate = pheader->bitrate;	/* stream bitrate (bps) */
    debug_printf ("Decoded bitrate from stream: %ld\n", gbs->bitrate);
    return MAD_FLOW_STOP;
}
