/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    recache.c

    Code related to resample cache.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdlib.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "mblock.h"
#include "recache.h"
#include "resample.h"

#define HASH_TABLE_SIZE 251
#define MIXLEN 256

#define MIN_LOOPSTART MIXLEN
#define MIN_LOOPLEN 1024
#define MAX_EXPANDLEN (1024 * 32)
#define CACHE_DATA_LEN (allocate_cache_size / sizeof(sample_t))

#define sp_hash(sp, note) ((unsigned int) (sp) + (unsigned int) (note))
#define CACHE_RESAMPLING_OK 0
#define CACHE_RESAMPLING_NOTOK 1
#define SORT_THRESHOLD 20
#define RESAMPLATION_CACHE _x = do_resamplation(src, ofs, &resrc); \
		dest[i] = (int16) ((_x > 32767) ? 32767 \
				: ((_x < -32768) ? -32768 : _x))

static sample_t *cache_data = NULL;
int32 allocate_cache_size = DEFAULT_CACHE_DATA_SIZE;
static splen_t cache_data_len;
static struct cache_hash *cache_hash_table[HASH_TABLE_SIZE];
static MBlockList hash_entry_pool;

static struct {
	int32 on[128];
	struct cache_hash *cache[128];
} channel_note_table[MAX_CHANNELS];

static double sample_resamp_info(Sample *, int,
		splen_t *, splen_t *, splen_t *);
static void qsort_cache_array(struct cache_hash **, long, long);
static void insort_cache_array(struct cache_hash **, long);
static int cache_resampling(struct cache_hash *);
static void loop_connect(sample_t *, int32, int32);

void resamp_cache_free_completely()
{
	if ( cache_data )
	{
		free( cache_data );
		cache_data = 0;
	}

	reuse_mblock(&hash_entry_pool);
}


void resamp_cache_free(void)
{
	if ( cache_data )
	{
		free( cache_data );
		cache_data = 0;
	}
}

void resamp_cache_reset(void)
{
	if (cache_data == NULL) {
		cache_data = (sample_t *)
				safe_large_malloc((CACHE_DATA_LEN + 1) * sizeof(sample_t));
		memset(cache_data, 0, (CACHE_DATA_LEN + 1) * sizeof(sample_t));
		init_mblock(&hash_entry_pool);
	}
	cache_data_len = 0;
	memset(cache_hash_table, 0, sizeof(cache_hash_table));
	memset(channel_note_table, 0, sizeof(channel_note_table));
	reuse_mblock(&hash_entry_pool);
}

struct cache_hash *resamp_cache_fetch(Sample *sp, int note)
{
	unsigned int addr;
	struct cache_hash *p;
	
	if (sp->vibrato_control_ratio || (sp->modes & MODES_PINGPONG)
			|| (sp->sample_rate == play_mode->rate
			&& sp->root_freq == get_note_freq(sp, sp->note_to_use)))
		return NULL;
	addr = sp_hash(sp, note) % HASH_TABLE_SIZE;
	p = cache_hash_table[addr];
	while (p && (p->note != note || p->sp != sp))
		p = p->next;
	if (p && p->resampled != NULL)
		return p;
	return NULL;
}

void resamp_cache_refer_on(Voice *vp, int32 sample_start)
{
	unsigned int addr;
	struct cache_hash *p;
	int note, ch;
	
	ch = vp->channel;
	if (vp->vibrato_control_ratio || channel[ch].portamento
			|| (vp->sample->modes & MODES_PINGPONG)
			|| vp->orig_frequency != vp->frequency
			|| (vp->sample->sample_rate == play_mode->rate
			&& vp->sample->root_freq
			== get_note_freq(vp->sample, vp->sample->note_to_use)))
		return;
	note = vp->note;
	if (channel_note_table[ch].cache[note])
		resamp_cache_refer_off(ch, note, sample_start);
	addr = sp_hash(vp->sample, note) % HASH_TABLE_SIZE;
	p = cache_hash_table[addr];
	while (p && (p->note != note || p->sp != vp->sample))
		p = p->next;
	if (! p) {
		p = (struct cache_hash *)
		new_segment(&hash_entry_pool, sizeof(struct cache_hash));
		p->cnt = 0;
		p->note = vp->note;
		p->sp = vp->sample;
		p->resampled = NULL;
		p->next = cache_hash_table[addr];
		cache_hash_table[addr] = p;
	}
	channel_note_table[ch].cache[note] = p;
	channel_note_table[ch].on[note] = sample_start;
}

void resamp_cache_refer_off(int ch, int note, int32 sample_end)
{
	int32 sample_start, len;
	struct cache_hash *p;
	Sample *sp;
	
	p = channel_note_table[ch].cache[note];
	if (p == NULL)
		return;
	sp = p->sp;
	if (sp->sample_rate == play_mode->rate
			&& sp->root_freq == get_note_freq(sp, sp->note_to_use))
		return;
	sample_start = channel_note_table[ch].on[note];
	len = sample_end - sample_start;
	if (len < 0) {
		channel_note_table[ch].cache[note] = NULL;
		return;
	}
	if (! (sp->modes & MODES_LOOPING)) {
		double a;
		int32 slen;
		
		a = ((double) sp->root_freq * play_mode->rate)
				/ ((double) sp->sample_rate * get_note_freq(sp, note));
		slen = (int32) ((sp->data_length >> FRACTION_BITS) * a);
		if (len > slen)
			len = slen;
	}
	p->cnt += len;
	channel_note_table[ch].cache[note] = NULL;
}

void resamp_cache_refer_alloff(int ch, int32 sample_end)
{
	int i;
	
	for (i = 0; i < 128; i++)
		resamp_cache_refer_off(ch, i, sample_end);
}

void resamp_cache_create(void)
{
	int i, skip;
	int32 n, t1, t2, total;
	struct cache_hash **array;
	
	/* It is NP completion that solve the best cache hit rate.
	 * So I thought better algorism O(n log n), but not a best solution.
	 * Follows implementation takes good hit rate, and it is fast.
	 */
	n = t1 = t2 = 0;
	total = 0;
	/* set size per count */
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		struct cache_hash *p, *q;
		
		p = cache_hash_table[i], q = NULL;
		while (p) {
			struct cache_hash *tmp;
			
			t1 += p->cnt;
			tmp = p, p = p->next;
			if (tmp->cnt > 0) {
				Sample *sp;
				splen_t newlen;
				
				sp = tmp->sp;
				sample_resamp_info(sp, tmp->note, NULL, NULL, &newlen);
				if (newlen > 0) {
					total += tmp->cnt;
					tmp->r = (double) newlen / tmp->cnt;
					tmp->next = q, q = tmp;
					n++;
				}
			}
		}
		cache_hash_table[i] = q;
	}
	if (n == 0) {
		ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "No pre-resampling cache hit");
		return;
	}
	array = (struct cache_hash **) new_segment(&hash_entry_pool,
			n * sizeof(struct cache_hash *));
	n = 0;
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		struct cache_hash *p;
		
		for (p = cache_hash_table[i]; p; p = p->next)
			array[n++] = p;
	}
	if (total > CACHE_DATA_LEN)
		qsort_cache_array(array, 0, n - 1);
	skip = 0;
	for (i = 0; i < n; i++) {
		if (array[i]->r != 0
				&& cache_resampling(array[i]) == CACHE_RESAMPLING_OK)
			t2 += array[i]->cnt;
		else
			skip++;
	}
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
			"Resample cache: Key %d/%d(%.1f%%) Sample %.1f%c/%.1f%c(%.1f%%)",
			n - skip, n, 100.0 * (n - skip) / n,
			t2 / ((t2 >= 1048576) ? 1048576.0 : 1024.0),
			(t2 >= 1048576) ? 'M' : 'K',
			t1 / ((t1 >= 1048576) ? 1048576.0 : 1024.0),
			(t1 >= 1048576) ? 'M' : 'K',
			100.0 * t2 / t1);
	/* update cache_hash_table */
	if (skip)
		for (i = 0; i < HASH_TABLE_SIZE; i++) {
			struct cache_hash *p, *q;
			
			p = cache_hash_table[i], q = NULL;
			while (p) {
				struct cache_hash *tmp;
				
				tmp = p, p = p->next;
				if (tmp->resampled)
					tmp->next = q, q = tmp;
			}
			cache_hash_table[i] = q;
		}
}

static double sample_resamp_info(Sample *sp, int note,
		splen_t *loop_start, splen_t *loop_end, splen_t *data_length)
{
	splen_t xls, xle, ls, le, ll, newlen;
	double a, xxls, xxle, xn;
	
	a = ((double) sp->sample_rate * get_note_freq(sp, note))
			/ ((double) sp->root_freq * play_mode->rate);
	a = TIM_FSCALENEG((double) (int32) TIM_FSCALE(a, FRACTION_BITS),
			FRACTION_BITS);
	xn = sp->data_length / a;
	if (xn >= SPLEN_T_MAX) {
		/* Ignore this sample */
		*data_length = 0;
		return 0.0;
	}
	newlen = (splen_t) (TIM_FSCALENEG(xn, FRACTION_BITS) + 0.5);
	ls = sp->loop_start;
	le = sp->loop_end;
	ll = le - ls;
	xxls = ls / a + 0.5;
	if (xxls >= SPLEN_T_MAX) {
		/* Ignore this sample */
		*data_length = 0;
		return 0.0;
	}
	xls = (splen_t) xxls;
	xxle = le / a + 0.5;
	if (xxle >= SPLEN_T_MAX) {
		/* Ignore this sample */
		*data_length = 0;
		return 0.0;
	}
	xle = (splen_t) xxle;
	if ((sp->modes & MODES_LOOPING)
			&& ((xle - xls) >> FRACTION_BITS) < MIN_LOOPLEN) {
		splen_t n;
		splen_t newxle;
		double xl;	/* Resampled new loop length */
		double xnewxle;
		
		xl = ll / a;
		if (xl >= SPLEN_T_MAX) {
			/* Ignore this sample */
			*data_length = 0;
			return 0.0;
		}
		n = (splen_t) (0.0001 + MIN_LOOPLEN
				/ TIM_FSCALENEG(xl, FRACTION_BITS)) + 1;
		xnewxle = le / a + n * xl + 0.5;
		if (xnewxle >= SPLEN_T_MAX) {
			/* Ignore this sample */
			*data_length = 0;
			return 0.0;
		}
		newxle = (splen_t) xnewxle;
		newlen += (newxle - xle) >> FRACTION_BITS;
		xle = newxle;
	}
	if (loop_start)
		*loop_start = (splen_t) (xls & ~FRACTION_MASK);
	if (loop_end)
		*loop_end = (splen_t) (xle & ~FRACTION_MASK);
	*data_length = newlen << FRACTION_BITS;
	return a;
}

static void qsort_cache_array(struct cache_hash **a, long first, long last)
{
	long i = first, j = last;
	struct cache_hash *x, *t;
	
	if (j - i < SORT_THRESHOLD) {
		insort_cache_array(a + i, j - i + 1);
		return;
	}
	x = a[(first + last) / 2];
	for (;;) {
		while (a[i]->r < x->r)
			i++;
		while (x->r < a[j]->r)
			j--;
		if (i >= j)
			break;
		t = a[i], a[i] = a[j], a[j] = t;
		i++, j--;
	}
	if (first < i - 1)
		qsort_cache_array(a, first, i - 1);
	if (j + 1 < last)
		qsort_cache_array(a, j + 1, last);
}

static void insort_cache_array(struct cache_hash **data, long n)
{
	long i, j;
	struct cache_hash *x;
	
	for (i = 1; i < n; i++) {
		x = data[i];
		for (j = i - 1; j >= 0 && x->r < data[j]->r; j--)
			data[j + 1] = data[j];
		data[j + 1] = x;
	}
}

static int cache_resampling(struct cache_hash *p)
{
	Sample *sp, *newsp;
	sample_t *src, *dest;
	splen_t newlen, ofs, le, ls, ll, xls, xle;
	int32 i, incr, _x;
	resample_rec_t resrc;
	double a;
	int8 note;
	
	sp = p->sp;
	if (sp->note_to_use)
		note = sp->note_to_use;
	else
		note = p->note;
	a = sample_resamp_info(sp, note, &xls, &xle, &newlen);
	if (newlen == 0)
		return CACHE_RESAMPLING_NOTOK;
	newlen >>= FRACTION_BITS;
	if (cache_data_len + newlen + 1 > CACHE_DATA_LEN)
		return CACHE_RESAMPLING_NOTOK;
	resrc.loop_start = ls = sp->loop_start;
	resrc.loop_end = le = sp->loop_end;
	resrc.data_length = sp->data_length;
	ll = sp->loop_end - sp->loop_start;
	dest = cache_data + cache_data_len;
	src = sp->data;
	newsp = (Sample *) new_segment(&hash_entry_pool, sizeof(Sample));
	memcpy(newsp, sp, sizeof(Sample));
	newsp->data = dest;
	ofs = 0;
	incr = (splen_t) (TIM_FSCALE(a, FRACTION_BITS) + 0.5);
	if (sp->modes & MODES_LOOPING)
		for (i = 0; i < newlen; i++) {
			if (ofs >= le)
				ofs -= ll;
			RESAMPLATION_CACHE;
			ofs += incr;
		}
	else
		for (i = 0; i < newlen; i++) {
			RESAMPLATION_CACHE;
			ofs += incr;
		}
	newsp->loop_start = xls;
	newsp->loop_end = xle;
	newsp->data_length = newlen << FRACTION_BITS;
	if (sp->modes & MODES_LOOPING)
		loop_connect(dest, (int32) (xls >> FRACTION_BITS),
				(int32) (xle >> FRACTION_BITS));
	dest[xle >> FRACTION_BITS] = dest[xls >> FRACTION_BITS];
	newsp->root_freq = get_note_freq(newsp, note);
	newsp->sample_rate = play_mode->rate;
	p->resampled = newsp;
	cache_data_len += newlen + 1;
	return CACHE_RESAMPLING_OK;
}

static void loop_connect(sample_t *data, int32 start, int32 end)
{
	int i, mixlen;
	int32 t0, t1;
	
	mixlen = MIXLEN;
	if (start < mixlen)
		mixlen = start;
	if (end - start < mixlen)
		mixlen = end - start;
	if (mixlen <= 0)
		return;
	t0 = start - mixlen;
	t1 = end   - mixlen;
	for (i = 0; i < mixlen; i++) {
		double x, b;
		
		b = i / (double) mixlen;	/* 0 <= b < 1 */
		x = b * data[t0 + i] + (1.0 - b) * data[t1 + i];
#ifdef LOOKUP_HACK
		if (x < -128)
			data[t1 + i] = -128;
		else if (x > 127)
			data[t1 + i] = 127;
#else
		if (x < -32768)
			data[t1 + i] = -32768;
		else if (x > 32767)
			data[t1 + i] = 32767;
#endif	/* LOOKUP_HACK */
		else
			data[t1 + i] = (sample_t) x;
	}
}

