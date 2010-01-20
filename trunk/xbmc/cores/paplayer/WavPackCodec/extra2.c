////////////////////////////////////////////////////////////////////////////
//			     **** WAVPACK ****				  //
//		    Hybrid Lossless Wavefile Compressor			  //
//		Copyright (c) 1998 - 2005 Conifer Software.		  //
//			    All Rights Reserved.			  //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// extra2.c

// This module handles the "extra" mode for stereo files.

#include "wavpack.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// #define EXTRA_DUMP

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

extern const char default_terms [], high_terms [], fast_terms [];

// #define MINMAX_WEIGHTS

#ifdef MINMAX_WEIGHTS
static int32_t min_weight, max_weight;
static int min_term, max_term;
#endif

static void decorr_stereo_pass (int32_t *in_samples, int32_t *out_samples, int32_t num_samples, struct decorr_pass *dpp, int dir)
{
    int m = 0;

    dpp->sum_A = dpp->sum_B = 0;

#ifdef MINMAX_WEIGHTS
    dpp->min = dpp->max = 0;
#endif

    if (dir < 0) {
	out_samples += (num_samples - 1) * 2;
	in_samples += (num_samples - 1) * 2;
	dir = -2;
    }
    else
	dir = 2;

    if (dpp->term == 17) {
	while (num_samples--) {
	    int32_t left, right;
	    int32_t sam_A, sam_B;

	    sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
	    dpp->samples_A [1] = dpp->samples_A [0];
	    dpp->samples_A [0] = left = in_samples [0];
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    out_samples [0] = left;

	    sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
	    dpp->samples_B [1] = dpp->samples_B [0];
	    dpp->samples_B [0] = right = in_samples [1];
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_B += dpp->weight_B;
	    out_samples [1] = right;

#ifdef MINMAX_WEIGHTS
	    if (dpp->weight_A > dpp->max) dpp->max = dpp->weight_A;
	    if (dpp->weight_B > dpp->max) dpp->max = dpp->weight_B;
	    if (dpp->weight_A < dpp->min) dpp->min = dpp->weight_A;
	    if (dpp->weight_B < dpp->min) dpp->min = dpp->weight_B;
#endif
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term == 18) {
	while (num_samples--) {
	    int32_t left, right;
	    int32_t sam_A, sam_B;

	    sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
	    dpp->samples_A [1] = dpp->samples_A [0];
	    dpp->samples_A [0] = left = in_samples [0];
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    out_samples [0] = left;

	    sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
	    dpp->samples_B [1] = dpp->samples_B [0];
	    dpp->samples_B [0] = right = in_samples [1];
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_B += dpp->weight_B;
	    out_samples [1] = right;

#ifdef MINMAX_WEIGHTS
	    if (dpp->weight_A > dpp->max) dpp->max = dpp->weight_A;
	    if (dpp->weight_B > dpp->max) dpp->max = dpp->weight_B;
	    if (dpp->weight_A < dpp->min) dpp->min = dpp->weight_A;
	    if (dpp->weight_B < dpp->min) dpp->min = dpp->weight_B;
#endif
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term > 0) {
	while (num_samples--) {
	    int k = (m + dpp->term) & (MAX_TERM - 1);
	    int32_t left, right;
	    int32_t sam_A, sam_B;

	    sam_A = dpp->samples_A [m];
	    dpp->samples_A [k] = left = in_samples [0];
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    out_samples [0] = left;

	    sam_B = dpp->samples_B [m];
	    dpp->samples_B [k] = right = in_samples [1];
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_B += dpp->weight_B;
	    out_samples [1] = right;

#ifdef MINMAX_WEIGHTS
	    if (dpp->weight_A > dpp->max) dpp->max = dpp->weight_A;
	    if (dpp->weight_B > dpp->max) dpp->max = dpp->weight_B;
	    if (dpp->weight_A < dpp->min) dpp->min = dpp->weight_A;
	    if (dpp->weight_B < dpp->min) dpp->min = dpp->weight_B;
#endif
	    in_samples += dir;
	    out_samples += dir;
	    m = (m + 1) & (MAX_TERM - 1);
	}
    }
    else if (dpp->term == -1) {
	while (num_samples--) {
	    int32_t left = in_samples [0];
	    int32_t right = in_samples [1];
	    int32_t sam_A = dpp->samples_A [0], sam_B = left;

	    dpp->samples_A [0] = right;
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight_clip (dpp->weight_B, dpp->delta, sam_B, right);
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight_clip (dpp->weight_A, dpp->delta, sam_A, left);
	    dpp->sum_A += dpp->weight_A;
	    dpp->sum_B += dpp->weight_B;
#ifdef MINMAX_WEIGHTS
	    if (dpp->weight_A > dpp->max) dpp->max = dpp->weight_A;
	    if (dpp->weight_B > dpp->max) dpp->max = dpp->weight_B;
	    if (dpp->weight_A < dpp->min) dpp->min = dpp->weight_A;
	    if (dpp->weight_B < dpp->min) dpp->min = dpp->weight_B;
#endif
	    out_samples [0] = left;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term == -2) {
	while (num_samples--) {
	    int32_t left = in_samples [0];
	    int32_t right = in_samples [1];
	    int32_t sam_B = dpp->samples_B [0], sam_A = right;

	    dpp->samples_B [0] = left;
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight_clip (dpp->weight_A, dpp->delta, sam_A, left);
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight_clip (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_A += dpp->weight_A;
	    dpp->sum_B += dpp->weight_B;
#ifdef MINMAX_WEIGHTS
	    if (dpp->weight_A > dpp->max) dpp->max = dpp->weight_A;
	    if (dpp->weight_B > dpp->max) dpp->max = dpp->weight_B;
	    if (dpp->weight_A < dpp->min) dpp->min = dpp->weight_A;
	    if (dpp->weight_B < dpp->min) dpp->min = dpp->weight_B;
#endif
	    out_samples [0] = left;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }
    else if (dpp->term == -3) {
	while (num_samples--) {
	    int32_t left = in_samples [0];
	    int32_t right = in_samples [1];
	    int32_t sam_A = dpp->samples_A [0], sam_B = dpp->samples_B [0];

	    dpp->samples_A [0] = right;
	    dpp->samples_B [0] = left;
	    left -= apply_weight (dpp->weight_A, sam_A);
	    update_weight_clip (dpp->weight_A, dpp->delta, sam_A, left);
	    right -= apply_weight (dpp->weight_B, sam_B);
	    update_weight_clip (dpp->weight_B, dpp->delta, sam_B, right);
	    dpp->sum_A += dpp->weight_A;
	    dpp->sum_B += dpp->weight_B;
#ifdef MINMAX_WEIGHTS
	    if (dpp->weight_A > dpp->max) dpp->max = dpp->weight_A;
	    if (dpp->weight_B > dpp->max) dpp->max = dpp->weight_B;
	    if (dpp->weight_A < dpp->min) dpp->min = dpp->weight_A;
	    if (dpp->weight_B < dpp->min) dpp->min = dpp->weight_B;
#endif
	    out_samples [0] = left;
	    out_samples [1] = right;
	    in_samples += dir;
	    out_samples += dir;
	}
    }

#ifdef MINMAX_WEIGHTS
    if (dpp->term != 0) {
	if (dpp->max > max_weight) { max_weight = dpp->max; max_term = dpp->term; }
	if (dpp->min < min_weight) { min_weight = dpp->min; min_term = dpp->term; }
    }
#endif

    if (m && dpp->term > 0 && dpp->term <= MAX_TERM) {
	int32_t temp_A [MAX_TERM], temp_B [MAX_TERM];
	int k;

	memcpy (temp_A, dpp->samples_A, sizeof (dpp->samples_A));
	memcpy (temp_B, dpp->samples_B, sizeof (dpp->samples_B));

	for (k = 0; k < MAX_TERM; k++) {
	    dpp->samples_A [k] = temp_A [m];
	    dpp->samples_B [k] = temp_B [m];
	    m = (m + 1) & (MAX_TERM - 1);
	}
    }
}

static void reverse_decorr (struct decorr_pass *dpp)
{
    if (dpp->term > MAX_TERM) {
	int32_t sam_A, sam_B;

	if (dpp->term & 1) {
	    sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
	    sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
	}
	else {
	    sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
	    sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
	}

	dpp->samples_A [1] = dpp->samples_A [0];
	dpp->samples_B [1] = dpp->samples_B [0];
	dpp->samples_A [0] = sam_A;
	dpp->samples_B [0] = sam_B;

	if (dpp->term & 1) {
	    sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
	    sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
	}
	else {
	    sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
	    sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
	}

	dpp->samples_A [1] = sam_A;
	dpp->samples_B [1] = sam_B;
    }
    else if (dpp->term > 1) {
	int i = 0, j = dpp->term - 1, cnt = dpp->term / 2;

	while (cnt--) {
	    i &= (MAX_TERM - 1);
	    j &= (MAX_TERM - 1);
	    dpp->samples_A [i] ^= dpp->samples_A [j];
	    dpp->samples_A [j] ^= dpp->samples_A [i];
	    dpp->samples_A [i] ^= dpp->samples_A [j];
	    dpp->samples_B [i] ^= dpp->samples_B [j];
	    dpp->samples_B [j] ^= dpp->samples_B [i];
	    dpp->samples_B [i++] ^= dpp->samples_B [j--];
	}

//	CLEAR (dpp->samples_A);
//	CLEAR (dpp->samples_B);
    }
    else if (dpp->term == -1) {
    }
    else if (dpp->term == -2) {
    }
    else if (dpp->term == -3) {
    }
}

static void decorr_stereo_buffer (int32_t *samples, int32_t *outsamples, int32_t num_samples, struct decorr_pass *dpp)
{
    int delta = dpp->delta, pre_delta;
    int term = dpp->term;
    struct decorr_pass dp;

    if (delta == 7)
	pre_delta = 7;
    else if (delta < 2)
	pre_delta = 3;
    else
	pre_delta = delta + 1;

    CLEAR (dp);
    dp.term = term;
    dp.delta = pre_delta;
    decorr_stereo_pass (samples, outsamples, num_samples > 2048 ? 2048 : num_samples, &dp, -1);
    dp.delta = delta;
    reverse_decorr (&dp);
    memcpy (dpp->samples_A, dp.samples_A, sizeof (dp.samples_A));
    memcpy (dpp->samples_B, dp.samples_B, sizeof (dp.samples_B));
    dpp->weight_A = dp.weight_A;
    dpp->weight_B = dp.weight_B;

    if (delta == 0) {
	dp.delta = 1;
	decorr_stereo_pass (samples, outsamples, num_samples, &dp, 1);
	dp.delta = 0;
	memcpy (dp.samples_A, dpp->samples_A, sizeof (dp.samples_A));
	memcpy (dp.samples_B, dpp->samples_B, sizeof (dp.samples_B));
	dpp->weight_A = dp.weight_A = dp.sum_A / num_samples;
	dpp->weight_B = dp.weight_B = dp.sum_B / num_samples;
    }

//    if (memcmp (dpp, &dp, sizeof (dp)))
//	error_line ("decorr_passes don't match, delta = %d", delta);

    decorr_stereo_pass (samples, outsamples, num_samples, &dp, 1);
}

static void recurse_stereo (WavpackContext *wpc, int32_t *sampleptrs[], struct decorr_pass dps[],
    int depth, int nterms, int delta, uint32_t input_bits, uint32_t *best_bits)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int term, branches = ((wpc->config.extra_flags & EXTRA_BRANCHES) >> 6) - depth;
    int32_t *samples, *outsamples;
    uint32_t term_bits [22], bits;

    if (branches < 1 || depth + 1 == nterms)
	branches = 1;

    CLEAR (term_bits);
    samples = sampleptrs [depth];
    outsamples = sampleptrs [depth + 1];

    for (term = -3; term <= 18; ++term) {
	if (!term)
	    continue;

	if (term == 17 && branches == 1 && depth + 1 < nterms)
	    continue;

	if (term == -1 || term == -2)
	    if (!(wps->wphdr.flags & CROSS_DECORR))
		continue;

	if (term >= 9 && term <= 16)
	    if (term > MAX_TERM || !(wpc->config.flags & CONFIG_HIGH_FLAG) || (wpc->config.extra_flags & EXTRA_SKIP_8TO16))
		continue;

	if ((wpc->config.flags & CONFIG_FAST_FLAG) && (term >= 5 && term <= 16))
	    continue;

	dps [depth].term = term;
	dps [depth].delta = delta;
	decorr_stereo_buffer (samples, outsamples, wps->wphdr.block_samples, &dps [depth]);
	bits = log2buffer (outsamples, wps->wphdr.block_samples * 2);

	if (bits < *best_bits) {
	    *best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, dps, sizeof (dps [0]) * (depth + 1));
	    memcpy (sampleptrs [nterms + 1], sampleptrs [depth + 1], wps->wphdr.block_samples * 8);
	}

	term_bits [term + 3] = bits;
    }

    while (depth + 1 < nterms && branches--) {
	uint32_t local_best_bits = input_bits;
	int best_term = 0, i;

	for (i = 0; i < 22; ++i)
	    if (term_bits [i] && term_bits [i] < local_best_bits) {
		local_best_bits = term_bits [i];
		term_bits [i] = 0;
		best_term = i - 3;
	    }

	if (!best_term)
	    break;

	dps [depth].term = best_term;
	dps [depth].delta = delta;
	decorr_stereo_buffer (samples, outsamples, wps->wphdr.block_samples, &dps [depth]);

//	if (log2buffer (outsamples, wps->wphdr.block_samples * 2) != local_best_bits)
//	    error_line ("data doesn't match!");

	recurse_stereo (wpc, sampleptrs, dps, depth + 1, nterms, delta, local_best_bits, best_bits);
    }
}

static void delta_stereo (WavpackContext *wpc, int32_t *sampleptrs[], struct decorr_pass dps[],
    int nterms, uint32_t *best_bits)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int lower = FALSE;
    int delta, d;
    uint32_t bits;

    if (wps->decorr_passes [0].term)
	delta = wps->decorr_passes [0].delta;
    else
	return;

    for (d = delta - 1; d >= 0; --d) {
	int i;

	if (!d && (wps->wphdr.flags & HYBRID_FLAG))
	    break;

	for (i = 0; i < nterms && wps->decorr_passes [i].term; ++i) {
	    dps [i].term = wps->decorr_passes [i].term;
	    dps [i].delta = d;
	    decorr_stereo_buffer (sampleptrs [i], sampleptrs [i+1], wps->wphdr.block_samples, &dps [i]);
	}

	bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);

	if (bits < *best_bits) {
	    lower = TRUE;
	    *best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, dps, sizeof (dps [0]) * i);
	    memcpy (sampleptrs [nterms + 1], sampleptrs [i], wps->wphdr.block_samples * 8);
	}
	else
	    break;
    }

    for (d = delta + 1; !lower && d <= 7; ++d) {
	int i;

	for (i = 0; i < nterms && wps->decorr_passes [i].term; ++i) {
	    dps [i].term = wps->decorr_passes [i].term;
	    dps [i].delta = d;
	    decorr_stereo_buffer (sampleptrs [i], sampleptrs [i+1], wps->wphdr.block_samples, &dps [i]);
	}

	bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);

	if (bits < *best_bits) {
	    *best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, dps, sizeof (dps [0]) * i);
	    memcpy (sampleptrs [nterms + 1], sampleptrs [i], wps->wphdr.block_samples * 8);
	}
	else
	    break;
    }
}

static void sort_stereo (WavpackContext *wpc, int32_t *sampleptrs[], struct decorr_pass dps[],
    int nterms, uint32_t *best_bits)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int reversed = TRUE;
    uint32_t bits;

    while (reversed) {
	int ri, i;

	memcpy (dps, wps->decorr_passes, sizeof (wps->decorr_passes));
	reversed = FALSE;

	for (ri = 0; ri < nterms && wps->decorr_passes [ri].term; ++ri) {

	    if (ri + 1 >= nterms || !wps->decorr_passes [ri+1].term)
		break;

	    if (wps->decorr_passes [ri].term == wps->decorr_passes [ri+1].term) {
		decorr_stereo_buffer (sampleptrs [ri], sampleptrs [ri+1], wps->wphdr.block_samples, &dps [ri]);
		continue;
	    }

	    dps [ri] = wps->decorr_passes [ri+1];
	    dps [ri+1] = wps->decorr_passes [ri];

	    for (i = ri; i < nterms && wps->decorr_passes [i].term; ++i)
		decorr_stereo_buffer (sampleptrs [i], sampleptrs [i+1], wps->wphdr.block_samples, &dps [i]);

	    bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);

	    if (bits < *best_bits) {
		reversed = TRUE;
		*best_bits = bits;
		CLEAR (wps->decorr_passes);
		memcpy (wps->decorr_passes, dps, sizeof (dps [0]) * i);
		memcpy (sampleptrs [nterms + 1], sampleptrs [i], wps->wphdr.block_samples * 8);
	    }
	    else {
		dps [ri] = wps->decorr_passes [ri];
		dps [ri+1] = wps->decorr_passes [ri+1];
		decorr_stereo_buffer (sampleptrs [ri], sampleptrs [ri+1], wps->wphdr.block_samples, &dps [ri]);
	    }
	}
    }
}

#define EXTRA_ADVANCED (EXTRA_BRANCHES | EXTRA_SORT_FIRST | EXTRA_SORT_LAST | EXTRA_TRY_DELTAS)

void analyze_stereo (WavpackContext *wpc, int32_t *samples)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
#ifdef EXTRA_DUMP
    uint32_t bits, best_bits, default_bits, cnt;
#else
    uint32_t bits, best_bits, cnt;
#endif
    const char *decorr_terms = default_terms, *tp;
    int32_t *sampleptrs [MAX_NTERMS+2], *lptr;
    struct decorr_pass dps [MAX_NTERMS];
    int nterms, i;


    CLEAR (wps->decorr_passes);
    cnt = wps->wphdr.block_samples * 2;
    lptr = samples;

    while (cnt--)
	if (*lptr++)
	    break;

    if (cnt == (uint32_t) -1) {
	scan_word (wps, samples, wps->wphdr.block_samples, -1);
	wps->num_terms = 0;
	return;
    }

    if (wpc->config.flags & CONFIG_HIGH_FLAG)
	decorr_terms = high_terms;
    else if (wpc->config.flags & CONFIG_FAST_FLAG)
	decorr_terms = fast_terms;

    nterms = strlen (decorr_terms);

    if (wpc->config.extra_flags & EXTRA_TERMS)
	if ((nterms += (wpc->config.extra_flags & EXTRA_TERMS) >> 10) > MAX_NTERMS)
	    nterms = MAX_NTERMS;

    for (i = 0; i < nterms + 2; ++i)
	sampleptrs [i] = malloc (wps->wphdr.block_samples * 8);

    memcpy (sampleptrs [nterms + 1], samples, wps->wphdr.block_samples * 8);
    best_bits = log2buffer (sampleptrs [nterms + 1], wps->wphdr.block_samples * 2);

    if ((wpc->config.extra_flags & EXTRA_STEREO_MODES) || !(wps->wphdr.flags & JOINT_STEREO)) {
	memcpy (sampleptrs [0], samples, wps->wphdr.block_samples * 8);

	CLEAR (dps);

	for (tp = decorr_terms, i = 0; *tp;) {
	    if (*tp > 0 || (wps->wphdr.flags & CROSS_DECORR))
		dps [i].term = *tp++;
	    else {
		dps [i].term = -3;
		tp++;
	    }

	    dps [i].delta = 2;
	    decorr_stereo_buffer (sampleptrs [i], sampleptrs [i+1], wps->wphdr.block_samples, &dps [i]);
	    ++i;
	}

#ifdef EXTRA_DUMP
	default_bits = bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);
#else
	bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);
#endif

	wps->wphdr.flags &= ~JOINT_STEREO;

	if (bits < best_bits) {
	    best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, dps, sizeof (dps [0]) * i);
	    memcpy (sampleptrs [nterms + 1], sampleptrs [i], wps->wphdr.block_samples * 8);
	}
    }

    if ((wpc->config.extra_flags & EXTRA_STEREO_MODES) || (wps->wphdr.flags & JOINT_STEREO)) {
	memcpy (sampleptrs [0], samples, wps->wphdr.block_samples * 8);
	cnt = wps->wphdr.block_samples;
	lptr = sampleptrs [0];

	while (cnt--) {
	    lptr [1] += ((lptr [0] -= lptr [1]) >> 1);
	    lptr += 2;
	}

	CLEAR (dps);

	for (tp = decorr_terms, i = 0; *tp;) {
	    if (*tp > 0 || (wps->wphdr.flags & CROSS_DECORR))
		dps [i].term = *tp++;
	    else {
		dps [i].term = -3;
		tp++;
	    }

	    dps [i].delta = 2;
	    decorr_stereo_buffer (sampleptrs [i], sampleptrs [i+1], wps->wphdr.block_samples, &dps [i]);
	    ++i;
	}

#ifdef EXTRA_DUMP
	default_bits = bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);
#else
	bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);
#endif

	wps->wphdr.flags |= JOINT_STEREO;

	if (bits < best_bits) {
	    best_bits = bits;
	    CLEAR (wps->decorr_passes);
	    memcpy (wps->decorr_passes, dps, sizeof (dps [0]) * i);
	    memcpy (sampleptrs [nterms + 1], sampleptrs [i], wps->wphdr.block_samples * 8);
	}
	else {
	    memcpy (sampleptrs [0], samples, wps->wphdr.block_samples * 8);
	    wps->wphdr.flags &= ~JOINT_STEREO;
	}
    }

    if ((wps->wphdr.flags & HYBRID_FLAG) && (wpc->config.extra_flags & EXTRA_ADVANCED)) {
	int shaping_weight, new = wps->wphdr.flags & NEW_SHAPING;
	int32_t *rptr = sampleptrs [nterms + 1], error [2], temp;

	scan_word (wps, rptr, wps->wphdr.block_samples, -1);
	cnt = wps->wphdr.block_samples;
	lptr = sampleptrs [0];
	CLEAR (error);

	if (wps->wphdr.flags & HYBRID_SHAPE) {
	    while (cnt--) {
		shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
		temp = -apply_weight (shaping_weight, error [0]);

		if (new && shaping_weight < 0 && temp) {
		    if (temp == error [0])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    lptr [0] += (error [0] = nosend_word (wps, rptr [0], 0) - rptr [0] + temp);
		}
		else
		    lptr [0] += (error [0] = nosend_word (wps, rptr [0], 0) - rptr [0]) + temp;

		shaping_weight = (wps->dc.shaping_acc [1] += wps->dc.shaping_delta [1]) >> 16;
		temp = -apply_weight (shaping_weight, error [1]);

		if (new && shaping_weight < 0 && temp) {
		    if (temp == error [1])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    lptr [1] += (error [1] = nosend_word (wps, rptr [1], 1) - rptr [1] + temp);
		}
		else
		    lptr [1] += (error [1] = nosend_word (wps, rptr [1], 1) - rptr [1]) + temp;

		lptr += 2;
		rptr += 2;
	    }

	    wps->dc.shaping_acc [0] -= wps->dc.shaping_delta [0] * wps->wphdr.block_samples;
	    wps->dc.shaping_acc [1] -= wps->dc.shaping_delta [1] * wps->wphdr.block_samples;
	}
	else
	    while (cnt--) {
		lptr [0] += nosend_word (wps, rptr [0], 0) - rptr [0];
		lptr [1] += nosend_word (wps, rptr [1], 1) - rptr [1];
		lptr += 2;
		rptr += 2;
	    }

	memcpy (dps, wps->decorr_passes, sizeof (dps));

	for (i = 0; i < nterms && dps [i].term; ++i)
	    decorr_stereo_buffer (sampleptrs [i], sampleptrs [i + 1], wps->wphdr.block_samples, dps + i);

#ifdef EXTRA_DUMP
	best_bits = default_bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);
#else
	best_bits = log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2);
#endif

	CLEAR (wps->decorr_passes);
	memcpy (wps->decorr_passes, dps, sizeof (dps [0]) * i);
	memcpy (sampleptrs [nterms + 1], sampleptrs [i], wps->wphdr.block_samples * 8);
    }

    if (wpc->config.extra_flags & EXTRA_BRANCHES)
	recurse_stereo (wpc, sampleptrs, dps, 0, nterms, (int) floor (wps->delta_decay + 0.5),
	    log2buffer (sampleptrs [0], wps->wphdr.block_samples * 2), &best_bits);

    if (wpc->config.extra_flags & EXTRA_SORT_FIRST)
	sort_stereo (wpc, sampleptrs, dps, nterms, &best_bits);

    if (wpc->config.extra_flags & EXTRA_TRY_DELTAS) {
	delta_stereo (wpc, sampleptrs, dps, nterms, &best_bits);

	if ((wpc->config.extra_flags & EXTRA_ADJUST_DELTAS) && wps->decorr_passes [0].term)
	    wps->delta_decay = (wps->delta_decay * 2.0 + wps->decorr_passes [0].delta) / 3.0;
	else
	    wps->delta_decay = 2.0;
    }

    if (wpc->config.extra_flags & EXTRA_SORT_LAST)
	sort_stereo (wpc, sampleptrs, dps, nterms, &best_bits);

#if 0
    memcpy (dps, wps->decorr_passes, sizeof (dps));

    for (i = 0; i < nterms && dps [i].term; ++i)
	decorr_stereo_pass (sampleptrs [i], sampleptrs [i + 1], wps->wphdr.block_samples, dps + i, 1);

    if (log2buffer (sampleptrs [i], wps->wphdr.block_samples * 2) != best_bits)
	error_line ("(1) samples do not match!");

    if (log2buffer (sampleptrs [nterms + 1], wps->wphdr.block_samples * 2) != best_bits)
	error_line ("(2) samples do not match!");
#endif

    scan_word (wps, sampleptrs [nterms + 1], wps->wphdr.block_samples, -1);

#ifdef EXTRA_DUMP
    if (1) {
	char string [256], substring [20];
	int i;

	sprintf (string, "%s: delta = %.4f%%, terms =",
	    (wps->wphdr.flags & JOINT_STEREO) ? "JS" : "TS",
		((double) best_bits - default_bits) / 256.0 / wps->wphdr.block_samples / 32.0 * 100.0);

	for (i = 0; i < nterms; ++i) {
	    if (wps->decorr_passes [i].term) {
		if (i && wps->decorr_passes [i-1].delta == wps->decorr_passes [i].delta)
		    sprintf (substring, " %d", wps->decorr_passes [i].term);
		else
		    sprintf (substring, " %d->%d", wps->decorr_passes [i].term,
			wps->decorr_passes [i].delta);
	    }
	    else
		sprintf (substring, " *");

	    strcat (string, substring);
	}

	error_line (string);
    }
#endif

    for (i = 0; i < nterms; ++i)
	if (!wps->decorr_passes [i].term)
	    break;

    wps->num_terms = i;

    for (i = 0; i < nterms + 2; ++i)
	free (sampleptrs [i]);

#ifdef MINMAX_WEIGHTS
    error_line ("weight range = %ld (%d) to %ld (%d)", min_weight, min_term, max_weight, max_term);
#endif
}
