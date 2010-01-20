////////////////////////////////////////////////////////////////////////////
//			     **** WAVPACK ****				  //
//		    Hybrid Lossless Wavefile Compressor			  //
//		Copyright (c) 1998 - 2005 Conifer Software.		  //
//			    All Rights Reserved.			  //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// unpack.c

// This module actually handles the decompression of the audio data, except
// for the entropy decoding which is handled by the words? modules. For
// maximum efficiency, the conversion is isolated to tight loops that handle
// an entire buffer.

#include "wavpack.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// This flag provides faster decoding speed at the expense of more code. The
// improvement applies to 16-bit stereo lossless only.

#define FAST_DECODE

#define LOSSY_MUTE

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

///////////////////////////// executable code ////////////////////////////////

// This function initializes everything required to unpack a WavPack block
// and must be called before unpack_samples() is called to obtain audio data.
// It is assumed that the WavpackHeader has been read into the wps->wphdr
// (in the current WavpackStream) and that the entire block has been read at
// wps->blockbuff. If a correction file is available (wpc->wvc_flag = TRUE)
// then the corresponding correction block must be read into wps->block2buff
// and its WavpackHeader has overwritten the header at wps->wphdr. This is
// where all the metadata blocks are scanned including those that contain
// bitstream data.

int unpack_init (WavpackContext *wpc)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    uchar *blockptr, *block2ptr;
    WavpackMetadata wpmd;

    if (wps->wphdr.block_samples && wps->wphdr.block_index != (uint32_t) -1)
	wps->sample_index = wps->wphdr.block_index;

    wps->mute_error = FALSE;
    wps->crc = wps->crc_x = 0xffffffff;
    CLEAR (wps->wvbits);
    CLEAR (wps->wvcbits);
    CLEAR (wps->wvxbits);
    CLEAR (wps->decorr_passes);
    CLEAR (wps->dc);
    CLEAR (wps->w);

    blockptr = wps->blockbuff + sizeof (WavpackHeader);

    while (read_metadata_buff (&wpmd, wps->blockbuff, &blockptr))
	if (!process_metadata (wpc, &wpmd)) {
	    sprintf (wpc->error_message, "invalid metadata %2x!", wpmd.id);
	    return FALSE;
	}

    block2ptr = wps->block2buff + sizeof (WavpackHeader);

    while (wpc->wvc_flag && wps->wphdr.block_samples && read_metadata_buff (&wpmd, wps->block2buff, &block2ptr))
	if (!process_metadata (wpc, &wpmd)) {
	    sprintf (wpc->error_message, "invalid metadata %2x in wvc file!", wpmd.id);
	    return FALSE;
	}

    if (wps->wphdr.block_samples && !bs_is_open (&wps->wvbits)) {
	if (bs_is_open (&wps->wvcbits))
	    strcpy (wpc->error_message, "can't unpack correction files alone!");

	return FALSE;
    }

    if (wps->wphdr.block_samples && !bs_is_open (&wps->wvxbits)) {
	if ((wps->wphdr.flags & INT32_DATA) && wps->int32_sent_bits)
	    wpc->lossy_blocks = TRUE;

	if ((wps->wphdr.flags & FLOAT_DATA) &&
	    wps->float_flags & (FLOAT_EXCEPTIONS | FLOAT_ZEROS_SENT | FLOAT_SHIFT_SENT | FLOAT_SHIFT_SAME))
		wpc->lossy_blocks = TRUE;
    }

    return TRUE;
}

// This function initialzes the main bitstream for audio samples, which must
// be in the "wv" file.

int init_wv_bitstream (WavpackStream *wps, WavpackMetadata *wpmd)
{
    bs_open_read (&wps->wvbits, wpmd->data, (char *) wpmd->data + wpmd->byte_length);
    return TRUE;
}

// This function initialzes the "correction" bitstream for audio samples,
// which currently must be in the "wvc" file.

int init_wvc_bitstream (WavpackStream *wps, WavpackMetadata *wpmd)
{
    bs_open_read (&wps->wvcbits, wpmd->data, (char *) wpmd->data + wpmd->byte_length);
    return TRUE;
}

// This function initialzes the "extra" bitstream for audio samples which
// contains the information required to losslessly decompress 32-bit float data
// or integer data that exceeds 24 bits. This bitstream is in the "wv" file
// for pure lossless data or the "wvc" file for hybrid lossless. This data
// would not be used for hybrid lossy mode. There is also a 32-bit CRC stored
// in the first 4 bytes of these blocks.

int init_wvx_bitstream (WavpackStream *wps, WavpackMetadata *wpmd)
{
    uchar *cp = wpmd->data;

    wps->crc_wvx = *cp++;
    wps->crc_wvx |= (int32_t) *cp++ << 8;
    wps->crc_wvx |= (int32_t) *cp++ << 16;
    wps->crc_wvx |= (int32_t) *cp++ << 24;

    bs_open_read (&wps->wvxbits, cp, (char *) wpmd->data + wpmd->byte_length);
    return TRUE;
}

// Read decorrelation terms from specified metadata block into the
// decorr_passes array. The terms range from -3 to 8, plus 17 & 18;
// other values are reserved and generate errors for now. The delta
// ranges from 0 to 7 with all values valid. Note that the terms are
// stored in the opposite order in the decorr_passes array compared
// to packing.

int read_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int termcnt = wpmd->byte_length;
    uchar *byteptr = wpmd->data;
    struct decorr_pass *dpp;

    if (termcnt > MAX_NTERMS)
	return FALSE;

    wps->num_terms = termcnt;

    for (dpp = wps->decorr_passes + termcnt - 1; termcnt--; dpp--) {
	dpp->term = (int)(*byteptr & 0x1f) - 5;
	dpp->delta = (*byteptr++ >> 5) & 0x7;

	if (!dpp->term || dpp->term < -3 || (dpp->term > MAX_TERM && dpp->term < 17) || dpp->term > 18)
	    return FALSE;
    }

    return TRUE;
}

// Read decorrelation weights from specified metadata block into the
// decorr_passes array. The weights range +/-1024, but are rounded and
// truncated to fit in signed chars for metadata storage. Weights are
// separate for the two channels and are specified from the "last" term
// (first during encode). Unspecified weights are set to zero.

int read_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int termcnt = wpmd->byte_length, tcount;
    char *byteptr = wpmd->data;
    struct decorr_pass *dpp;

    if (!(wps->wphdr.flags & MONO_FLAG))
	termcnt /= 2;

    if (termcnt > wps->num_terms)
	return FALSE;

    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
	dpp->weight_A = dpp->weight_B = 0;

    while (--dpp >= wps->decorr_passes && termcnt--) {
	dpp->weight_A = restore_weight (*byteptr++);

	if (!(wps->wphdr.flags & MONO_FLAG))
	    dpp->weight_B = restore_weight (*byteptr++);
    }

    return TRUE;
}

// Read decorrelation samples from specified metadata block into the
// decorr_passes array. The samples are signed 32-bit values, but are
// converted to signed log2 values for storage in metadata. Values are
// stored for both channels and are specified from the "last" term
// (first during encode) with unspecified samples set to zero. The
// number of samples stored varies with the actual term value, so
// those must obviously come first in the metadata.

int read_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd)
{
    uchar *byteptr = wpmd->data;
    uchar *endptr = byteptr + wpmd->byte_length;
    struct decorr_pass *dpp;
    int tcount;

    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
	CLEAR (dpp->samples_A);
	CLEAR (dpp->samples_B);
    }

    if (wps->wphdr.version == 0x402 && (wps->wphdr.flags & HYBRID_FLAG)) {
	wps->dc.error [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	byteptr += 2;

	if (!(wps->wphdr.flags & MONO_FLAG)) {
	    wps->dc.error [1] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	    byteptr += 2;
	}
    }

    while (dpp-- > wps->decorr_passes && byteptr < endptr)
	if (dpp->term > MAX_TERM) {
	    dpp->samples_A [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	    dpp->samples_A [1] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
	    byteptr += 4;

	    if (!(wps->wphdr.flags & MONO_FLAG)) {
		dpp->samples_B [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
		dpp->samples_B [1] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
		byteptr += 4;
	    }
	}
	else if (dpp->term < 0) {
	    dpp->samples_A [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	    dpp->samples_B [0] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
	    byteptr += 4;
	}
	else {
	    int m = 0, cnt = dpp->term;

	    while (cnt--) {
		dpp->samples_A [m] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
		byteptr += 2;

		if (!(wps->wphdr.flags & MONO_FLAG)) {
		    dpp->samples_B [m] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
		    byteptr += 2;
		}

		m++;
	    }
	}

    return byteptr == endptr;
}

// Read the shaping weights from specified metadata block into the
// WavpackStream structure. Note that there must be two values (even
// for mono streams) and that the values are stored in the same
// manner as decorrelation weights. These would normally be read from
// the "correction" file and are used for lossless reconstruction of
// hybrid data.

int read_shaping_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    if (wpmd->byte_length == 2) {
	char *byteptr = wpmd->data;

	wps->dc.shaping_acc [0] = (int32_t) restore_weight (*byteptr++) << 16;
	wps->dc.shaping_acc [1] = (int32_t) restore_weight (*byteptr++) << 16;
	return TRUE;
    }
    else if (wpmd->byte_length >= (wps->wphdr.flags & MONO_FLAG ? 4 : 8)) {
	uchar *byteptr = wpmd->data;

	wps->dc.error [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	wps->dc.shaping_acc [0] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
	byteptr += 4;

	if (!(wps->wphdr.flags & MONO_FLAG)) {
	    wps->dc.error [1] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	    wps->dc.shaping_acc [1] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
	    byteptr += 4;
	}

	if (wpmd->byte_length == (wps->wphdr.flags & MONO_FLAG ? 6 : 12)) {
	    wps->dc.shaping_delta [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));

	    if (!(wps->wphdr.flags & MONO_FLAG))
		wps->dc.shaping_delta [1] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
	}

	return TRUE;
    }

    return FALSE;
}

// Read the int32 data from the specified metadata into the specified stream.
// This data is used for integer data that has more than 24 bits of magnitude
// or, in some cases, used to eliminate redundant bits from any audio stream.

int read_int32_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    char *byteptr = wpmd->data;

    if (bytecnt != 4)
	return FALSE;

    wps->int32_sent_bits = *byteptr++;
    wps->int32_zeros = *byteptr++;
    wps->int32_ones = *byteptr++;
    wps->int32_dups = *byteptr;

    return TRUE;
}

// Read multichannel information from metadata. The first byte is the total
// number of channels and the following bytes represent the channel_mask
// as described for Microsoft WAVEFORMATEX.

int read_channel_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length, shift = 0;
    char *byteptr = wpmd->data;
    uint32_t mask = 0;

    if (!bytecnt || bytecnt > 5)
	return FALSE;

    wpc->config.num_channels = *byteptr++;

    while (--bytecnt) {
	mask |= (uint32_t) *byteptr++ << shift;
	shift += 8;
    }

    wpc->config.channel_mask = mask;
    return TRUE;
}

// Read configuration information from metadata.

int read_config_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    uchar *byteptr = wpmd->data;

    if (bytecnt >= 3) {
	wpc->config.flags &= 0xff;
	wpc->config.flags |= (int32_t) *byteptr++ << 8;
	wpc->config.flags |= (int32_t) *byteptr++ << 16;
	wpc->config.flags |= (int32_t) *byteptr << 24;
    }

    return TRUE;
}

// Read wrapper data from metadata. Currently, this consists of the RIFF
// header and trailer that wav files contain around the audio data but could
// be used for other formats as well. Because WavPack files contain all the
// information required for decoding and playback, this data can probably
// be ignored except when an exact wavefile restoration is needed.

int read_wrapper_data (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    if (wpc->open_flags & OPEN_WRAPPER) {
	wpc->wrapper_data = realloc (wpc->wrapper_data, wpc->wrapper_bytes + wpmd->byte_length);
	memcpy (wpc->wrapper_data + wpc->wrapper_bytes, wpmd->data, wpmd->byte_length);
	wpc->wrapper_bytes += wpmd->byte_length;
    }

    return TRUE;
}

#ifdef UNPACK

// This monster actually unpacks the WavPack bitstream(s) into the specified
// buffer as 32-bit integers or floats (depending on orignal data). Lossy
// samples will be clipped to their original limits (i.e. 8-bit samples are
// clipped to -128/+127) but are still returned in longs. It is up to the
// caller to potentially reformat this for the final output including any
// multichannel distribution, block alignment or endian compensation. The
// function unpack_init() must have been called and the entire WavPack block
// must still be visible (although wps->blockbuff will not be accessed again).
// For maximum clarity, the function is broken up into segments that handle
// various modes. This makes for a few extra infrequent flag checks, but
// makes the code easier to follow because the nesting does not become so
// deep. For maximum efficiency, the conversion is isolated to tight loops
// that handle an entire buffer. The function returns the total number of
// samples unpacked, which can be less than the number requested if an error
// occurs or the end of the block is reached.

static void decorr_stereo_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void decorr_stereo_pass_i (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void decorr_stereo_pass_id0 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void decorr_stereo_pass_id1 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void decorr_stereo_pass_id2 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);

static void fixup_samples (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count);

int32_t unpack_samples (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    uint32_t flags = wps->wphdr.flags, crc = wps->crc, i;
    int32_t mute_limit = (1L << ((flags & MAG_MASK) >> MAG_LSB)) + 2;
    int32_t correction [2], read_word, *bptr;
    struct decorr_pass *dpp;
    int tcount, m = 0;

    if (wps->sample_index + sample_count > wps->wphdr.block_index + wps->wphdr.block_samples)
	sample_count = wps->wphdr.block_index + wps->wphdr.block_samples - wps->sample_index;

    if (wps->mute_error) {
	memset (buffer, 0, sample_count * (flags & MONO_FLAG ? 4 : 8));
	wps->sample_index += sample_count;
	return sample_count;
    }

    if ((flags & HYBRID_FLAG) && !wpc->wvc_flag)
	mute_limit *= 2;

    ///////////////// handle version 4 lossless mono data /////////////////////

    if (!(flags & HYBRID_FLAG) && (flags & MONO_FLAG))
	for (bptr = buffer, i = 0; i < sample_count; ++i) {
	    if ((read_word = get_word_lossless (wps, 0)) == WORD_EOF)
		break;

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
		int32_t sam, temp;
		int k;

		if (dpp->term > MAX_TERM) {
		    if (dpp->term & 1)
			sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		    else
			sam = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;

		    dpp->samples_A [1] = dpp->samples_A [0];
		    k = 0;
		}
		else {
		    sam = dpp->samples_A [m];
		    k = (m + dpp->term) & (MAX_TERM - 1);
		}

		temp = apply_weight (dpp->weight_A, sam) + read_word;
		update_weight (dpp->weight_A, dpp->delta, sam, read_word);
		dpp->samples_A [k] = read_word = temp;
	    }

	    if (labs (read_word) > mute_limit)
		break;

	    m = (m + 1) & (MAX_TERM - 1);
	    crc = crc * 3 + read_word;
	    *bptr++ = read_word;
	}

    //////////////// handle version 4 lossless stereo data ////////////////////

    else if (!wpc->wvc_flag && !(flags & MONO_FLAG)) {
	int32_t *eptr = buffer + (sample_count * 2);

	i = sample_count;

	if (flags & HYBRID_FLAG) {
	    for (bptr = buffer; bptr < eptr; bptr += 2)
		if ((bptr [0] = get_word (wps, 0, NULL)) == WORD_EOF ||
		    (bptr [1] = get_word (wps, 1, NULL)) == WORD_EOF) {
			i = (bptr - buffer) / 2;
			break;
		}
	}
	else
	    for (bptr = buffer; bptr < eptr; bptr += 2)
		if ((bptr [0] = get_word_lossless (wps, 0)) == WORD_EOF ||
		    (bptr [1] = get_word_lossless (wps, 1)) == WORD_EOF) {
			i = (bptr - buffer) / 2;
			break;
		}

#ifdef FAST_DECODE
	for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
	    if (((flags & MAG_MASK) >> MAG_LSB) >= 16)
		decorr_stereo_pass (dpp, buffer, sample_count);
	    else if (dpp->delta > 2)
		decorr_stereo_pass_i (dpp, buffer, sample_count);
	    else if (dpp->delta == 2)
		decorr_stereo_pass_id2 (dpp, buffer, sample_count);
	    else if (dpp->delta == 1)
		decorr_stereo_pass_id1 (dpp, buffer, sample_count);
	    else
		decorr_stereo_pass_id0 (dpp, buffer, sample_count);
#else
	for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
	    decorr_stereo_pass (dpp, buffer, sample_count);
#endif

	if (flags & JOINT_STEREO)
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		bptr [0] += (bptr [1] -= (bptr [0] >> 1));

		if (labs (bptr [0]) > mute_limit || labs (bptr [1]) > mute_limit) {
		    i = (bptr - buffer) / 2;
		    break;
		}

		crc = (crc * 3 + bptr [0]) * 3 + bptr [1];
	    }
	else
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		if (labs (bptr [0]) > mute_limit || labs (bptr [1]) > mute_limit) {
		    i = (bptr - buffer) / 2;
		    break;
		}

		crc = (crc * 3 + bptr [0]) * 3 + bptr [1];
	    }

	m = sample_count & (MAX_TERM - 1);
    }

    //////////////// handle version 4 lossy/hybrid mono data //////////////////

    else if ((flags & HYBRID_FLAG) && (flags & MONO_FLAG))
	for (bptr = buffer, i = 0; i < sample_count; ++i) {

	    if ((read_word = get_word (wps, 0, correction)) == WORD_EOF)
		break;

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
		int32_t sam, temp;
		int k;

		if (dpp->term > MAX_TERM) {
		    if (dpp->term & 1)
			sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		    else
			sam = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;

		    dpp->samples_A [1] = dpp->samples_A [0];
		    k = 0;
		}
		else {
		    sam = dpp->samples_A [m];
		    k = (m + dpp->term) & (MAX_TERM - 1);
		}

		temp = apply_weight (dpp->weight_A, sam) + read_word;
		update_weight (dpp->weight_A, dpp->delta, sam, read_word);
		dpp->samples_A [k] = read_word = temp;
	    }

	    m = (m + 1) & (MAX_TERM - 1);

	    if (wpc->wvc_flag) {
		if (flags & HYBRID_SHAPE) {
		    int shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
		    int32_t temp = -apply_weight (shaping_weight, wps->dc.error [0]);

		    if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
			if (temp == wps->dc.error [0])
			    temp = (temp < 0) ? temp + 1 : temp - 1;

			wps->dc.error [0] = temp - correction [0];
		    }
		    else
			wps->dc.error [0] = -correction [0];

		    read_word += correction [0] - temp;
		}
		else
		    read_word += correction [0];
	    }

	    crc = crc * 3 + read_word;

#ifdef LOSSY_MUTE
	    if (labs (read_word) > mute_limit)
		break;
#endif
	    *bptr++ = read_word;
	}

    //////////////// handle version 4 lossy/hybrid stereo data ////////////////

    else if (wpc->wvc_flag && !(flags & MONO_FLAG))
	for (bptr = buffer, i = 0; i < sample_count; ++i) {
	    int32_t left, right, left_c, right_c, left2, right2;

	    if ((left = get_word (wps, 0, correction)) == WORD_EOF ||
		(right = get_word (wps, 1, correction + 1)) == WORD_EOF)
		    break;

	    if (flags & CROSS_DECORR) {
		left_c = left + correction [0];
		right_c = right + correction [1];

		for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
		    int32_t sam_A, sam_B;

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
			}
			else {
			    sam_A = dpp->samples_A [m];
			    sam_B = dpp->samples_B [m];
			}

			left_c += apply_weight (dpp->weight_A, sam_A);
			right_c += apply_weight (dpp->weight_B, sam_B);
		    }
		    else if (dpp->term == -1) {
			left_c += apply_weight (dpp->weight_A, dpp->samples_A [0]);
			right_c += apply_weight (dpp->weight_B, left_c);
		    }
		    else {
			right_c += apply_weight (dpp->weight_B, dpp->samples_B [0]);

			if (dpp->term == -3)
			    left_c += apply_weight (dpp->weight_A, dpp->samples_A [0]);
			else
			    left_c += apply_weight (dpp->weight_A, right_c);
		    }
		}

		if (flags & JOINT_STEREO)
		    left_c += (right_c -= (left_c >> 1));
	    }

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
		int32_t sam_A, sam_B;

		if (dpp->term > 0) {
		    int k;

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
			k = 0;
		    }
		    else {
			sam_A = dpp->samples_A [m];
			sam_B = dpp->samples_B [m];
			k = (m + dpp->term) & (MAX_TERM - 1);
		    }

		    left2 = apply_weight (dpp->weight_A, sam_A) + left;
		    right2 = apply_weight (dpp->weight_B, sam_B) + right;

		    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
		    update_weight (dpp->weight_B, dpp->delta, sam_B, right);

		    dpp->samples_A [k] = left = left2;
		    dpp->samples_B [k] = right = right2;
		}
		else if (dpp->term == -1) {
		    left2 = left + apply_weight (dpp->weight_A, dpp->samples_A [0]);
		    update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], left);
		    left = left2;
		    right2 = right + apply_weight (dpp->weight_B, left2);
		    update_weight_clip (dpp->weight_B, dpp->delta, left2, right);
		    dpp->samples_A [0] = right = right2;
		}
		else {
		    right2 = right + apply_weight (dpp->weight_B, dpp->samples_B [0]);
		    update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], right);
		    right = right2;

		    if (dpp->term == -3) {
			right2 = dpp->samples_A [0];
			dpp->samples_A [0] = right;
		    }

		    left2 = left + apply_weight (dpp->weight_A, right2);
		    update_weight_clip (dpp->weight_A, dpp->delta, right2, left);
		    dpp->samples_B [0] = left = left2;
		}
	    }

	    m = (m + 1) & (MAX_TERM - 1);

	    if (!(flags & CROSS_DECORR)) {
		left_c = left + correction [0];
		right_c = right + correction [1];

		if (flags & JOINT_STEREO)
		    left_c += (right_c -= (left_c >> 1));
	    }

	    if (flags & JOINT_STEREO)
		left += (right -= (left >> 1));

	    if (flags & HYBRID_SHAPE) {
		int shaping_weight;
		int32_t temp;
	
		correction [0] = left_c - left;
		shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
		temp = -apply_weight (shaping_weight, wps->dc.error [0]);

		if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
		    if (temp == wps->dc.error [0])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    wps->dc.error [0] = temp - correction [0];
		}
		else
		    wps->dc.error [0] = -correction [0];

		left = left_c - temp;
		correction [1] = right_c - right;
		shaping_weight = (wps->dc.shaping_acc [1] += wps->dc.shaping_delta [1]) >> 16;
		temp = -apply_weight (shaping_weight, wps->dc.error [1]);

		if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
		    if (temp == wps->dc.error [1])
			temp = (temp < 0) ? temp + 1 : temp - 1;

		    wps->dc.error [1] = temp - correction [1];
		}
		else
		    wps->dc.error [1] = -correction [1];

		right = right_c - temp;
	    }
	    else {
		left = left_c;
		right = right_c;
	    }

#ifdef LOSSY_MUTE
	    if (labs (left) > mute_limit || labs (right) > mute_limit)
		break;
#endif
	    crc = (crc * 3 + left) * 3 + right;
	    *bptr++ = left;
	    *bptr++ = right;
	}

    if (i != sample_count) {
	memset (buffer, 0, sample_count * (flags & MONO_FLAG ? 4 : 8));
	wps->mute_error = TRUE;
	i = sample_count;

	if (bs_is_open (&wps->wvxbits))
	    bs_close_read (&wps->wvxbits);
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

    fixup_samples (wpc, buffer, i);

    if ((flags & FLOAT_DATA) && (wpc->open_flags & OPEN_NORMALIZE))
	float_normalize (buffer, (flags & MONO_FLAG) ? i : i * 2,
	    127 - wps->float_norm_exp + wpc->norm_offset);

    wps->sample_index += i;
    wps->crc = crc;

    return i;
}

static void decorr_stereo_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {

	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight (dpp->weight_B, sam_B) + bptr [1];

		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight (dpp->weight_B, sam_B) + bptr [1];

		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [m];
		sam_B = dpp->samples_B [m];

		dpp->samples_A [k] = apply_weight (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [k] = apply_weight (dpp->weight_B, sam_B) + bptr [1];

		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [k];
		bptr [1] = dpp->samples_B [k];

		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		bptr [0] = sam_A;
		dpp->samples_A [0] = bptr [1] + apply_weight (dpp->weight_B, sam_A);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_A, bptr [1]);
		bptr [1] = dpp->samples_A [0];
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_B = bptr [1] + apply_weight (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [1] = sam_B;
		dpp->samples_B [0] = bptr [0] + apply_weight (dpp->weight_A, sam_B);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_B, bptr [0]);
		bptr [0] = dpp->samples_B [0];
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		sam_B = bptr [1] + apply_weight (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [0] = dpp->samples_B [0] = sam_A;
		bptr [1] = dpp->samples_A [0] = sam_B;
	    }

	    break;
    }
}

#ifdef FAST_DECODE

static void decorr_stereo_pass_i (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {

	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [m];
		sam_B = dpp->samples_B [m];

		dpp->samples_A [k] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [k] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [k];
		bptr [1] = dpp->samples_B [k];

		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		bptr [0] = sam_A;
		dpp->samples_A [0] = bptr [1] + apply_weight_i (dpp->weight_B, sam_A);
		update_weight_clip (dpp->weight_B, dpp->delta, sam_A, bptr [1]);
		bptr [1] = dpp->samples_A [0];
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_B = bptr [1] + apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [1] = sam_B;
		dpp->samples_B [0] = bptr [0] + apply_weight_i (dpp->weight_A, sam_B);
		update_weight_clip (dpp->weight_A, dpp->delta, sam_B, bptr [0]);
		bptr [0] = dpp->samples_B [0];
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		sam_B = bptr [1] + apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [0] = dpp->samples_B [0] = sam_A;
		bptr [1] = dpp->samples_A [0] = sam_B;
	    }

	    break;
    }
}

static void decorr_stereo_pass_id2 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {

	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [m];
		sam_B = dpp->samples_B [m];

		dpp->samples_A [k] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [k] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight_d2 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight_d2 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [k];
		bptr [1] = dpp->samples_B [k];

		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip_d2 (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		bptr [0] = sam_A;
		dpp->samples_A [0] = bptr [1] + apply_weight_i (dpp->weight_B, sam_A);
		update_weight_clip_d2 (dpp->weight_B, dpp->delta, sam_A, bptr [1]);
		bptr [1] = dpp->samples_A [0];
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_B = bptr [1] + apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip_d2 (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [1] = sam_B;
		dpp->samples_B [0] = bptr [0] + apply_weight_i (dpp->weight_A, sam_B);
		update_weight_clip_d2 (dpp->weight_A, dpp->delta, sam_B, bptr [0]);
		bptr [0] = dpp->samples_B [0];
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip_d2 (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		sam_B = bptr [1] + apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip_d2 (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [0] = dpp->samples_B [0] = sam_A;
		bptr [1] = dpp->samples_A [0] = sam_B;
	    }

	    break;
    }
}

static void decorr_stereo_pass_id1 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {

	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight_d1 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight_d1 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [0] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight_d1 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight_d1 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [0];
		bptr [1] = dpp->samples_B [0];
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = dpp->samples_A [m];
		sam_B = dpp->samples_B [m];

		dpp->samples_A [k] = apply_weight_i (dpp->weight_A, sam_A) + bptr [0];
		dpp->samples_B [k] = apply_weight_i (dpp->weight_B, sam_B) + bptr [1];

		update_weight_d1 (dpp->weight_A, dpp->delta, sam_A, bptr [0]);
		update_weight_d1 (dpp->weight_B, dpp->delta, sam_B, bptr [1]);

		bptr [0] = dpp->samples_A [k];
		bptr [1] = dpp->samples_B [k];

		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip_d1 (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		bptr [0] = sam_A;
		dpp->samples_A [0] = bptr [1] + apply_weight_i (dpp->weight_B, sam_A);
		update_weight_clip_d1 (dpp->weight_B, dpp->delta, sam_A, bptr [1]);
		bptr [1] = dpp->samples_A [0];
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_B = bptr [1] + apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip_d1 (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [1] = sam_B;
		dpp->samples_B [0] = bptr [0] + apply_weight_i (dpp->weight_A, sam_B);
		update_weight_clip_d1 (dpp->weight_A, dpp->delta, sam_B, bptr [0]);
		bptr [0] = dpp->samples_B [0];
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = bptr [0] + apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		update_weight_clip_d1 (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
		sam_B = bptr [1] + apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		update_weight_clip_d1 (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
		bptr [0] = dpp->samples_B [0] = sam_A;
		bptr [1] = dpp->samples_A [0] = sam_B;
	    }

	    break;
    }
}

static void decorr_stereo_pass_id0 (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2), sam_A, sam_B;
    int m, k;

    switch (dpp->term) {

	case 17:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = bptr [0] += apply_weight_i (dpp->weight_A, sam_A);
		dpp->samples_B [0] = bptr [1] += apply_weight_i (dpp->weight_B, sam_B);
	    }

	    break;

	case 18:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
		sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;

		dpp->samples_A [1] = dpp->samples_A [0];
		dpp->samples_B [1] = dpp->samples_B [0];

		dpp->samples_A [0] = bptr [0] += apply_weight_i (dpp->weight_A, sam_A);
		dpp->samples_B [0] = bptr [1] += apply_weight_i (dpp->weight_B, sam_B);
	    }

	    break;

	default:
	    for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
		dpp->samples_A [k] = bptr [0] += apply_weight_i (dpp->weight_A, dpp->samples_A [m]);
		dpp->samples_B [k] = bptr [1] += apply_weight_i (dpp->weight_B, dpp->samples_B [m]);
		m = (m + 1) & (MAX_TERM - 1);
		k = (k + 1) & (MAX_TERM - 1);
	    }

	    break;

	case -1:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		bptr [0] += apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		dpp->samples_A [0] = bptr [1] += apply_weight_i (dpp->weight_B, bptr [0]);
	    }

	    break;

	case -2:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		bptr [1] += apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		dpp->samples_B [0] = bptr [0] += apply_weight_i (dpp->weight_A, bptr [1]);
	    }

	    break;

	case -3:
	    for (bptr = buffer; bptr < eptr; bptr += 2) {
		bptr [0] += apply_weight_i (dpp->weight_A, dpp->samples_A [0]);
		dpp->samples_A [0] = bptr [1] += apply_weight_i (dpp->weight_B, dpp->samples_B [0]);
		dpp->samples_B [0] = bptr [0];
	    }

	    break;
    }
}

#endif

// This is a helper function for unpack_samples() that applies several final
// operations. First, if the data is 32-bit float data, then that conversion
// is done in the float.c module (whether lossy or lossless) and we return.
// Otherwise, if the extended integer data applies, then that operation is
// executed first. If the unpacked data is lossy (and not corrected) then
// it is clipped and shifted in a single operation. Otherwise, if it's
// lossless then the last step is to apply the final shift (if any).

static void fixup_samples (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    uint32_t flags = wps->wphdr.flags;
    int lossy_flag = (flags & HYBRID_FLAG) && !wpc->wvc_flag;
    int shift = (flags & SHIFT_MASK) >> SHIFT_LSB;

    if (flags & FLOAT_DATA) {
	float_values (wps, buffer, (flags & MONO_FLAG) ? sample_count : sample_count * 2);
	return;
    }

    if (flags & INT32_DATA) {
	uint32_t count = (flags & MONO_FLAG) ? sample_count : sample_count * 2;
	int sent_bits = wps->int32_sent_bits, zeros = wps->int32_zeros;
	int ones = wps->int32_ones, dups = wps->int32_dups;
	uint32_t data, mask = (1 << sent_bits) - 1;
	int32_t *dptr = buffer;

	if (bs_is_open (&wps->wvxbits)) {
	    uint32_t crc = wps->crc_x;

	    while (count--) {
//		if (sent_bits) {
		    getbits (&data, sent_bits, &wps->wvxbits);
		    *dptr = (*dptr << sent_bits) | (data & mask);
//		}

		if (zeros)
		    *dptr <<= zeros;
		else if (ones)
		    *dptr = ((*dptr + 1) << ones) - 1;
		else if (dups)
		    *dptr = ((*dptr + (*dptr & 1)) << dups) - (*dptr & 1);

		crc = crc * 9 + (*dptr & 0xffff) * 3 + ((*dptr >> 16) & 0xffff);
		dptr++;
	    }

	    wps->crc_x = crc;
	}
	else if (!sent_bits && (zeros + ones + dups)) {
	    while (lossy_flag && (flags & BYTES_STORED) == 3 && shift < 8) {
		if (zeros)
		    zeros--;
		else if (ones)
		    ones--;
		else if (dups)
		    dups--;
		else
		    break;

		shift++;
	    }

	    while (count--) {
		if (zeros)
		    *dptr <<= zeros;
		else if (ones)
		    *dptr = ((*dptr + 1) << ones) - 1;
		else if (dups)
		    *dptr = ((*dptr + (*dptr & 1)) << dups) - (*dptr & 1);

		dptr++;
	    }
	}
	else
	    shift += zeros + sent_bits + ones + dups;
    }

    if (lossy_flag) {
	int32_t min_value, max_value, min_shifted, max_shifted;

	switch (flags & BYTES_STORED) {
	    case 0:
		min_shifted = (min_value = -128 >> shift) << shift;
		max_shifted = (max_value = 127 >> shift) << shift;
		break;

	    case 1:
		min_shifted = (min_value = -32768 >> shift) << shift;
		max_shifted = (max_value = 32767 >> shift) << shift;
		break;

	    case 2:
		min_shifted = (min_value = -8388608 >> shift) << shift;
		max_shifted = (max_value = 8388607 >> shift) << shift;
		break;

	    case 3:
		min_shifted = (min_value = (int32_t) 0x80000000 >> shift) << shift;
		max_shifted = (max_value = (int32_t) 0x7fffffff >> shift) << shift;
		break;
	}

	if (!(flags & MONO_FLAG))
	    sample_count *= 2;

	while (sample_count--) {
	    if (*buffer < min_value)
		*buffer++ = min_shifted;
	    else if (*buffer > max_value)
		*buffer++ = max_shifted;
	    else
		*buffer++ <<= shift;
	}
    }
    else if (shift) {
	if (!(flags & MONO_FLAG))
	    sample_count *= 2;

	while (sample_count--)
	    *buffer++ <<= shift;
    }
}

// This function checks the crc value(s) for an unpacked block, returning the
// number of actual crc errors detected for the block. The block must be
// completely unpacked before this test is valid. For losslessly unpacked
// blocks of float or extended integer data the extended crc is also checked.
// Note that WavPack's crc is not a CCITT approved polynomial algorithm, but
// is a much simpler method that is virtually as robust for real world data.

int check_crc_error (WavpackContext *wpc)
{
    int result = 0, stream;

    for (stream = 0; stream < wpc->num_streams; stream++) {
	WavpackStream *wps = wpc->streams [stream];

	if (wps->crc != wps->wphdr.crc)
	    ++result;
	else if (bs_is_open (&wps->wvxbits) && wps->crc_x != wps->crc_wvx)
	    ++result;
    }

    return result;
}

#endif
