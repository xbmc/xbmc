////////////////////////////////////////////////////////////////////////////
//			     **** WAVPACK ****				  //
//		    Hybrid Lossless Wavefile Compressor			  //
//		Copyright (c) 1998 - 2005 Conifer Software.		  //
//			    All Rights Reserved.			  //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// pack.c

// This module actually handles the compression of the audio data, except for
// the entropy coding which is handled by the words? modules. For efficiency,
// the conversion is isolated to tight loops that handle an entire buffer.

#include "wavpack.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// This flag provides faster encoding speed at the expense of more code. The
// improvement applies to 16-bit stereo lossless only.

#define FAST_ENCODE

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

//////////////////////////////// local tables ///////////////////////////////

// These two tables specify the characteristics of the decorrelation filters.
// Each term represents one layer of the sequential filter, where positive
// values indicate the relative sample involved from the same channel (1=prev),
// 17 & 18 are special functions using the previous 2 samples, and negative
// values indicate cross channel decorrelation (in stereo only).

const char default_terms [] = { 18,18,2,3,-2,0 };
const char high_terms [] = { 18,18,2,3,-2,18,2,4,7,5,3,6,8,-1,18,2,0 };
const char fast_terms [] = { 17,17,0 };

///////////////////////////// executable code ////////////////////////////////

// This function initializes everything required to pack WavPack bitstreams
// and must be called BEFORE any other function in this module.

void pack_init (WavpackContext *wpc)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    uint32_t flags = wps->wphdr.flags;
    struct decorr_pass *dpp;
    const char *term_string;
    int ti;

    wps->sample_index = 0;
    wps->delta_decay = 2.0;
    CLEAR (wps->decorr_passes);
    CLEAR (wps->dc);

    if (wpc->config.flags & CONFIG_AUTO_SHAPING)
	wps->dc.shaping_acc [0] = wps->dc.shaping_acc [1] =
	    (wpc->config.sample_rate < 64000 || (wps->wphdr.flags & CROSS_DECORR)) ? -512L << 16 : 1024L << 16;
    else {
	int32_t weight = (int32_t) floor (wpc->config.shaping_weight * 1024.0 + 0.5);

	if (weight <= -1000)
	    weight = -1000;

	wps->dc.shaping_acc [0] = wps->dc.shaping_acc [1] = weight << 16;
    }

    if (wpc->config.flags & CONFIG_HIGH_FLAG)
	term_string = high_terms;
    else if (wpc->config.flags & CONFIG_FAST_FLAG)
	term_string = fast_terms;
    else
	term_string = default_terms;

    for (dpp = wps->decorr_passes, ti = 0; ti < strlen (term_string); ti++)
	if (term_string [ti] >= 0 || (flags & CROSS_DECORR)) {
	    dpp->term = term_string [ti];
	    dpp++->delta = 2;
	}
	else if (!(flags & MONO_FLAG)) {
	    dpp->term = -3;
	    dpp++->delta = 2;
	}

    wps->num_terms = dpp - wps->decorr_passes;
    init_words (wps);
}

// Allocate room for and copy the decorrelation terms from the decorr_passes
// array into the specified metadata structure. Both the actual term id and
// the delta are packed into single characters.

void write_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int tcount = wps->num_terms;
    struct decorr_pass *dpp;
    char *byteptr;

    byteptr = wpmd->data = malloc (tcount + 1);
    wpmd->id = ID_DECORR_TERMS;

    for (dpp = wps->decorr_passes; tcount--; ++dpp)
	*byteptr++ = ((dpp->term + 5) & 0x1f) | ((dpp->delta << 5) & 0xe0);

    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Allocate room for and copy the decorrelation term weights from the
// decorr_passes array into the specified metadata structure. The weights
// range +/-1024, but are rounded and truncated to fit in signed chars for
// metadata storage. Weights are separate for the two channels

void write_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int tcount = wps->num_terms;
    struct decorr_pass *dpp;
    char *byteptr;

    byteptr = wpmd->data = malloc ((tcount * 2) + 1);
    wpmd->id = ID_DECORR_WEIGHTS;

    for (dpp = wps->decorr_passes; tcount--; ++dpp) {
	dpp->weight_A = restore_weight (*byteptr++ = store_weight (dpp->weight_A));

	if (!(wps->wphdr.flags & MONO_FLAG))
	    dpp->weight_B = restore_weight (*byteptr++ = store_weight (dpp->weight_B));
    }

    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Allocate room for and copy the decorrelation samples from the decorr_passes
// array into the specified metadata structure. The samples are signed 32-bit
// values, but are converted to signed log2 values for storage in metadata.
// Values are stored for both channels and are specified from the first term
// with unspecified samples set to zero. The number of samples stored varies
// with the actual term value, so those must obviously be specified before
// these in the metadata list. Any number of terms can have their samples
// specified from no terms to all the terms, however I have found that
// sending more than the first term's samples is a waste. The "wcount"
// variable can be set to the number of terms to have their samples stored.

void write_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int tcount = wps->num_terms, wcount = 1, temp;
    struct decorr_pass *dpp;
    uchar *byteptr;

    byteptr = wpmd->data = malloc (256);
    wpmd->id = ID_DECORR_SAMPLES;

    for (dpp = wps->decorr_passes; tcount--; ++dpp)
	if (wcount) {
	    if (dpp->term > MAX_TERM) {
		dpp->samples_A [0] = exp2s (temp = log2s (dpp->samples_A [0]));
		*byteptr++ = temp;
		*byteptr++ = temp >> 8;
		dpp->samples_A [1] = exp2s (temp = log2s (dpp->samples_A [1]));
		*byteptr++ = temp;
		*byteptr++ = temp >> 8;

		if (!(wps->wphdr.flags & MONO_FLAG)) {
		    dpp->samples_B [0] = exp2s (temp = log2s (dpp->samples_B [0]));
		    *byteptr++ = temp;
		    *byteptr++ = temp >> 8;
		    dpp->samples_B [1] = exp2s (temp = log2s (dpp->samples_B [1]));
		    *byteptr++ = temp;
		    *byteptr++ = temp >> 8;
		}
	    }
	    else if (dpp->term < 0) {
		dpp->samples_A [0] = exp2s (temp = log2s (dpp->samples_A [0]));
		*byteptr++ = temp;
		*byteptr++ = temp >> 8;
		dpp->samples_B [0] = exp2s (temp = log2s (dpp->samples_B [0]));
		*byteptr++ = temp;
		*byteptr++ = temp >> 8;
	    }
	    else {
		int m = 0, cnt = dpp->term;

		while (cnt--) {
		    dpp->samples_A [m] = exp2s (temp = log2s (dpp->samples_A [m]));
		    *byteptr++ = temp;
		    *byteptr++ = temp >> 8;

		    if (!(wps->wphdr.flags & MONO_FLAG)) {
			dpp->samples_B [m] = exp2s (temp = log2s (dpp->samples_B [m]));
			*byteptr++ = temp;
			*byteptr++ = temp >> 8;
		    }

		    m++;
		}
	    }

	    wcount--;
	}
	else {
	    CLEAR (dpp->samples_A);
	    CLEAR (dpp->samples_B);
	}

    wpmd->byte_length = byteptr - (uchar *) wpmd->data;
}

// Allocate room for and copy the noise shaping info into the specified
// metadata structure. These would normally be written to the
// "correction" file and are used for lossless reconstruction of
// hybrid data. The "delta" parameter is not yet used in encoding as it
// will be part of the "quality" mode.

void write_shaping_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    char *byteptr;
    int temp;

#if 0
    if (wps->wphdr.block_samples) {
	wps->dc.shaping_delta [0] = (-wps->dc.shaping_acc [0] - wps->dc.shaping_acc [0]) / (int32_t) wps->wphdr.block_samples;
	wps->dc.shaping_delta [1] = (-wps->dc.shaping_acc [1] - wps->dc.shaping_acc [1]) / (int32_t) wps->wphdr.block_samples;
    }
#endif

    byteptr = wpmd->data = malloc (12);
    wpmd->id = ID_SHAPING_WEIGHTS;

    wps->dc.error [0] = exp2s (temp = log2s (wps->dc.error [0]));
    *byteptr++ = temp;
    *byteptr++ = temp >> 8;
    wps->dc.shaping_acc [0] = exp2s (temp = log2s (wps->dc.shaping_acc [0]));
    *byteptr++ = temp;
    *byteptr++ = temp >> 8;

    if (!(wps->wphdr.flags & MONO_FLAG)) {
	wps->dc.error [1] = exp2s (temp = log2s (wps->dc.error [1]));
	*byteptr++ = temp;
	*byteptr++ = temp >> 8;
	wps->dc.shaping_acc [1] = exp2s (temp = log2s (wps->dc.shaping_acc [1]));
	*byteptr++ = temp;
	*byteptr++ = temp >> 8;
    }

    if (wps->dc.shaping_delta [0] | wps->dc.shaping_delta [1]) {
	wps->dc.shaping_delta [0] = exp2s (temp = log2s (wps->dc.shaping_delta [0]));
	*byteptr++ = temp;
	*byteptr++ = temp >> 8;

	if (!(wps->wphdr.flags & MONO_FLAG)) {
	    wps->dc.shaping_delta [1] = exp2s (temp = log2s (wps->dc.shaping_delta [1]));
	    *byteptr++ = temp;
	    *byteptr++ = temp >> 8;
	}
    }

    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Allocate room for and copy the int32 data values into the specified
// metadata structure. This data is used for integer data that has more
// than 24 bits of magnitude or, in some cases, it's used to eliminate
// redundant bits from any audio stream.

void write_int32_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    char *byteptr;

    byteptr = wpmd->data = malloc (4);
    wpmd->id = ID_INT32_INFO;
    *byteptr++ = wps->int32_sent_bits;
    *byteptr++ = wps->int32_zeros;
    *byteptr++ = wps->int32_ones;
    *byteptr++ = wps->int32_dups;
    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Allocate room for and copy the multichannel information into the specified
// metadata structure. The first byte is the total number of channels and the
// following bytes represent the channel_mask as described for Microsoft
// WAVEFORMATEX.

void write_channel_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    uint32_t mask = wpc->config.channel_mask;
    char *byteptr;

    byteptr = wpmd->data = malloc (4);
    wpmd->id = ID_CHANNEL_INFO;
    *byteptr++ = wpc->config.num_channels;

    while (mask) {
	*byteptr++ = mask;
	mask >>= 8;
    }

    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Allocate room for and copy the configuration information into the specified
// metadata structure. Currently, we just store the upper 3 bytes of
// config.flags and only in the first block of audio data. Note that this is
// for informational purposes not required for playback or decoding (like
// whether high or fast mode was specified).

void write_config_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    char *byteptr;

    byteptr = wpmd->data = malloc (4);
    wpmd->id = ID_CONFIG_BLOCK;
    *byteptr++ = (char) (wpc->config.flags >> 8);
    *byteptr++ = (char) (wpc->config.flags >> 16);
    *byteptr++ = (char) (wpc->config.flags >> 24);
    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Pack an entire block of samples (either mono or stereo) into a completed
// WavPack block. This function is actually a shell for pack_samples() and
// performs tasks like handling any shift required by the format, preprocessing
// of floating point data or integer data over 24 bits wide, and implementing
// the "extra" mode (via the extra?.c modules). It is assumed that there is
// sufficient space for the completed block at "wps->blockbuff" and that
// "wps->blockend" points to the end of the available space. A return value of
// FALSE indicates an error.

static int scan_int32_data (WavpackStream *wps, int32_t *values, int32_t num_values);
static void send_int32_data (WavpackStream *wps, int32_t *values, int32_t num_values);
static int pack_samples (WavpackContext *wpc, int32_t *buffer);

int pack_block (WavpackContext *wpc, int32_t *buffer)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    uint32_t flags = wps->wphdr.flags, sflags = wps->wphdr.flags;
    uint32_t sample_count = wps->wphdr.block_samples;
    int32_t *orig_data = NULL;

    if (flags & SHIFT_MASK) {
	int shift = (flags & SHIFT_MASK) >> SHIFT_LSB;
	int mag = (flags & MAG_MASK) >> MAG_LSB;
	uint32_t cnt = sample_count;
	int32_t *ptr = buffer;

	if (flags & MONO_FLAG)
	    while (cnt--)
		*ptr++ >>= shift;
	else
	    while (cnt--) {
		*ptr++ >>= shift;
		*ptr++ >>= shift;
	    }

	if ((mag -= shift) < 0)
	    flags &= ~MAG_MASK;
	else
	    flags -= (1 << MAG_LSB) * shift;

	wps->wphdr.flags = flags;
    }

    if ((flags & FLOAT_DATA) || (flags & MAG_MASK) >> MAG_LSB >= 24) {
	if ((!(flags & HYBRID_FLAG) || wpc->wvc_flag) && !(wpc->config.flags & CONFIG_SKIP_WVX)) {
	    orig_data = malloc (sizeof (f32) * ((flags & MONO_FLAG) ? sample_count : sample_count * 2));
	    memcpy (orig_data, buffer, sizeof (f32) * ((flags & MONO_FLAG) ? sample_count : sample_count * 2));

	    if (flags & FLOAT_DATA) {
		wps->float_norm_exp = wpc->config.float_norm_exp;

		if (!scan_float_data (wps, (f32 *) buffer, (flags & MONO_FLAG) ? sample_count : sample_count * 2)) {
		    free (orig_data);
		    orig_data = NULL;
		}
	    }
	    else {
		if (!scan_int32_data (wps, buffer, (flags & MONO_FLAG) ? sample_count : sample_count * 2)) {
		    free (orig_data);
		    orig_data = NULL;
		}
	    }
	}
	else {
	    if (flags & FLOAT_DATA) {
		wps->float_norm_exp = wpc->config.float_norm_exp;

		if (scan_float_data (wps, (f32 *) buffer, (flags & MONO_FLAG) ? sample_count : sample_count * 2))
		    wpc->lossy_blocks = TRUE;
	    }
	    else if (scan_int32_data (wps, buffer, (flags & MONO_FLAG) ? sample_count : sample_count * 2))
		wpc->lossy_blocks = TRUE;
	}

	wpc->config.extra_flags |= EXTRA_SCAN_ONLY;
    }
    else if (wpc->config.extra_flags)
	scan_int32_data (wps, buffer, (flags & MONO_FLAG) ? sample_count : sample_count * 2);

    if (wpc->config.extra_flags) {
	if (flags & MONO_FLAG)
	    analyze_mono (wpc, buffer);
	else
	    analyze_stereo (wpc, buffer);
    }
    else if (!wps->sample_index || !wps->num_terms) {
	wpc->config.extra_flags = EXTRA_SCAN_ONLY;

	if (flags & MONO_FLAG)
	    analyze_mono (wpc, buffer);
	else
	    analyze_stereo (wpc, buffer);

	wpc->config.extra_flags = 0;
    }

    if (!pack_samples (wpc, buffer)) {
	wps->wphdr.flags = sflags;

	if (orig_data)
	    free (orig_data);

	return FALSE;
    }
    else
	wps->wphdr.flags = sflags;

    if (orig_data) {
	uint32_t data_count;
	uchar *cptr;

	if (wpc->wvc_flag)
	    cptr = wps->block2buff + ((WavpackHeader *) wps->block2buff)->ckSize + 8;
	else
	    cptr = wps->blockbuff + ((WavpackHeader *) wps->blockbuff)->ckSize + 8;

	bs_open_write (&wps->wvxbits, cptr + 8, wpc->wvc_flag ? wps->block2end : wps->blockend);

	if (flags & FLOAT_DATA)
	    send_float_data (wps, (f32*) orig_data, (flags & MONO_FLAG) ? sample_count : sample_count * 2);
	else
	    send_int32_data (wps, orig_data, (flags & MONO_FLAG) ? sample_count : sample_count * 2);

	data_count = bs_close_write (&wps->wvxbits);
	free (orig_data);

	if (data_count) {
	    if (data_count != (uint32_t) -1) {
		*cptr++ = ID_WVX_BITSTREAM | ID_LARGE;
		*cptr++ = (data_count += 4) >> 1;
		*cptr++ = data_count >> 9;
		*cptr++ = data_count >> 17;
		*cptr++ = wps->crc_x;
		*cptr++ = wps->crc_x >> 8;
		*cptr++ = wps->crc_x >> 16;
		*cptr = wps->crc_x >> 24;

		if (wpc->wvc_flag)
		    ((WavpackHeader *) wps->block2buff)->ckSize += data_count + 4;
		else
		    ((WavpackHeader *) wps->blockbuff)->ckSize += data_count + 4;
	    }
	    else
		return FALSE;
	}
    }

    return TRUE;
}

// Scan a buffer of long integer data and determine whether any redundancy in
// the LSBs can be used to reduce the data's magnitude. If yes, then the
// INT32_DATA flag is set and the int32 parameters are set. If bits must still
// be transmitted literally to get down to 24 bits (which is all the integer
// compression code can handle) then we return TRUE to indicate that a wvx
// stream must be created in either lossless mode.

static int scan_int32_data (WavpackStream *wps, int32_t *values, int32_t num_values)
{
    uint32_t magdata = 0, ordata = 0, xordata = 0, anddata = ~0;
    uint32_t crc = 0xffffffff;
    int total_shift = 0;
    int32_t *dp, count;

    wps->int32_sent_bits = wps->int32_zeros = wps->int32_ones = wps->int32_dups = 0;

    for (dp = values, count = num_values; count--; dp++) {
	crc = crc * 9 + (*dp & 0xffff) * 3 + ((*dp >> 16) & 0xffff);
	magdata |= (*dp < 0) ? ~*dp : *dp;
	xordata |= *dp ^ -(*dp & 1);
	anddata &= *dp;
	ordata |= *dp;
    }

    wps->crc_x = crc;
    wps->wphdr.flags &= ~MAG_MASK;

    while (magdata) {
	wps->wphdr.flags += 1 << MAG_LSB;
	magdata >>= 1;
    }

    if (!((wps->wphdr.flags & MAG_MASK) >> MAG_LSB)) {
	wps->wphdr.flags &= ~INT32_DATA;
	return FALSE;
    }

    if (!(ordata & 1))
	while (!(ordata & 1)) {
	    wps->wphdr.flags -= 1 << MAG_LSB;
	    wps->int32_zeros++;
	    total_shift++;
	    ordata >>= 1;
	}
    else if (anddata & 1)
	while (anddata & 1) {
	    wps->wphdr.flags -= 1 << MAG_LSB;
	    wps->int32_ones++;
	    total_shift++;
	    anddata >>= 1;
	}
    else if (!(xordata & 2))
	while (!(xordata & 2)) {
	    wps->wphdr.flags -= 1 << MAG_LSB;
	    wps->int32_dups++;
	    total_shift++;
	    xordata >>= 1;
	}

    if (((wps->wphdr.flags & MAG_MASK) >> MAG_LSB) > 23) {
	wps->int32_sent_bits = ((wps->wphdr.flags & MAG_MASK) >> MAG_LSB) - 23;
	total_shift += wps->int32_sent_bits;
	wps->wphdr.flags &= ~MAG_MASK;
	wps->wphdr.flags += 23 << MAG_LSB;
    }

    if (total_shift) {
	wps->wphdr.flags |= INT32_DATA;

	for (dp = values, count = num_values; count--; dp++)
	    *dp >>= total_shift;
    }

#if 0
    if (wps->int32_sent_bits + wps->int32_zeros + wps->int32_ones + wps->int32_dups)
	error_line ("sent bits = %d, zeros/ones/dups = %d/%d/%d", wps->int32_sent_bits,
	    wps->int32_zeros, wps->int32_ones, wps->int32_dups);
#endif

    return wps->int32_sent_bits;
}

// For the specified buffer values and the int32 parameters stored in "wps",
// send the literal bits required to the "wvxbits" bitstream.

static void send_int32_data (WavpackStream *wps, int32_t *values, int32_t num_values)
{
    int sent_bits = wps->int32_sent_bits, pre_shift;
    int32_t mask = (1 << sent_bits) - 1;
    int32_t count, value, *dp;

    pre_shift = wps->int32_zeros + wps->int32_ones + wps->int32_dups;

    if (sent_bits)
	for (dp = values, count = num_values; count--; dp++) {
	    value = (*dp >> pre_shift) & mask;
	    putbits (value, sent_bits, &wps->wvxbits);
	}
}

// Pack an entire block of samples (either mono or stereo) into a completed
// WavPack block. It is assumed that there is sufficient space for the
// completed block at "wps->blockbuff" and that "wps->blockend" points to the
// end of the available space. A return value of FALSE indicates an error.
// Any unsent metadata is transmitted first, then required metadata for this
// block is sent, and finally the compressed integer data is sent. If a "wpx"
// stream is required for floating point data or large integer data, then this
// must be handled outside this function. To find out how much data was written
// the caller must look at the ckSize field of the written WavpackHeader, NOT
// the one in the WavpackStream.

static void decorr_stereo_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void decorr_stereo_pass_i (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void decorr_stereo_pass_id2 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);

static int pack_samples (WavpackContext *wpc, int32_t *buffer)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    uint32_t sample_count = wps->wphdr.block_samples;
    uint32_t flags = wps->wphdr.flags, data_count;
    int mag16 = ((flags & MAG_MASK) >> MAG_LSB) >= 16;
    int tcount, lossy = FALSE, m = 0;
    double noise_acc = 0.0, noise;
    struct decorr_pass *dpp;
    WavpackMetadata wpmd;
    uint32_t crc, crc2, i;
    int32_t *bptr;

    crc = crc2 = 0xffffffff;

    wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
    memcpy (wps->blockbuff, &wps->wphdr, sizeof (WavpackHeader));

    if (wpc->metacount) {
	WavpackMetadata *wpmdp = wpc->metadata;

	while (wpc->metacount) {
	    copy_metadata (wpmdp, wps->blockbuff, wps->blockend);
	    wpc->metabytes -= wpmdp->byte_length;
	    free_metadata (wpmdp++);
	    wpc->metacount--;
	}

	free (wpc->metadata);
	wpc->metadata = NULL;
    }

    if (!sample_count)
	return TRUE;

    write_decorr_terms (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    write_decorr_weights (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    write_decorr_samples (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    write_entropy_vars (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    if (flags & HYBRID_FLAG) {
	write_hybrid_profile (wps, &wpmd);
	copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
	free_metadata (&wpmd);
    }

    if (flags & FLOAT_DATA) {
	write_float_info (wps, &wpmd);
	copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
	free_metadata (&wpmd);
    }

    if (flags & INT32_DATA) {
	write_int32_info (wps, &wpmd);
	copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
	free_metadata (&wpmd);
    }

    if ((flags & INITIAL_BLOCK) &&
	(wpc->config.num_channels > 2 ||
	wpc->config.channel_mask != 0x5 - wpc->config.num_channels)) {
	    write_channel_info (wpc, &wpmd);
	    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
	    free_metadata (&wpmd);
    }

    if ((flags & INITIAL_BLOCK) && !wps->sample_index) {
	write_config_info (wpc, &wpmd);
	copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
	free_metadata (&wpmd);
    }

    bs_open_write (&wps->wvbits, wps->blockbuff + ((WavpackHeader *) wps->blockbuff)->ckSize + 12, wps->blockend);

    if (wpc->wvc_flag) {
	wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
	memcpy (wps->block2buff, &wps->wphdr, sizeof (WavpackHeader));

	if (flags & HYBRID_SHAPE) {
	    write_shaping_info (wps, &wpmd);
	    copy_metadata (&wpmd, wps->block2buff, wps->block2end);
	    free_metadata (&wpmd);
	}

	bs_open_write (&wps->wvcbits, wps->block2buff + ((WavpackHeader *) wps->block2buff)->ckSize + 12, wps->block2end);
    }

    /////////////////////// handle lossless mono mode /////////////////////////

    if (!(flags & HYBRID_FLAG) && (flags & MONO_FLAG))
	for (bptr = buffer, i = 0; i < sample_count; ++i) {
	    int32_t code;

	    crc = crc * 3 + (code = *bptr++);

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
		int32_t sam;

		if (dpp->term > MAX_TERM) {
		    if (dpp->term & 1)
			sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		    else
			sam = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;

		    dpp->samples_A [1] = dpp->samples_A [0];
		    dpp->samples_A [0] = code;
		}
		else {
		    sam = dpp->samples_A [m];
		    dpp->samples_A [(m + dpp->term) & (MAX_TERM - 1)] = code;
		}

		code -= apply_weight (dpp->weight_A, sam);
		update_weight (dpp->weight_A, dpp->delta, sam, code);
	    }

	    m = (m + 1) & (MAX_TERM - 1);
	    send_word_lossless (wps, code, 0);
	}

    //////////////////// handle the lossless stereo mode //////////////////////

#ifdef FAST_ENCODE
    else if (!(flags & HYBRID_FLAG) && !(flags & MONO_FLAG)) {
	int32_t *eptr = buffer + (sample_count * 2), sam_A, sam_B;

	if (flags & JOINT_STEREO)
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		crc = crc * 9 + bptr [0] * 3 + bptr [1];
		bptr [1] += ((bptr [0] -= bptr [1]) >> 1);
	    }
	else
	    for (bptr = buffer; bptr < eptr; bptr += 2)
		crc = crc * 9 + bptr [0] * 3 + bptr [1];

	for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount-- ; dpp++)
	    if (((flags & MAG_MASK) >> MAG_LSB) >= 16)
		decorr_stereo_pass (dpp, buffer, sample_count);
	    else if (dpp->delta != 2)
		decorr_stereo_pass_i (dpp, buffer, sample_count);
	    else
		decorr_stereo_pass_id2 (dpp, buffer, sample_count);

	for (bptr = buffer; bptr < eptr; bptr += 2) {
	    send_word_lossless (wps, bptr [0], 0);
	    send_word_lossless (wps, bptr [1], 1);
	}

	m = sample_count & (MAX_TERM - 1);
    }
#else
    else if (!(flags & HYBRID_FLAG) && !(flags & MONO_FLAG))
	for (bptr = buffer, i = 0; i < sample_count; ++i, bptr += 2) {
	    int32_t left, right, sam_A, sam_B;

	    crc = crc * 3 + (left = bptr [0]);
	    crc = crc * 3 + (right = bptr [1]);

	    if (flags & JOINT_STEREO)
		right += ((left -= right) >> 1);

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount-- ; dpp++) {
		if (dpp->term > 0) {
		    if (dpp->term > MAX_TERM) {
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
			dpp->samples_A [0] = left;
			dpp->samples_B [0] = right;
		    }
		    else {
			int k = (m + dpp->term) & (MAX_TERM - 1);

			sam_A = dpp->samples_A [m];
			sam_B = dpp->samples_B [m];
			dpp->samples_A [k] = left;
			dpp->samples_B [k] = right;
		    }

		    left -= apply_weight (dpp->weight_A, sam_A);
		    right -= apply_weight (dpp->weight_B, sam_B);
		    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
		    update_weight (dpp->weight_B, dpp->delta, sam_B, right);
		}
		else {
		    sam_A = (dpp->term == -2) ? right : dpp->samples_A [0];
		    sam_B = (dpp->term == -1) ? left : dpp->samples_B [0];
		    dpp->samples_A [0] = right;
		    dpp->samples_B [0] = left;
		    left -= apply_weight (dpp->weight_A, sam_A);
		    right -= apply_weight (dpp->weight_B, sam_B);
		    update_weight_clip (dpp->weight_A, dpp->delta, sam_A, left);
		    update_weight_clip (dpp->weight_B, dpp->delta, sam_B, right);
		}
	    }

	    m = (m + 1) & (MAX_TERM - 1);
	    send_word_lossless (wps, left, 0);
	    send_word_lossless (wps, right, 1);
	}
#endif

    /////////////////// handle the lossy/hybrid mono mode /////////////////////

    else if ((flags & HYBRID_FLAG) && (flags & MONO_FLAG))
	for (bptr = buffer, i = 0; i < sample_count; ++i) {
	    int32_t code, temp;

	    crc2 = crc2 * 3 + (code = *bptr++);

	    if (flags & HYBRID_SHAPE) {
		int shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
		temp = -apply_weight (shaping_weight, wps->dc.error [0]);

		if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
		    if (temp == wps->dc.error [0])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    wps->dc.error [0] = -code;
		    code += temp;
		}
		else
		    wps->dc.error [0] = -(code += temp);
	    }

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount-- ; dpp++)
		if (dpp->term > MAX_TERM) {
		    if (dpp->term & 1)
			dpp->samples_A [2] = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		    else
			dpp->samples_A [2] = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;

		    code -= (dpp->aweight_A = apply_weight (dpp->weight_A, dpp->samples_A [2]));
		}
		else
		    code -= (dpp->aweight_A = apply_weight (dpp->weight_A, dpp->samples_A [m]));

	    code = send_word (wps, code, 0);

	    while (--dpp >= wps->decorr_passes) {
		if (dpp->term > MAX_TERM) {
		    update_weight (dpp->weight_A, dpp->delta, dpp->samples_A [2], code);
		    dpp->samples_A [1] = dpp->samples_A [0];
		    dpp->samples_A [0] = (code += dpp->aweight_A);
		}
		else {
		    int32_t sam = dpp->samples_A [m];

		    update_weight (dpp->weight_A, dpp->delta, sam, code);
		    dpp->samples_A [(m + dpp->term) & (MAX_TERM - 1)] = (code += dpp->aweight_A);
		}
	    }

	    wps->dc.error [0] += code;
	    m = (m + 1) & (MAX_TERM - 1);

	    if ((crc = crc * 3 + code) != crc2)
		lossy = TRUE;

	    if (wpc->config.flags & CONFIG_CALC_NOISE) {
		noise = code - bptr [-1];

		noise_acc += noise *= noise;
		wps->dc.noise_ave = (wps->dc.noise_ave * 0.99) + (noise * 0.01);

		if (wps->dc.noise_ave > wps->dc.noise_max)
		    wps->dc.noise_max = wps->dc.noise_ave;
	    }
	}

    /////////////////// handle the lossy/hybrid stereo mode ///////////////////

    else if ((flags & HYBRID_FLAG) && !(flags & MONO_FLAG))
	for (bptr = buffer, i = 0; i < sample_count; ++i) {
	    int32_t left, right, temp;
	    int shaping_weight;

	    left = *bptr++;
	    crc2 = (crc2 * 3 + left) * 3 + (right = *bptr++);

	    if (flags & HYBRID_SHAPE) {
		shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
		temp = -apply_weight (shaping_weight, wps->dc.error [0]);

		if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
		    if (temp == wps->dc.error [0])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    wps->dc.error [0] = -left;
		    left += temp;
		}
		else
		    wps->dc.error [0] = -(left += temp);

		shaping_weight = (wps->dc.shaping_acc [1] += wps->dc.shaping_delta [1]) >> 16;
		temp = -apply_weight (shaping_weight, wps->dc.error [1]);

		if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
		    if (temp == wps->dc.error [1])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    wps->dc.error [1] = -right;
		    right += temp;
		}
		else
		    wps->dc.error [1] = -(right += temp);
	    }

	    if (flags & JOINT_STEREO)
		right += ((left -= right) >> 1);

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount-- ; dpp++)
		if (dpp->term > MAX_TERM) {
		    if (dpp->term & 1) {
			dpp->samples_A [2] = 2 * dpp->samples_A [0] - dpp->samples_A [1];
			dpp->samples_B [2] = 2 * dpp->samples_B [0] - dpp->samples_B [1];
		    }
		    else {
			dpp->samples_A [2] = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
			dpp->samples_B [2] = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
		    }

		    left -= (dpp->aweight_A = apply_weight (dpp->weight_A, dpp->samples_A [2]));
		    right -= (dpp->aweight_B = apply_weight (dpp->weight_B, dpp->samples_B [2]));
		}
		else if (dpp->term > 0) {
		    left -= (dpp->aweight_A = apply_weight (dpp->weight_A, dpp->samples_A [m]));
		    right -= (dpp->aweight_B = apply_weight (dpp->weight_B, dpp->samples_B [m]));
		}
		else {
		    if (dpp->term == -1)
			dpp->samples_B [0] = left;
		    else if (dpp->term == -2)
			dpp->samples_A [0] = right;

		    left -= (dpp->aweight_A = apply_weight (dpp->weight_A, dpp->samples_A [0]));
		    right -= (dpp->aweight_B = apply_weight (dpp->weight_B, dpp->samples_B [0]));
		}
#if 0
if (labs (left) > 60000000 || labs (right) > 60000000)
    error_line ("sending %d, %d; samples = %d, %d", left, right, bptr [-2], bptr [-1]);
#endif
	    left = send_word (wps, left, 0);
	    right = send_word (wps, right, 1);

	    while (--dpp >= wps->decorr_passes)
		if (dpp->term > MAX_TERM) {
		    update_weight (dpp->weight_A, dpp->delta, dpp->samples_A [2], left);
		    update_weight (dpp->weight_B, dpp->delta, dpp->samples_B [2], right);

		    dpp->samples_A [1] = dpp->samples_A [0];
		    dpp->samples_B [1] = dpp->samples_B [0];

		    dpp->samples_A [0] = (left += dpp->aweight_A);
		    dpp->samples_B [0] = (right += dpp->aweight_B);
		}
		else if (dpp->term > 0) {
		    int k = (m + dpp->term) & (MAX_TERM - 1);

		    update_weight (dpp->weight_A, dpp->delta, dpp->samples_A [m], left);
		    dpp->samples_A [k] = (left += dpp->aweight_A);

		    update_weight (dpp->weight_B, dpp->delta, dpp->samples_B [m], right);
		    dpp->samples_B [k] = (right += dpp->aweight_B);
		}
		else {
		    if (dpp->term == -1) {
			dpp->samples_B [0] = left + dpp->aweight_A;
			dpp->aweight_B = apply_weight (dpp->weight_B, dpp->samples_B [0]);
		    }
		    else if (dpp->term == -2) {
			dpp->samples_A [0] = right + dpp->aweight_B;
			dpp->aweight_A = apply_weight (dpp->weight_A, dpp->samples_A [0]);
		    }

		    update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], left);
		    update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], right);
		    dpp->samples_B [0] = (left += dpp->aweight_A);
		    dpp->samples_A [0] = (right += dpp->aweight_B);
		}

	    if (flags & JOINT_STEREO)
		left += (right -= (left >> 1));

	    wps->dc.error [0] += left;
	    wps->dc.error [1] += right;
	    m = (m + 1) & (MAX_TERM - 1);

	    if ((crc = (crc * 3 + left) * 3 + right) != crc2)
		lossy = TRUE;

	    if (wpc->config.flags & CONFIG_CALC_NOISE) {
		noise = (double)(left - bptr [-2]) * (left - bptr [-2]);
		noise += (double)(right - bptr [-1]) * (right - bptr [-1]);

		noise_acc += noise /= 2.0;
		wps->dc.noise_ave = (wps->dc.noise_ave * 0.99) + (noise * 0.01);

		if (wps->dc.noise_ave > wps->dc.noise_max)
		    wps->dc.noise_max = wps->dc.noise_ave;
	    }
	}

    if (m)
	for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
	    if (dpp->term > 0 && dpp->term <= MAX_TERM) {
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

    if (wpc->config.flags & CONFIG_CALC_NOISE)
	wps->dc.noise_sum += noise_acc;

    flush_word (wps);
    data_count = bs_close_write (&wps->wvbits);

    if (data_count) {
	if (data_count != (uint32_t) -1) {
	    uchar *cptr = wps->blockbuff + ((WavpackHeader *) wps->blockbuff)->ckSize + 8;

	    *cptr++ = ID_WV_BITSTREAM | ID_LARGE;
	    *cptr++ = data_count >> 1;
	    *cptr++ = data_count >> 9;
	    *cptr++ = data_count >> 17;
	    ((WavpackHeader *) wps->blockbuff)->ckSize += data_count + 4;
	}
	else
	    return FALSE;
    }

    ((WavpackHeader *) wps->blockbuff)->crc = crc;

    if (wpc->wvc_flag) {
	data_count = bs_close_write (&wps->wvcbits);

	if (data_count && lossy) {
	    if (data_count != (uint32_t) -1) {
		uchar *cptr = wps->block2buff + ((WavpackHeader *) wps->block2buff)->ckSize + 8;

		*cptr++ = ID_WVC_BITSTREAM | ID_LARGE;
		*cptr++ = data_count >> 1;
		*cptr++ = data_count >> 9;
		*cptr++ = data_count >> 17;
		((WavpackHeader *) wps->block2buff)->ckSize += data_count + 4;
	    }
	    else
		return FALSE;
	}

	((WavpackHeader *) wps->block2buff)->crc = crc2;
    }
    else if (lossy)
	wpc->lossy_blocks = TRUE;

    wps->sample_index += sample_count;
    return TRUE;
}

#ifdef FAST_ENCODE

static void decorr_stereo_pass_id2 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {
	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_A [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
		dpp->samples_B [1] = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [1];
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_A [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
		dpp->samples_B [1] = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [1];
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case 8:
	    for (m = 0, bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [m];
		dpp->samples_A [m] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = dpp->samples_B [m];
		dpp->samples_B [m] = bptr [1];
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		m = (m + 1) & (MAX_TERM - 1);
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		dpp->samples_A [k] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, dpp->samples_A [m]);
		update_weight_d2 (dpp->weight_A, dpp->delta, dpp->samples_A [m], bptr [0]);

		dpp->samples_B [k] = bptr [1];
		bptr [1] -= apply_weight_i (dpp->weight_B, dpp->samples_B [m]);
		update_weight_d2 (dpp->weight_B, dpp->delta, dpp->samples_B [m], bptr [1]);

		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [0];
		sam_B = bptr [0];
		dpp->samples_A [0] = bptr [1];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_clip_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_clip_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [1];
		sam_B = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_clip_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_clip_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [0];
		sam_B = dpp->samples_B [0];
		dpp->samples_A [0] = bptr [1];
		dpp->samples_B [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_clip_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_clip_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;
    }
}

static void decorr_stereo_pass_i (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {
	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_A [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
		dpp->samples_B [1] = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [1];
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_A [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
		dpp->samples_B [1] = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [1];
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [m];
		dpp->samples_A [k] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = dpp->samples_B [m];
		dpp->samples_B [k] = bptr [1];
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [0];
		sam_B = bptr [0];
		dpp->samples_A [0] = bptr [1];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [1];
		sam_B = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [0];
		sam_B = dpp->samples_B [0];
		dpp->samples_A [0] = bptr [1];
		dpp->samples_B [0] = bptr [0];
		bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;
    }
}

static void decorr_stereo_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {
	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_A [0] = bptr [0];
		bptr [0] -= apply_weight (dpp->weight_A, sam_A);
		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
		dpp->samples_B [1] = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [1];
		bptr [1] -= apply_weight (dpp->weight_B, sam_B);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_A [0] = bptr [0];
		bptr [0] -= apply_weight (dpp->weight_A, sam_A);
		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
		dpp->samples_B [1] = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [1];
		bptr [1] -= apply_weight (dpp->weight_B, sam_B);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [m];
		dpp->samples_A [k] = bptr [0];
		bptr [0] -= apply_weight (dpp->weight_A, sam_A);
		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);

		sam_B = dpp->samples_B [m];
		dpp->samples_B [k] = bptr [1];
		bptr [1] -= apply_weight (dpp->weight_B, sam_B);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [0];
		sam_B = bptr [0];
		dpp->samples_A [0] = bptr [1];
		bptr [0] -= apply_weight (dpp->weight_A, sam_A);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight (dpp->weight_B, sam_B);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [1];
		sam_B = dpp->samples_B [0];
		dpp->samples_B [0] = bptr [0];
		bptr [0] -= apply_weight (dpp->weight_A, sam_A);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight (dpp->weight_B, sam_B);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [0];
		sam_B = dpp->samples_B [0];
		dpp->samples_A [0] = bptr [1];
		dpp->samples_B [0] = bptr [0];
		bptr [0] -= apply_weight (dpp->weight_A, sam_A);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		bptr [1] -= apply_weight (dpp->weight_B, sam_B);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_B, bptr [1]);
	    }

	    break;
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function returns the accumulated RMS noise as a double if the       //
// CALC_NOISE bit was set in the WavPack header. The peak noise can also be //
// returned if desired. See wavpack.c for the calculations required to      //
// convert this into decibels of noise below full scale.                    //
//////////////////////////////////////////////////////////////////////////////

double pack_noise (WavpackContext *wpc, double *peak)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];

    if (peak)
	*peak = wps->dc.noise_max;

    return wps->dc.noise_sum;
}
