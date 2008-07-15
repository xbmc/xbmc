////////////////////////////////////////////////////////////////////////////
//			     **** WAVPACK ****				  //
//		    Hybrid Lossless Wavefile Compressor			  //
//		Copyright (c) 1998 - 2005 Conifer Software.		  //
//			    All Rights Reserved.			  //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wputils.c

// This module provides a high-level interface to reading and writing WavPack
// files. WavPack input files can be opened as standard "C" streams using a
// provided filename. However, an alternate entry uses stream-reading
// callbacks to make using another file I/O method easy. Note that in this
// case the user application is responsible for finding and opening the .wvc
// file if the use of them is desired.

// For writing WavPack files there are no I/O routines used; a callback for
// writing completed blocks is provided.

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#endif

#include "wavpack.h"

#if !defined(WIN32)
#define stricmp(x,y) strcasecmp(x,y)
#endif

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

static void free_streams (WavpackContext *wpc);

///////////////////////////// local table storage ////////////////////////////

const uint32_t sample_rates [] = { 6000, 8000, 9600, 11025, 12000, 16000, 22050,
    24000, 32000, 44100, 48000, 64000, 88200, 96000, 192000 };

///////////////////////////// executable code ////////////////////////////////

#ifdef TAGS
static int load_tag (WavpackContext *wpc);
static int valid_tag (M_Tag *m_tag);
static void free_tag (M_Tag *m_tag);
#endif

#if defined(UNPACK) || defined(INFO_ONLY)

static uint32_t read_next_header (stream_reader *reader, void *id, WavpackHeader *wphdr);
static uint32_t seek_final_index (stream_reader *reader, void *id);

// This code provides an interface between the reader callback mechanism that
// WavPack uses internally and the standard fstream C library.

#ifdef USE_FSTREAMS

static int32_t read_bytes (void *id, void *data, int32_t bcount)
{
    return fread (data, 1, bcount, (FILE*) id);
}

static uint32_t get_pos (void *id)
{
    return ftell ((FILE*) id);
}

static int set_pos_abs (void *id, uint32_t pos)
{
    return fseek (id, pos, SEEK_SET);
}

static int set_pos_rel (void *id, int32_t delta, int mode)
{
    return fseek (id, delta, mode);
}

static int push_back_byte (void *id, int c)
{
    return ungetc (c, id);
}

static uint32_t get_length (void *id)
{
    FILE *file = id;
    struct stat statbuf;

    if (!file || fstat (fileno (file), &statbuf) || !(statbuf.st_mode & S_IFREG))
	return 0;

    return statbuf.st_size;
}

static int can_seek (void *id)
{
    FILE *file = id;
    struct stat statbuf;
    return file && !fstat (fileno (file), &statbuf) && (statbuf.st_mode & S_IFREG);
}

static stream_reader freader = {
    read_bytes, get_pos, set_pos_abs, set_pos_rel, push_back_byte, get_length, can_seek
};
	
// This function attempts to open the specified WavPack file for reading. If
// this fails for any reason then an appropriate message is copied to "error"
// and NULL is returned, otherwise a pointer to a WavpackContext structure is
// returned (which is used to call all other functions in this module). A
// filename beginning with "-" is assumed to be stdin. The "flags" argument
// has the following bit mask values to specify details of the open operation:

// OPEN_WVC:  attempt to open/read "correction" file
// OPEN_TAGS:  attempt to read ID3v1 / APEv2 tags (requires seekable file)
// OPEN_WRAPPER:  make audio wrapper available (i.e. RIFF) to caller
// OPEN_2CH_MAX:  open only first stream of multichannel file (usually L/R)
// OPEN_NORMALIZE:  normalize floating point data to +/- 1.0 (w/ offset exp)
// OPEN_STREAMING:  blindly unpacks blocks w/o regard to header file position

// Version 4.2 of the WavPack library adds the OPEN_STREAMING flag. This is
// essentially a "raw" mode where the library will simply decode any blocks
// fed it through the reader callback, regardless of where those blocks came
// from in a stream. The only requirement is that complete WavPack blocks are
// fed to the decoder (and this may require multiple blocks in multichannel
// mode) and that complete blocks are decoded (even if all samples are not
// actually required). All the blocks must contain the same number of channels
// and bit resolution, and the correction data must be either present or not.
// All other parameters may change from block to block (like lossy/lossless).
// Obviously, in this mode any seeking must be performed by the application
// (and again, decoding must start at the beginning of the block containing
// the seek sample).

WavpackContext *WavpackOpenFileInput (const char *infilename, char *error, int flags, int norm_offset)
{
    FILE *wv_id, *wvc_id;
    WavpackContext *wpc;

    if (*infilename == '-') {
	wv_id = stdin;
#if defined(WIN32)
	setmode (fileno (stdin), O_BINARY);
#endif
    }
    else if ((wv_id = fopen (infilename, "rb")) == NULL) {
	strcpy (error, "can't open file");
	return NULL;
    }

    if (wv_id != stdin && (flags & OPEN_WVC)) {
	char *in2filename = malloc (strlen (infilename) + 10);

	strcpy (in2filename, infilename);
	strcat (in2filename, "c");
	wvc_id = fopen (in2filename, "rb");
    }
    else
	wvc_id = NULL;

    wpc = WavpackOpenFileInputEx (&freader, wv_id, wvc_id, error, flags, norm_offset);

    if (!wpc) {
	if (wv_id)
	    fclose (wv_id);

	if (wvc_id)
	    fclose (wvc_id);
    }
    else
	wpc->close_files = TRUE;

    return wpc;
}

#endif

// This function is identical to WavpackOpenFileInput() except that instead
// of providing a filename to open, the caller provides a pointer to a set of
// reader callbacks and instances of up to two streams. The first of these
// streams is required and contains the regular WavPack data stream; the second
// contains the "correction" file if desired. Unlike the standard open
// function which handles the correction file transparently, in this case it
// is the responsibility of the caller to be aware of correction files.

WavpackContext *WavpackOpenFileInputEx (stream_reader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset)
{
    WavpackContext *wpc = malloc (sizeof (WavpackContext));
    WavpackStream *wps;
    uchar first_byte;
    uint32_t bcount;

    if (!wpc) {
	strcpy (error, "can't allocate memory");
	return NULL;
    }

    CLEAR (*wpc);
    wpc->wv_in = wv_id;
    wpc->wvc_in = wvc_id;
    wpc->reader = reader;
    wpc->total_samples = (uint32_t) -1;
    wpc->norm_offset = norm_offset;
    wpc->open_flags = flags;

    wpc->filelen = wpc->reader->get_length (wpc->wv_in);

#ifdef TAGS
    if ((flags & OPEN_TAGS) && wpc->reader->can_seek (wpc->wv_in)) {
	load_tag (wpc);
	wpc->reader->set_pos_abs (wpc->wv_in, 0);
    }
#endif

#ifdef VER3
    if (wpc->reader->read_bytes (wpc->wv_in, &first_byte, 1) != 1) {
	strcpy (error, "can't read all of WavPack file!");
	return WavpackCloseFile (wpc);
    }

    wpc->reader->push_back_byte (wpc->wv_in, first_byte);

    if (first_byte == 'R')
	return open_file3 (wpc, error);
#endif

    wpc->streams [0] = wps = malloc (sizeof (WavpackStream));
    wpc->num_streams = 1;
    CLEAR (*wps);

    while (!wps->wphdr.block_samples) {

	wpc->filepos = wpc->reader->get_pos (wpc->wv_in);
	bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

	if (bcount == (uint32_t) -1) {
	    strcpy (error, "not compatible with this version of WavPack file!");
	    return WavpackCloseFile (wpc);
	}

	wpc->filepos += bcount;
	wps->blockbuff = malloc (wps->wphdr.ckSize + 8);
	memcpy (wps->blockbuff, &wps->wphdr, 32);

	if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) != wps->wphdr.ckSize - 24) {
	    strcpy (error, "can't read all of WavPack file!");
	    return WavpackCloseFile (wpc);
	}

	if ((wps->wphdr.flags & UNKNOWN_FLAGS) || wps->wphdr.version < 0x402 || wps->wphdr.version > 0x40f) {
	    strcpy (error, "not compatible with this version of WavPack file!");
	    return WavpackCloseFile (wpc);
	}

	if (wps->wphdr.block_samples && !(flags & OPEN_STREAMING)) {
	    if (wps->wphdr.total_samples == (uint32_t) -1 && wpc->reader->can_seek (wpc->wv_in)) {
		uint32_t pos_save = wpc->reader->get_pos (wpc->wv_in);
		uint32_t final_index = seek_final_index (wpc->reader, wpc->wv_in);

		if (final_index != (uint32_t) -1)
		    wpc->total_samples = final_index - wps->wphdr.block_index;

		wpc->reader->set_pos_abs (wpc->wv_in, pos_save);
	    }
	    else
		wpc->total_samples = wps->wphdr.total_samples;
	}

	if (wpc->wvc_in && wps->wphdr.block_samples && (wps->wphdr.flags & HYBRID_FLAG)) {
	    wpc->file2len = wpc->reader->get_length (wpc->wvc_in);
	    wpc->wvc_flag = TRUE;
	}

	if (wpc->wvc_flag) {
	    wpc->file2pos = wpc->reader->get_pos (wpc->wvc_in);
	    bcount = read_next_header (wpc->reader, wpc->wvc_in, &wps->wphdr);

	    if (bcount == (uint32_t) -1) {
		strcpy (error, "problem with correction file");
		return WavpackCloseFile (wpc);
	    }

	    wpc->file2pos += bcount;
	    wps->block2buff = malloc (wps->wphdr.ckSize + 8);
	    memcpy (wps->block2buff, &wps->wphdr, 32);

	    if (wpc->reader->read_bytes (wpc->wvc_in, wps->block2buff + 32, wps->wphdr.ckSize - 24) !=
		wps->wphdr.ckSize - 24) {
		    strcpy (error, "can't read all of WavPack file!");
		    return WavpackCloseFile (wpc);
	    }
	}

	if (!unpack_init (wpc)) {
	    strcpy (error, wpc->error_message [0] ? wpc->error_message :
		"not compatible with this version of WavPack file!");

	    return WavpackCloseFile (wpc);
	}
    }

    wpc->config.flags &= ~0xff;
    wpc->config.flags |= wps->wphdr.flags & 0xff;
    wpc->config.bytes_per_sample = (wps->wphdr.flags & BYTES_STORED) + 1;
    wpc->config.float_norm_exp = wps->float_norm_exp;

    wpc->config.bits_per_sample = (wpc->config.bytes_per_sample * 8) - 
	((wps->wphdr.flags & SHIFT_MASK) >> SHIFT_LSB);

    if (!wpc->config.sample_rate) {
	if (!wps || !wps->wphdr.block_samples || (wps->wphdr.flags & SRATE_MASK) == SRATE_MASK)
	    wpc->config.sample_rate = 44100;
	else
	    wpc->config.sample_rate = sample_rates [(wps->wphdr.flags & SRATE_MASK) >> SRATE_LSB];
    }

    if (!wpc->config.num_channels) {
	wpc->config.num_channels = (wps->wphdr.flags & MONO_FLAG) ? 1 : 2;
	wpc->config.channel_mask = 0x5 - wpc->config.num_channels;
    }

    if ((flags & OPEN_2CH_MAX) && !(wps->wphdr.flags & FINAL_BLOCK))
	wpc->reduced_channels = (wps->wphdr.flags & MONO_FLAG) ? 1 : 2;

    return wpc;
}

// This function obtains general information about an open file and returns
// a mask with the following bit values:

// MODE_WVC:  a .wvc file has been found and will be used for lossless
// MODE_LOSSLESS:  file is lossless (either pure or hybrid)
// MODE_HYBRID:  file is hybrid mode (either lossy or lossless)
// MODE_FLOAT:  audio data is 32-bit ieee floating point
// MODE_VALID_TAG:  file conatins a valid ID3v1 or APEv2 tag
// MODE_HIGH:  file was created in "high" mode (information only)
// MODE_FAST:  file was created in "fast" mode (information only)
// MODE_EXTRA:  file was created using "extra" mode (information only)
// MODE_APETAG:  file contains a valid APEv2 tag
// MODE_SFX:  file was created as a "self-extracting" executable

int WavpackGetMode (WavpackContext *wpc)
{
    int mode = 0;

    if (wpc) {
	if (wpc->config.flags & CONFIG_HYBRID_FLAG)
	    mode |= MODE_HYBRID;
	else if (!(wpc->config.flags & CONFIG_LOSSY_MODE))
	    mode |= MODE_LOSSLESS;

	if (wpc->wvc_flag)
	    mode |= (MODE_LOSSLESS | MODE_WVC);

	if (wpc->lossy_blocks)
	    mode &= ~MODE_LOSSLESS;

	if (wpc->config.flags & CONFIG_FLOAT_DATA)
	    mode |= MODE_FLOAT;

	if (wpc->config.flags & CONFIG_HIGH_FLAG)
	    mode |= MODE_HIGH;

	if (wpc->config.flags & CONFIG_FAST_FLAG)
	    mode |= MODE_FAST;

	if (wpc->config.flags & CONFIG_EXTRA_MODE)
	    mode |= MODE_EXTRA;

	if (wpc->config.flags & CONFIG_CREATE_EXE)
	    mode |= MODE_SFX;

#ifdef TAGS
	if (valid_tag (&wpc->m_tag)) {
	    mode |= MODE_VALID_TAG;

	    if (valid_tag (&wpc->m_tag) == 'A')
		mode |= MODE_APETAG;
	}
#endif
    }

    return mode;
}

// This function returns the major version number of the WavPack program
// (or library) that created the open file. Currently, this can be 1 to 4.
// Minor versions are not recorded in WavPack files.

int WavpackGetVersion (WavpackContext *wpc)
{
    if (wpc) {
#ifdef VER3
	if (wpc->stream3)
	    return get_version3 (wpc);
#endif
	return 4;
    }

    return 0;
}

#endif

#ifdef UNPACK

// Unpack the specified number of samples from the current file position.
// Note that "samples" here refers to "complete" samples, which would be
// 2 longs for stereo files or even more for multichannel files, so the
// required memory at "buffer" is 4 * samples * num_channels bytes. The
// audio data is returned right-justified in 32-bit longs in the endian
// mode native to the executing processor. So, if the original data was
// 16-bit, then the values returned would be +/-32k. Floating point data
// can also be returned if the source was floating point data (and this
// can be optionally normalized to +/-1.0 by using the appropriate flag
// in the call to WavpackOpenFileInput ()). The actual number of samples
// unpacked is returned, which should be equal to the number requested unless
// the end of fle is encountered or an error occurs.

uint32_t WavpackUnpackSamples (WavpackContext *wpc, int32_t *buffer, uint32_t samples)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream = 0];
    uint32_t bcount, samples_unpacked = 0, samples_to_unpack;
    int num_channels = wpc->config.num_channels;

#ifdef VER3
    if (wpc->stream3)
	return unpack_samples3 (wpc, buffer, samples);
#endif

    while (samples) {
	if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) ||
	    wps->sample_index >= wps->wphdr.block_index + wps->wphdr.block_samples) {
		free_streams (wpc);
		wpc->filepos = wpc->reader->get_pos (wpc->wv_in);
		bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

		if (bcount == (uint32_t) -1)
		    break;

		if (wpc->open_flags & OPEN_STREAMING)
		    wps->wphdr.block_index = wps->sample_index = 0;

		wpc->filepos += bcount;
		wps->blockbuff = malloc (wps->wphdr.ckSize + 8);
		memcpy (wps->blockbuff, &wps->wphdr, 32);

		if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) !=
		    wps->wphdr.ckSize - 24) {
			strcpy (wpc->error_message, "can't read all of WavPack file!");
			break;
		}

		if (wps->wphdr.version < 0x402 || wps->wphdr.version > 0x40f) {
		    strcpy (wpc->error_message, "not compatible with this version of WavPack file!");
		    break;
		}

		if (wps->wphdr.block_samples && wpc->wvc_flag) {
		    wpc->file2pos = wpc->reader->get_pos (wpc->wvc_in);
		    bcount = read_next_header (wpc->reader, wpc->wvc_in, &wps->wphdr);

		    if (bcount == (uint32_t) -1)
			break;

		    if (wpc->open_flags & OPEN_STREAMING)
			wps->wphdr.block_index = wps->sample_index = 0;

		    wpc->file2pos += bcount;
		    wps->block2buff = malloc (wps->wphdr.ckSize + 8);
		    memcpy (wps->block2buff, &wps->wphdr, 32);

		    if (wpc->reader->read_bytes (wpc->wvc_in, wps->block2buff + 32, wps->wphdr.ckSize - 24) !=
			wps->wphdr.ckSize - 24)
			    break;
		}

		if (!wps->wphdr.block_samples || wps->sample_index == wps->wphdr.block_index)
		    if (!unpack_init (wpc))
			break;
	}

	if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) ||
	    wps->sample_index >= wps->wphdr.block_index + wps->wphdr.block_samples)
		continue;

	if (wps->sample_index < wps->wphdr.block_index) {
	    samples_to_unpack = wps->wphdr.block_index - wps->sample_index;

	    if (samples_to_unpack > samples)
		samples_to_unpack = samples;

	    wps->sample_index += samples_to_unpack;
	    samples_unpacked += samples_to_unpack;
	    samples -= samples_to_unpack;

	    if (wpc->reduced_channels)
		samples_to_unpack *= wpc->reduced_channels;
	    else
		samples_to_unpack *= num_channels;

	    while (samples_to_unpack--)
		*buffer++ = 0;

	    continue;
	}

	samples_to_unpack = wps->wphdr.block_index + wps->wphdr.block_samples - wps->sample_index;

	if (samples_to_unpack > samples)
	    samples_to_unpack = samples;

	if (!wpc->reduced_channels && !(wps->wphdr.flags & FINAL_BLOCK)) {
	    int32_t *temp_buffer = malloc (samples_to_unpack * 8), *src, *dst;
	    int offset = 0;
	    uint32_t samcnt;

	    while (1) {
		if (wpc->current_stream == wpc->num_streams) {
		    wps = wpc->streams [wpc->num_streams++] = malloc (sizeof (WavpackStream));
		    CLEAR (*wps);
		    bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

		    if (bcount == (uint32_t) -1)
			break;

		    if (wpc->open_flags & OPEN_STREAMING)
			wps->wphdr.block_index = wps->sample_index = 0;

		    wps->blockbuff = malloc (wps->wphdr.ckSize + 8);
		    memcpy (wps->blockbuff, &wps->wphdr, 32);

		    if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) !=
			wps->wphdr.ckSize - 24) {
			    strcpy (wpc->error_message, "can't read all of WavPack file!");
			    break;
		    }

		    if (wpc->wvc_flag) {
			bcount = read_next_header (wpc->reader, wpc->wvc_in, &wps->wphdr);

			if (bcount == (uint32_t) -1)
			    break;

			if (wpc->open_flags & OPEN_STREAMING)
			    wps->wphdr.block_index = wps->sample_index = 0;

			wps->block2buff = malloc (wps->wphdr.ckSize + 8);
			memcpy (wps->block2buff, &wps->wphdr, 32);

			if (wpc->reader->read_bytes (wpc->wvc_in, wps->block2buff + 32, wps->wphdr.ckSize - 24) !=
			    wps->wphdr.ckSize - 24)
				break;
		    }

		    if (!unpack_init (wpc))
			break;
		}
		else
		    wps = wpc->streams [wpc->current_stream];

		unpack_samples (wpc, src = temp_buffer, samples_to_unpack);
		samcnt = samples_to_unpack;
		dst = buffer + offset;

		if (wps->wphdr.flags & MONO_FLAG) {
		    while (samcnt--) {
			dst [0] = *src++;
			dst += num_channels;
		    }

		    offset++;
		}
		else {
		    while (samcnt--) {
			dst [0] = *src++;
			dst [1] = *src++;
			dst += num_channels;
		    }

		    offset += 2;
		}
	
		if (wps->wphdr.flags & FINAL_BLOCK)
		    break;
		else
		    wpc->current_stream++;
	    }

	    wps = wpc->streams [wpc->current_stream = 0];
	    free (temp_buffer);
	}
	else
	    unpack_samples (wpc, buffer, samples_to_unpack);

	if (wpc->reduced_channels)
	    buffer += samples_to_unpack * wpc->reduced_channels;
	else
	    buffer += samples_to_unpack * num_channels;

	samples_unpacked += samples_to_unpack;
	samples -= samples_to_unpack;

	if (wps->sample_index == wps->wphdr.block_index + wps->wphdr.block_samples) {
	    if (check_crc_error (wpc) && wps->blockbuff) {

		if (wpc->reader->can_seek (wpc->wv_in)) {
		    int32_t rseek = ((WavpackHeader *) wps->blockbuff)->ckSize / 3;
		    wpc->reader->set_pos_rel (wpc->wv_in, (rseek > 16384) ? -16384 : -rseek, SEEK_CUR);
		}

		if (wpc->wvc_flag && wps->block2buff && wpc->reader->can_seek (wpc->wvc_in)) {
		    int32_t rseek = ((WavpackHeader *) wps->block2buff)->ckSize / 3;
		    wpc->reader->set_pos_rel (wpc->wvc_in, (rseek > 16384) ? -16384 : -rseek, SEEK_CUR);
		}

		wpc->crc_errors++;
	    }
	}

	if (wps->sample_index == wpc->total_samples)
	    break;
    }

    return samples_unpacked;
}

#ifdef SEEKING

static uint32_t find_sample (WavpackContext *wpc, void *infile, uint32_t header_pos, uint32_t sample);

// Seek to the specifed sample index, returning TRUE on success. Note that
// files generated with version 4.0 or newer will seek almost immediately.
// Older files can take quite long if required to seek through unplayed
// portions of the file, but will create a seek map so that reverse seeks
// (or forward seeks to already scanned areas) will be very fast.

int WavpackSeekSample (WavpackContext *wpc, uint32_t sample)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream = 0];
    uint32_t bcount, samples_to_skip;
    int32_t *buffer;

    if (wpc->total_samples == (uint32_t) -1 || sample >= wpc->total_samples ||
	!wpc->reader->can_seek (wpc->wv_in) || (wpc->open_flags & OPEN_STREAMING) ||
	(wpc->wvc_flag && !wpc->reader->can_seek (wpc->wvc_in)))
	    return FALSE;

#ifdef VER3
    if (wpc->stream3)
	return seek_sample3 (wpc, sample);
#endif

    if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) || sample < wps->wphdr.block_index ||
	sample >= wps->wphdr.block_index + wps->wphdr.block_samples) {

	    free_streams (wpc);
	    wpc->filepos = find_sample (wpc, wpc->wv_in, wpc->filepos, sample);

	    if (wpc->filepos == (uint32_t) -1)
		return FALSE;

	    if (wpc->wvc_flag) {
		wpc->file2pos = find_sample (wpc, wpc->wvc_in, 0, sample);

		if (wpc->file2pos == (uint32_t) -1)
		    return FALSE;
	    }
    }

    if (!wps->blockbuff) {
	wpc->reader->set_pos_abs (wpc->wv_in, wpc->filepos);
	wpc->reader->read_bytes (wpc->wv_in, &wps->wphdr, sizeof (WavpackHeader));
	little_endian_to_native (&wps->wphdr, WavpackHeaderFormat);
	wps->blockbuff = malloc (wps->wphdr.ckSize + 8);
	memcpy (wps->blockbuff, &wps->wphdr, sizeof (WavpackHeader));

	if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + sizeof (WavpackHeader), wps->wphdr.ckSize - 24) !=
	    wps->wphdr.ckSize - 24) {
		free_streams (wpc);
		return FALSE;
	}

	if (wpc->wvc_flag) {
	    wpc->reader->set_pos_abs (wpc->wvc_in, wpc->file2pos);
	    wpc->reader->read_bytes (wpc->wvc_in, &wps->wphdr, sizeof (WavpackHeader));
	    little_endian_to_native (&wps->wphdr, WavpackHeaderFormat);
	    wps->block2buff = malloc (wps->wphdr.ckSize + 8);
	    memcpy (wps->block2buff, &wps->wphdr, sizeof (WavpackHeader));

	    if (wpc->reader->read_bytes (wpc->wvc_in, wps->block2buff + sizeof (WavpackHeader), wps->wphdr.ckSize - 24) !=
		wps->wphdr.ckSize - 24) {
		    free_streams (wpc);
		    return FALSE;
	    }
	}

	if (!unpack_init (wpc)) {
	    free_streams (wpc);
	    return FALSE;
	}
    }

    while (!wpc->reduced_channels && !(wps->wphdr.flags & FINAL_BLOCK)) {
	if (++wpc->current_stream == wpc->num_streams) {
	    wps = wpc->streams [wpc->num_streams++] = malloc (sizeof (WavpackStream));
	    CLEAR (*wps);
	    bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

	    if (bcount == (uint32_t) -1)
		break;

	    wps->blockbuff = malloc (wps->wphdr.ckSize + 8);
	    memcpy (wps->blockbuff, &wps->wphdr, 32);

	    if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) !=
		wps->wphdr.ckSize - 24) {
		    strcpy (wpc->error_message, "can't read all of WavPack file!");
		    break;
	    }

	    if (wpc->wvc_flag) {
		bcount = read_next_header (wpc->reader, wpc->wvc_in, &wps->wphdr);

		if (bcount == (uint32_t) -1)
		    break;

		wps->block2buff = malloc (wps->wphdr.ckSize + 8);
		memcpy (wps->block2buff, &wps->wphdr, 32);

		if (wpc->reader->read_bytes (wpc->wvc_in, wps->block2buff + 32, wps->wphdr.ckSize - 24) !=
		    wps->wphdr.ckSize - 24)
			break;
	    }

	    if (!unpack_init (wpc))
		break;
	}
	else
	    wps = wpc->streams [wpc->current_stream];
    }

    if (sample < wps->sample_index)
	for (wpc->current_stream = 0; wpc->current_stream < wpc->num_streams; wpc->current_stream++)
	    if (!unpack_init (wpc))
		return FALSE;

    samples_to_skip = sample - wps->sample_index;

    if (samples_to_skip) {
	buffer = malloc (samples_to_skip * 8);

	for (wpc->current_stream = 0; wpc->current_stream < wpc->num_streams; wpc->current_stream++)
	    unpack_samples (wpc, buffer, samples_to_skip);

	free (buffer);
    }

    wpc->current_stream = 0;
    return TRUE;
}

#endif

#ifdef TAGS

// Attempt to get the specified item from the specified ID3v1 or APEv2 tag.
// The "size" parameter specifies the amount of space available at "value",
// if the desired item will not fit in this space then ellipses (...) will
// be appended and the string terminated. Only text data are supported.

static void tagcpy (char *dest, char *src, int tag_size);

int WavpackGetTagItem (WavpackContext *wpc, const char *item, char *value, int size)
{
    M_Tag *m_tag = &wpc->m_tag;
    char *lvalue = NULL;

    if (value)
	*value = 0;

    if (m_tag->ape_tag_hdr.ID [0] == 'A') {
	char *p = m_tag->ape_tag_data;
	char *q = p + m_tag->ape_tag_hdr.length - sizeof (APE_Tag_Hdr);
	int i;

	for (i = 0; i < m_tag->ape_tag_hdr.item_count; ++i) {
	    int vsize, flags, isize;

	    vsize = * (int32_t *) p; p += 4;
	    flags = * (int32_t *) p; p += 4;
	    isize = strlen (p);

	    little_endian_to_native (&vsize, "L");
	    little_endian_to_native (&flags, "L");

	    if (p + isize + vsize + 1 > q)
		break;

	    if (isize && vsize && !stricmp (item, p) && !(flags & 6)) {

		if ((lvalue = malloc (vsize + 1)) != NULL) {
		    strncpy (lvalue, p + isize + 1, vsize);
		    lvalue [vsize] = 0;
		}

		break;
	    }
	    else
		p += isize + vsize + 1;
	}
    }
    else if (m_tag->id3_tag.tag_id [0] == 'T') {
	if ((lvalue = malloc (128)) == NULL)
	    return FALSE;

	lvalue [0] = 0;

	if (!stricmp (item, "title"))
	    tagcpy (lvalue, m_tag->id3_tag.title, sizeof (m_tag->id3_tag.title));
	else if (!stricmp (item, "artist"))
	    tagcpy (lvalue, m_tag->id3_tag.artist, sizeof (m_tag->id3_tag.artist));
	else if (!stricmp (item, "album"))
	    tagcpy (lvalue, m_tag->id3_tag.album, sizeof (m_tag->id3_tag.album));
	else if (!stricmp (item, "year"))
	    tagcpy (lvalue, m_tag->id3_tag.year, sizeof (m_tag->id3_tag.year));
	else if (!stricmp (item, "comment"))
	    tagcpy (lvalue, m_tag->id3_tag.comment, sizeof (m_tag->id3_tag.title));

	if (!lvalue [0]) {
	    free (lvalue);
	    return FALSE;
	}
    }
    else
	return FALSE;

    if (lvalue) {
	if (value && size >= 4) {
	    if (strlen (lvalue) >= size) {
		lvalue [size - 4] = lvalue [size - 3] = lvalue [size - 2] = '.';
		lvalue [size - 1] = 0;
	    }

	    strcpy (value, lvalue);
	}

	free (lvalue);
	return TRUE;
    }
    else
	return FALSE;
}

#endif

#endif

#ifdef PACK

// Open context for writing WavPack files. The returned context pointer is used
// in all following calls to the library. The "blockout" function will be used
// to store the actual completed WavPack blocks and will be called with the id
// pointers containing user defined data (one for the wv file and one for the
// wvc file). A return value of NULL indicates that memory could not be
// allocated for the context.

WavpackContext *WavpackOpenFileOutput (blockout_f blockout, void *wv_id, void *wvc_id)
{
    WavpackContext *wpc = malloc (sizeof (WavpackContext));

    if (!wpc)
	return NULL;

    CLEAR (*wpc);
    wpc->blockout = blockout;
    wpc->wv_out = wv_id;
    wpc->wvc_out = wvc_id;
    return wpc;
}

// Set configuration for writing WavPack files. This must be done before
// sending any actual samples, however it is okay to send wrapper or other
// metadata before calling this. The "config" structure contains the following
// required information:

// config->bytes_per_sample     see WavpackGetBytesPerSample() for info
// config->bits_per_sample      see WavpackGetBitsPerSample() for info
// config->channel_mask         Microsoft standard (mono = 4, stereo = 3)
// config->num_channels         self evident
// config->sample_rate          self evident

// In addition, the following fields and flags may be set: 

// config->flags:
// --------------
// o CONFIG_HYBRID_FLAG         select hybrid mode (must set bitrate)
// o CONFIG_JOINT_STEREO        select joint stereo (must set override also)
// o CONFIG_JOINT_OVERRIDE      override default joint stereo selection
// o CONFIG_HYBRID_SHAPE        select hybrid noise shaping (set override &
//                                                      shaping_weight != 0.0)
// o CONFIG_SHAPE_OVERRIDE      override default hybrid noise shaping
//                               (set CONFIG_HYBRID_SHAPE and shaping_weight)
// o CONFIG_FAST_FLAG           "fast" compression mode
// o CONFIG_HIGH_FLAG           "high" compression mode
// o CONFIG_BITRATE_KBPS        hybrid bitrate is kbps, not bits / sample
// o CONFIG_CREATE_WVC          create correction file
// o CONFIG_OPTIMIZE_WVC        maximize bybrid compression (-cc option)
// o CONFIG_CALC_NOISE          calc noise in hybrid mode
// o CONFIG_EXTRA_MODE          extra processing mode (slow!)
// o CONFIG_SKIP_WVX		no wvx stream for floats & large ints

// config->bitrate              hybrid bitrate in either bits/sample or kbps
// config->shaping_weight       hybrid noise shaping coefficient override
// config->block_samples        force samples per WavPack block (0 = use deflt)
// config->float_norm_exp       select floating-point data (127 for +/-1.0)
// config->xmode                extra mode processing value override

// If the number of samples to be written is known then it should be passed
// here. If the duration is not known then pass -1. In the case that the size
// is not known (or the writing is terminated early) then it is suggested that
// the application retrieve the first block written and let the library update
// the total samples indication. A function is provided to do this update and
// it should be done to the "correction" file also. If this cannot be done
// (because a pipe is being used, for instance) then a valid WavPack will still
// be created, but when applications want to access that file they will have
// to seek all the way to the end to determine the actual duration. Also, if
// a RIFF header has been included then it should be updated as well or the
// WavPack file will not be directly unpackable to a valid wav file (although
// it will still be usable by itself). A return of FALSE indicates an error.

static const uint32_t xtable [] = { 123, 3, 27, 59, 123, 187, 251 };
static const uint32_t f_xtable [] = { 251, 3, 27, 59, 123, 187, 251 };
static const uint32_t h_xtable [] = { 91, 3, 27, 91, 123, 187, 251 };

int WavpackSetConfiguration (WavpackContext *wpc, WavpackConfig *config, uint32_t total_samples)
{
    uint32_t flags = (config->bytes_per_sample - 1), bps = 0, shift = 0;
    uint32_t chan_mask = config->channel_mask;
    int num_chans = config->num_channels;
    int i;

    wpc->total_samples = total_samples;
    wpc->config.sample_rate = config->sample_rate;
    wpc->config.num_channels = config->num_channels;
    wpc->config.channel_mask = config->channel_mask;
    wpc->config.bits_per_sample = config->bits_per_sample;
    wpc->config.bytes_per_sample = config->bytes_per_sample;
    wpc->config.block_samples = config->block_samples;
    wpc->config.flags = config->flags;

    if (config->float_norm_exp) {
	wpc->config.float_norm_exp = config->float_norm_exp;
	flags |= FLOAT_DATA;    
    }
    else
	shift = (config->bytes_per_sample * 8) - config->bits_per_sample;

    for (i = 0; i < 15; ++i)
	if (wpc->config.sample_rate == sample_rates [i])
	    break;

    flags |= i << SRATE_LSB;
    flags |= shift << SHIFT_LSB;

    if (config->flags & CONFIG_HYBRID_FLAG) {
	flags |= HYBRID_FLAG | HYBRID_BITRATE | HYBRID_BALANCE;

	if (!(wpc->config.flags & CONFIG_SHAPE_OVERRIDE)) {
	    wpc->config.flags |= CONFIG_HYBRID_SHAPE | CONFIG_AUTO_SHAPING;
	    flags |= HYBRID_SHAPE | NEW_SHAPING;
	}
	else if (wpc->config.flags & CONFIG_HYBRID_SHAPE) {
	    wpc->config.shaping_weight = config->shaping_weight;
	    flags |= HYBRID_SHAPE | NEW_SHAPING;
	}

	if (wpc->config.flags & CONFIG_OPTIMIZE_WVC)
	    flags |= CROSS_DECORR;

	if (config->flags & CONFIG_BITRATE_KBPS) {
	    bps = floor (config->bitrate * 256000.0 / config->sample_rate / config->num_channels + 0.5);

	    if (bps > (64 << 8))
		bps = 64 << 8;
	}
	else
	    bps = floor (config->bitrate * 256.0 + 0.5);
    }
    else
	flags |= CROSS_DECORR;

    if (!(config->flags & CONFIG_JOINT_OVERRIDE) || (config->flags & CONFIG_JOINT_STEREO))
	flags |= JOINT_STEREO;

    if (config->flags & CONFIG_CREATE_WVC)
	wpc->wvc_flag = TRUE;

    for (wpc->current_stream = 0; num_chans; wpc->current_stream++) {
	WavpackStream *wps = malloc (sizeof (WavpackStream));
	uint32_t stereo_mask, mono_mask;
	int pos, chans;

	wpc->streams [wpc->current_stream] = wps;
	CLEAR (*wps);

	for (pos = 1; pos <= 18; ++pos) {
	    stereo_mask = 3 << (pos - 1);
	    mono_mask = 1 << (pos - 1);

	    if ((chan_mask & stereo_mask) == stereo_mask && (mono_mask & 0x251)) {
		chan_mask &= ~stereo_mask;
		chans = 2;
		break;
	    }
	    else if (chan_mask & mono_mask) {
		chan_mask &= ~mono_mask;
		chans = 1;
		break;
	    }
	}

	if (pos == 19)
	    chans = num_chans > 1 ? 2 : 1;

	num_chans -= chans;

	if (num_chans && wpc->current_stream == MAX_STREAMS - 1)
	    break;

	memcpy (wps->wphdr.ckID, "wvpk", 4);
	wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
	wps->wphdr.total_samples = wpc->total_samples;
	wps->wphdr.version = 0x403;
	wps->wphdr.flags = flags;
	wps->bits = bps;

	if (!wpc->current_stream)
	    wps->wphdr.flags |= INITIAL_BLOCK;

	if (!num_chans)
	    wps->wphdr.flags |= FINAL_BLOCK;

	if (chans == 1) {
	    wps->wphdr.flags &= ~(JOINT_STEREO | CROSS_DECORR | HYBRID_BALANCE);
	    wps->wphdr.flags |= MONO_FLAG;
	}
    }

    wpc->num_streams = wpc->current_stream;
    wpc->current_stream = 0;

    if (num_chans) {
	strcpy (wpc->error_message, "too many channels!");
	return FALSE;
    }

    if (config->flags & CONFIG_EXTRA_MODE) {

	if (config->flags & CONFIG_HIGH_FLAG)
	    wpc->config.extra_flags = h_xtable [config->xmode];
	else if (config->flags & CONFIG_FAST_FLAG)
	    wpc->config.extra_flags = f_xtable [config->xmode];
	else
	    wpc->config.extra_flags = xtable [config->xmode];

	if (config->flags & CONFIG_JOINT_OVERRIDE)
	    wpc->config.extra_flags &= ~EXTRA_STEREO_MODES;
    }

    return TRUE;
}

// Prepare to actually pack samples by determining the size of the WavPack
// blocks and allocating sample buffers and initializing each stream. Call
// after WavpackSetConfiguration() and before WavpackPackSamples(). A return
// of FALSE indicates an error.

int WavpackPackInit (WavpackContext *wpc)
{
    if (wpc->metabytes > 4096)
	write_metadata_block (wpc);

    if (wpc->config.block_samples)
	wpc->block_samples = wpc->config.block_samples;
    else {
	if (wpc->config.flags & CONFIG_HIGH_FLAG)
	    wpc->block_samples = wpc->config.sample_rate;
//	else if ((wpc->config.flags & CONFIG_FAST_FLAG) && !(wpc->config.sample_rate % 4))
//	    wpc->block_samples = wpc->config.sample_rate / 4;
	else if (!(wpc->config.sample_rate % 2))
	    wpc->block_samples = wpc->config.sample_rate / 2;
	else
	    wpc->block_samples = wpc->config.sample_rate;

	while (wpc->block_samples * wpc->config.num_channels > 100000)
	    wpc->block_samples /= 2;

	while (wpc->block_samples * wpc->config.num_channels < 40000)
	    wpc->block_samples *= 2;
    }

    wpc->max_samples = wpc->block_samples + (wpc->block_samples >> 1);

    for (wpc->current_stream = 0; wpc->streams [wpc->current_stream]; wpc->current_stream++) {
	WavpackStream *wps = wpc->streams [wpc->current_stream];

	wps->sample_buffer = malloc (wpc->max_samples * (wps->wphdr.flags & MONO_FLAG ? 4 : 8));
	pack_init (wpc);
    }

    return TRUE;
}

// Pack the specified samples. Samples must be stored in longs in the native
// endian format of the executing processor. The number of samples specified
// indicates composite samples (sometimes called "frames"). So, the actual
// number of data points would be this "sample_count" times the number of
// channels. Note that samples are accumulated here until enough exist to
// create a complete WavPack block (or several blocks for multichannel audio).
// If an application wants to break a block at a specific sample, then it must
// simply call WavpackFlushSamples() to force an early termination. Completed
// WavPack blocks are send to the function provided in the initial call to
// WavpackOpenFileOutput(). A return of FALSE indicates an error.

static int pack_streams (WavpackContext *wpc, uint32_t block_samples);

int WavpackPackSamples (WavpackContext *wpc, int32_t *sample_buffer, uint32_t sample_count)
{
    int nch = wpc->config.num_channels;

    while (sample_count) {
	int32_t *source_pointer = sample_buffer;
	uint samples_to_copy;

	if (wpc->acc_samples + sample_count > wpc->max_samples)
	    samples_to_copy = wpc->max_samples - wpc->acc_samples;
	else
	    samples_to_copy = sample_count;
   
	for (wpc->current_stream = 0; wpc->streams [wpc->current_stream]; wpc->current_stream++) {
	    WavpackStream *wps = wpc->streams [wpc->current_stream];
	    int32_t *dptr, *sptr, cnt;

	    dptr = wps->sample_buffer + wpc->acc_samples * (wps->wphdr.flags & MONO_FLAG ? 1 : 2);
	    sptr = source_pointer;
	    cnt = samples_to_copy;

	    if (wps->wphdr.flags & MONO_FLAG) {
		while (cnt--) {
		    *dptr++ = *sptr;
		    sptr += nch;
		}

		source_pointer++;
	    }
	    else {
		while (cnt--) {
		    *dptr++ = sptr [0];
		    *dptr++ = sptr [1];
		    sptr += nch;
		}

		source_pointer += 2;
	    }
	}

	sample_buffer += samples_to_copy * nch;
	sample_count -= samples_to_copy;

	if ((wpc->acc_samples += samples_to_copy) == wpc->max_samples &&
	    !pack_streams (wpc, wpc->block_samples))
		return FALSE;
    }

    return TRUE;
}

// Flush all accumulated samples into WavPack blocks. This is normally called
// after all samples have been sent to WavpackPackSamples(), but can also be
// called to terminate a WavPack block at a specific sample (in other words it
// is possible to continue after this operation). This is also called to
// dump non-audio blocks like those holding metadata for various purposes.
// A return of FALSE indicates an error.

int WavpackFlushSamples (WavpackContext *wpc)
{
    while (wpc->acc_samples) {
	uint32_t block_samples;

	if (wpc->acc_samples > wpc->block_samples)
	    block_samples = wpc->acc_samples / 2;
	else
	    block_samples = wpc->acc_samples;

	if (!pack_streams (wpc, block_samples))
	    return FALSE;
    }

    if (wpc->metacount)
	write_metadata_block (wpc);

    return TRUE;
}

// Add wrapper (currently RIFF only) to WavPack blocks. This should be called
// before sending any audio samples for the RIFF header or after all samples
// have been sent for any RIFF trailer. WavpackFlushSamples() should be called
// between sending the last samples and calling this for trailer data to make
// sure that headers and trailers don't get mixed up in very short files. If
// the exact contents of the RIFF header are not known because, for example,
// the file duration is uncertain or trailing chunks are possible, simply write
// a "dummy" header of the correct length. When all data has been written it
// will be possible to read the first block written and update the header
// directly. An example of this can be found in the Audition filter. A
// return of FALSE indicates an error.

int WavpackAddWrapper (WavpackContext *wpc, void *data, uint32_t bcount)
{
    uint32_t index = WavpackGetSampleIndex (wpc);

    return add_to_metadata (wpc, data, bcount,
	(uchar)((!index || index == (uint32_t) -1) ? ID_RIFF_HEADER : ID_RIFF_TRAILER));
}

// Store computed MD5 sum in WavPack metadata. Note that the user must compute
// the 16 byte sum; it is not done here. A return of FALSE indicates an error.

int WavpackStoreMD5Sum (WavpackContext *wpc, uchar data [16])
{
    return add_to_metadata (wpc, data, 16, ID_MD5_CHECKSUM);
}

static int pack_streams (WavpackContext *wpc, uint32_t block_samples)
{
    uint32_t max_blocksize = block_samples * 10 + 4096, bcount;
    uchar *outbuff, *outend, *out2buff, *out2end;
    int result;

    out2buff = (wpc->wvc_flag) ? malloc (max_blocksize) : NULL;
    out2end = out2buff + max_blocksize;
    outbuff = malloc (max_blocksize);
    outend = outbuff + max_blocksize;

    for (wpc->current_stream = 0; wpc->streams [wpc->current_stream]; wpc->current_stream++) {
	WavpackStream *wps = wpc->streams [wpc->current_stream];
	uint32_t flags = wps->wphdr.flags;

	flags &= ~MAG_MASK;
	flags += (1 << MAG_LSB) * ((flags & BYTES_STORED) * 8 + 7);

	wps->wphdr.block_index = wps->sample_index;
	wps->wphdr.block_samples = block_samples;
	wps->wphdr.flags = flags;
	wps->block2buff = out2buff;
	wps->block2end = out2end;
	wps->blockbuff = outbuff;
	wps->blockend = outend;

	result = pack_block (wpc, wps->sample_buffer);
	wps->blockbuff = wps->block2buff = NULL;

	if (!result) {
	    strcpy (wpc->error_message, "output buffer overflowed!");
	    break;
	}

	bcount = ((WavpackHeader *) outbuff)->ckSize + 8;
	native_to_little_endian ((WavpackHeader *) outbuff, WavpackHeaderFormat);
	result = wpc->blockout (wpc->wv_out, outbuff, bcount);

	if (!result) {
	    strcpy (wpc->error_message, "can't write WavPack data, disk probably full!");
	    break;
	}

	wpc->filelen += bcount;

	if (out2buff) {
	    bcount = ((WavpackHeader *) out2buff)->ckSize + 8;
	    native_to_little_endian ((WavpackHeader *) out2buff, WavpackHeaderFormat);
	    result = wpc->blockout (wpc->wvc_out, out2buff, bcount);

	    if (!result) {
		strcpy (wpc->error_message, "can't write WavPack data, disk probably full!");
		break;
	    }

	    wpc->file2len += bcount;
	}

	if (wpc->acc_samples != block_samples)
	    memcpy (wps->sample_buffer, wps->sample_buffer + block_samples * (flags & MONO_FLAG ? 1 : 2),
		(wpc->acc_samples - block_samples) * sizeof (int32_t) * (flags & MONO_FLAG ? 1 : 2));
    }

    wpc->current_stream = 0;
    wpc->acc_samples -= block_samples;
    free (outbuff);

    if (out2buff)
	free (out2buff);

    return result;
}

// Given the pointer to the first block written (to either a .wv or .wvc file),
// update the block with the actual number of samples written. This should
// be done if WavpackSetConfiguration() was called with an incorrect number
// of samples (or -1). It is the responsibility of the application to read and
// rewrite the block. An example of this can be found in the Audition filter.

void WavpackUpdateNumSamples (WavpackContext *wpc, void *first_block)
{
    little_endian_to_native (wpc, WavpackHeaderFormat);
    ((WavpackHeader *) first_block)->total_samples = WavpackGetSampleIndex (wpc);
    native_to_little_endian (wpc, WavpackHeaderFormat);
}

// Given the pointer to the first block written to a WavPack file, this
// function returns the location of the stored RIFF header that was originally
// written with WavpackAddWrapper(). This would normally be used to update
// the wav header to indicate that a different number of samples was actually
// written or if additional RIFF chunks are written at the end of the file.
// It is the responsibility of the application to read and rewrite the block.
// An example of this can be found in the Audition filter.

void *WavpackGetWrapperLocation (void *first_block)
{
    if (((uchar *) first_block) [32] == ID_RIFF_HEADER)
	return ((uchar *) first_block) + 34;
    else
	return NULL;
}

#ifdef TAGS

// Limited functionality to append APEv2 tags to WavPack files when they are
// created has been added for version 4.2. This function is used to append the
// specified field to the tag being created. If no tag has been started, then
// an empty one will be allocated first. When finished, use WavpackWriteTag()
// to write the completed tag to the file. Note that ID3 tags are not
// supported and that no editing of existing tags is allowed (there are several
// fine libraries available for this).

int WavpackAppendTagItem (WavpackContext *wpc, const char *item, const char *value)
{
    M_Tag *m_tag = &wpc->m_tag;
    int vsize = strlen (value);
    int isize = strlen (item);

    if (!m_tag->ape_tag_hdr.ID [0]) {
	strncpy (m_tag->ape_tag_hdr.ID, "APETAGEX", sizeof (m_tag->ape_tag_hdr.ID));
	m_tag->ape_tag_hdr.version = 2000;
	m_tag->ape_tag_hdr.length = sizeof (m_tag->ape_tag_hdr);
	m_tag->ape_tag_hdr.item_count = 0;
	m_tag->ape_tag_hdr.flags = 0x80000000;
    }

    if (m_tag->ape_tag_hdr.ID [0] == 'A') {
	int new_item_len = vsize + isize + 9, flags = 0;
	char *p;

	m_tag->ape_tag_hdr.item_count++;
	m_tag->ape_tag_hdr.length += new_item_len;
	p = m_tag->ape_tag_data = realloc (m_tag->ape_tag_data, m_tag->ape_tag_hdr.length);
	p += m_tag->ape_tag_hdr.length - sizeof (APE_Tag_Hdr) - new_item_len;
	native_to_little_endian (&vsize, "L");
	native_to_little_endian (&flags, "L");
	* (int32_t *) p = vsize; p += 4;
	* (int32_t *) p = flags; p += 4;
	little_endian_to_native (&vsize, "L");
	little_endian_to_native (&flags, "L");
	strcpy (p, item);
	p += isize + 1;
	memcpy (p, value, vsize);

	return TRUE;
    }
    else
	return FALSE;
}

// Once a APEv2 tag has been created with WavpackAppendTag(), this function is
// used to write the completed tag to the end of the WavPack file. Note that
// this function uses the same "blockout" function that is used to write
// regular WavPack blocks, although that's where the similarity ends.

int WavpackWriteTag (WavpackContext *wpc)
{
    M_Tag *m_tag = &wpc->m_tag;
    int result = TRUE;

    if (m_tag->ape_tag_hdr.ID [0] == 'A') {
	m_tag->ape_tag_hdr.flags |= 0x20000000;
	native_to_little_endian (&m_tag->ape_tag_hdr, APE_Tag_Hdr_Format);
	result = wpc->blockout (wpc->wv_out, &m_tag->ape_tag_hdr, sizeof (m_tag->ape_tag_hdr));
	little_endian_to_native (&m_tag->ape_tag_hdr, APE_Tag_Hdr_Format);

	if (m_tag->ape_tag_hdr.length > sizeof (m_tag->ape_tag_hdr))
	    result = wpc->blockout (wpc->wv_out, m_tag->ape_tag_data, m_tag->ape_tag_hdr.length - sizeof (m_tag->ape_tag_hdr));

	m_tag->ape_tag_hdr.flags &= ~0x20000000;
	native_to_little_endian (&m_tag->ape_tag_hdr, APE_Tag_Hdr_Format);
	result = wpc->blockout (wpc->wv_out, &m_tag->ape_tag_hdr, sizeof (m_tag->ape_tag_hdr));
	little_endian_to_native (&m_tag->ape_tag_hdr, APE_Tag_Hdr_Format);
    }
    else if (m_tag->id3_tag.tag_id [0] == 'T')
	result = wpc->blockout (wpc->wv_out, &m_tag->id3_tag, sizeof (m_tag->id3_tag));

    if (!result)
	strcpy (wpc->error_message, "can't write WavPack data, disk probably full!");

    return result;
}

#endif

#endif

// Get total number of samples contained in the WavPack file, or -1 if unknown

uint32_t WavpackGetNumSamples (WavpackContext *wpc)
{
    return wpc ? wpc->total_samples : (uint32_t) -1;
}

// Get the current sample index position, or -1 if unknown

uint32_t WavpackGetSampleIndex (WavpackContext *wpc)
{
    if (wpc) {
#ifdef VER3
	if (wpc->stream3)
	    return get_sample_index3 (wpc);
	else if (wpc->streams [0])
	    return wpc->streams [0]->sample_index;
#else
	if (wpc->streams [0])
	    return wpc->streams [0]->sample_index;
#endif
    }

    return (uint32_t) -1;
}

// Get the number of errors encountered so far

int WavpackGetNumErrors (WavpackContext *wpc)
{
    return wpc ? wpc->crc_errors : 0;
}

// return TRUE if any uncorrected lossy blocks were actually written or read

int WavpackLossyBlocks (WavpackContext *wpc)
{
    return wpc ? wpc->lossy_blocks : 0;
}

// Calculate the progress through the file as a double from 0.0 (for begin)
// to 1.0 (for done). A return value of -1.0 indicates that the progress is
// unknown.

double WavpackGetProgress (WavpackContext *wpc)
{
    if (wpc && wpc->total_samples != (uint32_t) -1 && wpc->total_samples != 0)
	return (double) WavpackGetSampleIndex (wpc) / wpc->total_samples;
    else
	return -1.0;
}

// Return the total size of the WavPack file(s) in bytes.

uint32_t WavpackGetFileSize (WavpackContext *wpc)
{
    return wpc ? wpc->filelen + wpc->file2len : 0;
}

// Calculate the ratio of the specified WavPack file size to the size of the
// original audio data as a double greater than 0.0 and (usually) smaller than
// 1.0. A value greater than 1.0 represents "negative" compression and a
// return value of 0.0 indicates that the ratio cannot be determined.

double WavpackGetRatio (WavpackContext *wpc)
{
    if (wpc && wpc->total_samples != (uint32_t) -1 && wpc->filelen) {
	double output_size = (double) wpc->total_samples * wpc->config.num_channels *
	    wpc->config.bytes_per_sample;
	double input_size = (double) wpc->filelen + wpc->file2len;

	if (output_size >= 1.0 && input_size >= 1.0)
	    return input_size / output_size;
    }

    return 0.0;
}

// Calculate the average bitrate of the WavPack file in bits per second. A
// return of 0.0 indicates that the bitrate cannot be determined. An option is
// provided to use (or not use) any attendant .wvc file.

double WavpackGetAverageBitrate (WavpackContext *wpc, int count_wvc)
{
    if (wpc && wpc->total_samples != (uint32_t) -1 && wpc->filelen) {
	double output_time = (double) wpc->total_samples / wpc->config.sample_rate;
	double input_size = (double) wpc->filelen + (count_wvc ? wpc->file2len : 0);

	if (output_time >= 1.0 && input_size >= 1.0)
	    return input_size * 8.0 / output_time;
    }

    return 0.0;
}

#ifdef UNPACK

// Calculate the bitrate of the current WavPack file block in bits per second.
// This can be used for an "instant" bit display and gets updated from about
// 1 to 4 times per second. A return of 0.0 indicates that the bitrate cannot
// be determined.

double WavpackGetInstantBitrate (WavpackContext *wpc)
{
    if (wpc->stream3)
	return WavpackGetAverageBitrate (wpc, TRUE);

    if (wpc && wpc->streams [0] && wpc->streams [0]->wphdr.block_samples) {
	double output_time = (double) wpc->streams [0]->wphdr.block_samples / wpc->config.sample_rate;
	double input_size = 0;
	int si;

	for (si = 0; si < wpc->num_streams; ++si) {
	    if (wpc->streams [si]->blockbuff)
		input_size += ((WavpackHeader *) wpc->streams [si]->blockbuff)->ckSize;

	    if (wpc->streams [si]->block2buff)
		input_size += ((WavpackHeader *) wpc->streams [si]->block2buff)->ckSize;
	}

	if (output_time > 0.0 && input_size >= 1.0)
	    return input_size * 8.0 / output_time;
    }

    return 0.0;
}

#endif

// Close the specified WavPack file and release all resources used by it.
// Returns NULL.

WavpackContext *WavpackCloseFile (WavpackContext *wpc)
{
    free_streams (wpc);

    if (wpc->streams [0])
	free (wpc->streams [0]);

#ifdef VER3
    if (wpc->stream3)
	free_stream3 (wpc);
#endif

#if defined(UNPACK) || defined(INFO_ONLY)
    if (wpc->close_files) {
#ifdef USE_FSTREAMS
	if (wpc->wv_in != NULL)
	    fclose (wpc->wv_in);

	if (wpc->wvc_in != NULL)
	    fclose (wpc->wvc_in);
#endif
    }

    WavpackFreeWrapper (wpc);
#endif

#ifdef TAGS
    free_tag (&wpc->m_tag);
#endif

    free (wpc);

    return NULL;
}

// Returns the sample rate of the specified WavPack file

uint32_t WavpackGetSampleRate (WavpackContext *wpc)
{
    return wpc ? wpc->config.sample_rate : 44100;
}

// Returns the number of channels of the specified WavPack file. Note that
// this is the actual number of channels contained in the file even if the
// OPEN_2CH_MAX flag was specified when the file was opened.

int WavpackGetNumChannels (WavpackContext *wpc)
{
    return wpc ? wpc->config.num_channels : 2;
}

int WavpackGetFloatNormExp (WavpackContext *wpc)
{
    return wpc->config.float_norm_exp;
}

// Returns the actual number of valid bits per sample contained in the
// original file, which may or may not be a multiple of 8. Floating data
// always has 32 bits, integers may be from 1 to 32 bits each. When this
// value is not a multiple of 8, then the "extra" bits are located in the
// LSBs of the results. That is, values are right justified when unpacked
// into ints, but are left justified in the number of bytes used by the
// original data.

int WavpackGetBitsPerSample (WavpackContext *wpc)
{
    return wpc ? wpc->config.bits_per_sample : 16;
}

// Returns the number of bytes used for each sample (1 to 4) in the original
// file. This is required information for the user of this module because the
// audio data is returned in the LOWER bytes of the long buffer and must be
// left-shifted 8, 16, or 24 bits if normalized longs are required.

int WavpackGetBytesPerSample (WavpackContext *wpc)
{
    return wpc ? wpc->config.bytes_per_sample : 2;
}

#if defined(UNPACK) || defined(INFO_ONLY)

// If the OPEN_2CH_MAX flag is specified when opening the file, this function
// will return the actual number of channels decoded from the file (which may
// or may not be less than the actual number of channels, but will always be
// 1 or 2). Normally, this will be the front left and right channels of a
// multichannel file.

int WavpackGetReducedChannels (WavpackContext *wpc)
{
    if (wpc)
	return wpc->reduced_channels ? wpc->reduced_channels : wpc->config.num_channels;
    else
	return 2;
}

// These routines are used to access (and free) header and trailer data that
// was retrieved from the Wavpack file. The header will be available before
// the samples are decoded and the trailer will be available after all samples
// have been read.

uint32_t WavpackGetWrapperBytes (WavpackContext *wpc)
{
    return wpc ? wpc->wrapper_bytes : 0;
}

uchar *WavpackGetWrapperData (WavpackContext *wpc)
{
    return wpc ? wpc->wrapper_data : NULL;
}

void WavpackFreeWrapper (WavpackContext *wpc)
{
    if (wpc && wpc->wrapper_data) {
	free (wpc->wrapper_data);
	wpc->wrapper_data = NULL;
	wpc->wrapper_bytes = 0;
    }
}

// Get any MD5 checksum stored in the metadata (should be called after reading
// last sample or an extra seek will occur). A return value of FALSE indicates
// that no MD5 checksum was stored.

static int seek_md5 (stream_reader *reader, void *id, uchar data [16]);

int WavpackGetMD5Sum (WavpackContext *wpc, uchar data [16])
{
    if (wpc->config.flags & CONFIG_MD5_CHECKSUM) {
	if (wpc->config.md5_read) {
	    memcpy (data, wpc->config.md5_checksum, 16);
	    return TRUE;
	}
	else if (wpc->reader->can_seek (wpc->wv_in)) {
	    uint32_t pos_save = wpc->reader->get_pos (wpc->wv_in);

	    wpc->config.md5_read = seek_md5 (wpc->reader, wpc->wv_in, wpc->config.md5_checksum);
	    wpc->reader->set_pos_abs (wpc->wv_in, pos_save);

	    if (wpc->config.md5_read) {
		memcpy (data, wpc->config.md5_checksum, 16);
		return TRUE;
	    }
	    else
		return FALSE;
	}
    }

    return FALSE;
}

#endif

// Free all memory allocated for raw WavPack blocks (for all allocated streams)
// and free all additonal streams. This does not free the default stream ([0])
// which is always kept around.

static void free_streams (WavpackContext *wpc)
{
    int si = wpc->num_streams;

    while (si--) {
	if (wpc->streams [si]->blockbuff) {
	    free (wpc->streams [si]->blockbuff);
	    wpc->streams [si]->blockbuff = NULL;
	}

	if (wpc->streams [si]->block2buff) {
	    free (wpc->streams [si]->block2buff);
	    wpc->streams [si]->block2buff = NULL;
	}

	if (wpc->streams [si]->sample_buffer) {
	    free (wpc->streams [si]->sample_buffer);
	    wpc->streams [si]->sample_buffer = NULL;
	}

	if (si) {
	    wpc->num_streams--;
	    free (wpc->streams [si]);
	    wpc->streams [si] = NULL;
	}
    }

    wpc->current_stream = 0;
}

#ifdef TAGS

// Return TRUE is a valid ID3v1 or APEv2 tag has been loaded.

static int valid_tag (M_Tag *m_tag)
{
    if (m_tag->ape_tag_hdr.ID [0] == 'A')
	return 'A';
    else if (m_tag->id3_tag.tag_id [0] == 'T')
	return 'T';
    else
	return 0;
}

// Free the data for any APEv2 tag that was allocated.

static void free_tag (M_Tag *m_tag)
{
    if (m_tag->ape_tag_data) {
	free (m_tag->ape_tag_data);
	m_tag->ape_tag_data = 0;
    }
}

#endif

#if defined(UNPACK) || defined(INFO_ONLY)

// Read from current file position until a valid 32-byte WavPack 4.0 header is
// found and read into the specified pointer. The number of bytes skipped is
// returned. If no WavPack header is found within 1 meg, then a -1 is returned
// to indicate the error. No additional bytes are read past the header and it
// is returned in the processor's native endian mode. Seeking is not required.

static uint32_t read_next_header (stream_reader *reader, void *id, WavpackHeader *wphdr)
{
    char buffer [sizeof (*wphdr)], *sp = buffer + sizeof (*wphdr), *ep = sp;
    uint32_t bytes_skipped = 0;
    int bleft;

    while (1) {
	if (sp < ep) {
	    bleft = ep - sp;
	    memcpy (buffer, sp, bleft);
	}
	else
	    bleft = 0;

	if (reader->read_bytes (id, buffer + bleft, sizeof (*wphdr) - bleft) != sizeof (*wphdr) - bleft)
	    return -1;

	sp = buffer;

	if (*sp++ == 'w' && *sp == 'v' && *++sp == 'p' && *++sp == 'k' &&
	    !(*++sp & 1) && sp [2] < 16 && !sp [3] && sp [5] == 4 && sp [4] >= 2 && sp [4] <= 0xf) {
		memcpy (wphdr, buffer, sizeof (*wphdr));
		little_endian_to_native (wphdr, WavpackHeaderFormat);
		return bytes_skipped;
	    }

	while (sp < ep && *sp != 'w')
	    sp++;

	if ((bytes_skipped += sp - buffer) > 1024 * 1024)
	    return -1;
    }
}

// This function is used to seek to end of a file to determine its actual
// length in samples by reading the last header block containing data.
// Currently, all WavPack files contain the sample length in the first block
// containing samples, however this might not always be the case. Obviously,
// this function requires a seekable file or stream and leaves the file
// pointer undefined. A return value of -1 indicates the length could not
// be determined.

static uint32_t seek_final_index (stream_reader *reader, void *id)
{
    uint32_t result = (uint32_t) -1, bcount;
    WavpackHeader wphdr;

    if (reader->get_length (id) > 1200000L)
	reader->set_pos_rel (id, -1048576L, SEEK_END);

    while (1) {
	bcount = read_next_header (reader, id, &wphdr);

	if (bcount == (uint32_t) -1)
	    return result;

	if (wphdr.block_samples && (wphdr.flags & FINAL_BLOCK))
	    result = wphdr.block_index + wphdr.block_samples;

	if (wphdr.ckSize > sizeof (WavpackHeader) - 8)
	    reader->set_pos_rel (id, wphdr.ckSize - sizeof (WavpackHeader) + 8, SEEK_CUR);
    }
}

static int seek_md5 (stream_reader *reader, void *id, uchar data [16])
{
    uchar meta_id, c1, c2;
    uint32_t bcount, meta_bc;
    WavpackHeader wphdr;

    if (reader->get_length (id) > 1200000L)
	reader->set_pos_rel (id, -1048576L, SEEK_END);

    while (1) {
	bcount = read_next_header (reader, id, &wphdr);

	if (bcount == (uint32_t) -1)
	    return FALSE;

	bcount = wphdr.ckSize - sizeof (WavpackHeader) + 8;

	while (bcount >= 2) {
	    if (reader->read_bytes (id, &meta_id, 1) != 1 ||
		reader->read_bytes (id, &c1, 1) != 1)
		    return FALSE;

	    meta_bc = c1 << 1;
	    bcount -= 2;

	    if (meta_id & ID_LARGE) {
		if (bcount < 2 || reader->read_bytes (id, &c1, 1) != 1 ||
		    reader->read_bytes (id, &c2, 1) != 1)
			return FALSE;

		meta_bc += ((uint32_t) c1 << 9) + ((uint32_t) c2 << 17);
		bcount -= 2;
	    }

	    if (meta_id == ID_MD5_CHECKSUM)
		return (meta_bc == 16 && bcount >= 16 &&
		    reader->read_bytes (id, data, 16) == 16);

	    reader->set_pos_rel (id, meta_bc, SEEK_CUR);
	    bcount -= meta_bc;
	}
    }
}

#ifdef SEEKING

// Find a valid WavPack header, searching either from the current file position
// (or from the specified position if not -1) and store it (endian corrected)
// at the specified pointer. The return value is the exact file position of the
// header, although we may have actually read past it. Because this function
// is used for seeking to a specific audio sample, it only considers blocks
// that contain audio samples for the initial stream to be valid.

#define BUFSIZE 4096

static uint32_t find_header (stream_reader *reader, void *id, uint32_t filepos, WavpackHeader *wphdr)
{
    char *buffer = malloc (BUFSIZE), *sp = buffer, *ep = buffer;

    if (filepos != (uint32_t) -1 && reader->set_pos_abs (id, filepos)) {
	free (buffer);
	return -1;
    }

    while (1) {
	int bleft;

	if (sp < ep) {
	    bleft = ep - sp;
	    memcpy (buffer, sp, bleft);
	    ep -= (sp - buffer);
	    sp = buffer;
	}
	else {
	    if (sp > ep)
		if (reader->set_pos_rel (id, sp - ep, SEEK_CUR)) {
		    free (buffer);
		    return -1;
		}

	    sp = ep = buffer;
	    bleft = 0;
	}

	ep += reader->read_bytes (id, ep, BUFSIZE - bleft);

	if (ep - sp < 32) {
	    free (buffer);
	    return -1;
	}

	while (sp + 32 <= ep)
	    if (*sp++ == 'w' && *sp == 'v' && *++sp == 'p' && *++sp == 'k' &&
		!(*++sp & 1) && sp [2] < 16 && !sp [3] && sp [5] == 4 && sp [4] >= 2 && sp [4] <= 0xf) {
		    memcpy (wphdr, sp - 4, sizeof (*wphdr));
		    little_endian_to_native (wphdr, WavpackHeaderFormat);

		    if (wphdr->block_samples && (wphdr->flags & INITIAL_BLOCK)) {
			free (buffer);
			return reader->get_pos (id) - (ep - sp + 4);
		    }

		    if (wphdr->ckSize > 1024)
			sp += wphdr->ckSize - 1024;
	    }
    }
}

// Find the WavPack block that contains the specified sample. If "header_pos"
// is zero, then no information is assumed except the total number of samples
// in the file and its size in bytes. If "header_pos" is non-zero then we
// assume that it is the file position of the valid header image contained in
// the first stream and we can limit our search to either the portion above
// or below that point. If a .wvc file is being used, then this must be called
// for that file also.

static uint32_t find_sample (WavpackContext *wpc, void *infile, uint32_t header_pos, uint32_t sample)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    uint32_t file_pos1 = 0, file_pos2 = wpc->reader->get_length (infile);
    uint32_t sample_pos1 = 0, sample_pos2 = wpc->total_samples;
    double ratio = 0.96;
    int file_skip = 0;

    if (sample >= wpc->total_samples)
	return -1;

    if (header_pos) {
	if (wps->wphdr.block_index > sample) {
	    sample_pos2 = wps->wphdr.block_index;
	    file_pos2 = header_pos;
	}
	else if (wps->wphdr.block_index + wps->wphdr.block_samples <= sample) {
	    sample_pos1 = wps->wphdr.block_index;
	    file_pos1 = header_pos;
	}
	else
	    return header_pos;
    }

    while (1) {
	double bytes_per_sample;
	uint32_t seek_pos;

	bytes_per_sample = file_pos2 - file_pos1;
	bytes_per_sample /= sample_pos2 - sample_pos1;
	seek_pos = file_pos1 + (file_skip ? 32 : 0);
	seek_pos += (uint32_t)(bytes_per_sample * (sample - sample_pos1) * ratio);
	seek_pos = find_header (wpc->reader, infile, seek_pos, &wps->wphdr);

	if (seek_pos == (uint32_t) -1 || seek_pos >= file_pos2) {
	    if (ratio > 0.0) {
		if ((ratio -= 0.24) < 0.0)
		    ratio = 0.0;
	    }
	    else
		return -1;
	}
	else if (wps->wphdr.block_index > sample) {
	    sample_pos2 = wps->wphdr.block_index;
	    file_pos2 = seek_pos;
	}
	else if (wps->wphdr.block_index + wps->wphdr.block_samples <= sample) {

	    if (seek_pos == file_pos1)
		file_skip = 1;
	    else {
		sample_pos1 = wps->wphdr.block_index;
		file_pos1 = seek_pos;
	    }
	}
	else
	    return seek_pos;
    }
}

#endif

#ifdef TAGS

// This function attempts to load an ID3v1 or APEv2 tag from the specified
// file into the specified M_Tag structure. The ID3 tag fits in completely,
// but an APEv2 tag is variable length and so space must be allocated here
// to accomodate the data, and this will need to be freed later. A return
// value of TRUE indicates a valid tag was found and loaded. Note that the
// file pointer is undefined when this function exits.

static int load_tag (WavpackContext *wpc)
{
    M_Tag *m_tag = &wpc->m_tag;

    CLEAR (*m_tag);

    // First, attempt to find an APEv2 tag...

    wpc->reader->set_pos_rel (wpc->wv_in, -sizeof (APE_Tag_Hdr), SEEK_END);

    if (wpc->reader->read_bytes (wpc->wv_in, &m_tag->ape_tag_hdr, sizeof (APE_Tag_Hdr)) == sizeof (APE_Tag_Hdr) &&
	!strncmp (m_tag->ape_tag_hdr.ID, "APETAGEX", 8)) {

	    little_endian_to_native (&m_tag->ape_tag_hdr, APE_Tag_Hdr_Format);

	    if (m_tag->ape_tag_hdr.version == 2000 && m_tag->ape_tag_hdr.item_count &&
		m_tag->ape_tag_hdr.length > sizeof (m_tag->ape_tag_hdr) &&
		m_tag->ape_tag_hdr.length < (1024 * 1024) &&
		(m_tag->ape_tag_data = malloc (m_tag->ape_tag_hdr.length)) != NULL) {

		    memset (m_tag->ape_tag_data, 0, m_tag->ape_tag_hdr.length);
		    wpc->reader->set_pos_rel (wpc->wv_in, -m_tag->ape_tag_hdr.length, SEEK_END);

		    if (wpc->reader->read_bytes (wpc->wv_in, m_tag->ape_tag_data, m_tag->ape_tag_hdr.length - sizeof (APE_Tag_Hdr)) !=
			m_tag->ape_tag_hdr.length - sizeof (APE_Tag_Hdr)) {
			    free (m_tag->ape_tag_data);
			    CLEAR (*m_tag);
			    return FALSE;
		    }
		    else
			return TRUE;
	    }
    }

    // ...if not, try a ID3v1 tag

    wpc->reader->set_pos_rel (wpc->wv_in, -sizeof (ID3_Tag), SEEK_END);

    if (wpc->reader->read_bytes (wpc->wv_in, &m_tag->id3_tag, sizeof (ID3_Tag)) == sizeof (ID3_Tag) &&
	!strncmp (m_tag->id3_tag.tag_id, "TAG", 3))
	    return TRUE;
    else {
	CLEAR (*m_tag);
	return FALSE;
    }
}

// Copy the specified ID3v1 tag value (with specified field size) from the
// source pointer to the destination, eliminating leading spaces and trailing
// spaces and nulls.

static void tagcpy (char *dest, char *src, int tag_size)
{
    char *s1 = src, *s2 = src + tag_size - 1;

    while (s1 <= s2)
	if (*s1 == ' ')
	    ++s1;
	else if (!*s2 || *s2 == ' ')
	    --s2;
	else
	    break;

    while (*s1 && s1 <= s2)
	*dest++ = *s1++;

    *dest = 0;
}

#endif

#endif
