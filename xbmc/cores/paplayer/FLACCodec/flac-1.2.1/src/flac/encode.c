/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined _WIN32 && !defined __CYGWIN__
/* where MSVC puts unlink() */
# include <io.h>
#else
# include <unistd.h>
#endif
#if defined _MSC_VER || defined __MINGW32__
#include <sys/types.h> /* for off_t */
#if _MSC_VER <= 1600 /* @@@ [2G limit] */
#define fseeko fseek
#define ftello ftell
#endif
#endif
#include <errno.h>
#include <limits.h> /* for LONG_MAX */
#include <math.h> /* for floor() */
#include <stdio.h> /* for FILE etc. */
#include <stdlib.h> /* for malloc */
#include <string.h> /* for strcmp(), strerror() */
#include "FLAC/all.h"
#include "share/alloc.h"
#include "share/grabbag.h"
#include "encode.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))
#ifdef max
#undef max
#endif
#define max(x,y) ((x)>(y)?(x):(y))

/* this MUST be >= 588 so that sector aligning can take place with one read */
#define CHUNK_OF_SAMPLES 2048

typedef struct {
#if FLAC__HAS_OGG
	FLAC__bool use_ogg;
#endif
	FLAC__bool verify;
	FLAC__bool is_stdout;
	FLAC__bool outputfile_opened; /* true if we successfully opened the output file and we want it to be deleted if there is an error */
	const char *inbasefilename;
	const char *infilename;
	const char *outfilename;

	FLAC__uint64 skip;
	FLAC__uint64 until; /* a value of 0 mean end-of-stream (i.e. --until=-0) */
	FLAC__bool treat_warnings_as_errors;
	FLAC__bool continue_through_decode_errors;
	FLAC__bool replay_gain;
	unsigned channels;
	unsigned bits_per_sample;
	unsigned sample_rate;
	FLAC__uint64 unencoded_size;
	FLAC__uint64 total_samples_to_encode;
	FLAC__uint64 bytes_written;
	FLAC__uint64 samples_written;
	unsigned stats_mask;

	FLAC__StreamEncoder *encoder;

	FILE *fin;
	FLAC__StreamMetadata *seek_table_template;
} EncoderSession;

/* this is data attached to the FLAC decoder when encoding from a FLAC file */
typedef struct {
	EncoderSession *encoder_session;
	off_t filesize;
	const FLAC__byte *lookahead;
	unsigned lookahead_length;
	size_t num_metadata_blocks;
	FLAC__StreamMetadata *metadata_blocks[1024]; /*@@@ BAD MAGIC number */
	FLAC__uint64 samples_left_to_process;
	FLAC__bool fatal_error;
} FLACDecoderData;

const int FLAC_ENCODE__DEFAULT_PADDING = 8192;

static FLAC__bool is_big_endian_host_;

static unsigned char ucbuffer_[CHUNK_OF_SAMPLES*FLAC__MAX_CHANNELS*((FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE+7)/8)];
static signed char *scbuffer_ = (signed char *)ucbuffer_;
static FLAC__uint16 *usbuffer_ = (FLAC__uint16 *)ucbuffer_;
static FLAC__int16 *ssbuffer_ = (FLAC__int16 *)ucbuffer_;

static FLAC__int32 in_[FLAC__MAX_CHANNELS][CHUNK_OF_SAMPLES];
static FLAC__int32 *input_[FLAC__MAX_CHANNELS];


/*
 * unpublished debug routines from the FLAC libs
 */
extern FLAC__bool FLAC__stream_encoder_disable_constant_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_fixed_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_verbatim_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_set_do_md5(FLAC__StreamEncoder *encoder, FLAC__bool value);

/*
 * local routines
 */
static FLAC__bool EncoderSession_construct(EncoderSession *e, FLAC__bool use_ogg, FLAC__bool verify, FLAC__bool treat_warnings_as_errors, FLAC__bool continue_through_decode_errors, FILE *infile, const char *infilename, const char *outfilename);
static void EncoderSession_destroy(EncoderSession *e);
static int EncoderSession_finish_ok(EncoderSession *e, int info_align_carry, int info_align_zero, foreign_metadata_t *foreign_metadata);
static int EncoderSession_finish_error(EncoderSession *e);
static FLAC__bool EncoderSession_init_encoder(EncoderSession *e, encode_options_t options, FLAC__uint32 channel_mask, unsigned channels, unsigned bps, unsigned sample_rate, const foreign_metadata_t *foreign_metadata, FLACDecoderData *flac_decoder_data);
static FLAC__bool EncoderSession_process(EncoderSession *e, const FLAC__int32 * const buffer[], unsigned samples);
static FLAC__bool convert_to_seek_table_template(const char *requested_seek_points, int num_requested_seek_points, FLAC__StreamMetadata *cuesheet, EncoderSession *e);
static FLAC__bool canonicalize_until_specification(utils__SkipUntilSpecification *spec, const char *inbasefilename, unsigned sample_rate, FLAC__uint64 skip, FLAC__uint64 total_samples_in_input);
static FLAC__bool verify_metadata(const EncoderSession *e, FLAC__StreamMetadata **metadata, unsigned num_metadata);
static FLAC__bool format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps, unsigned shift, size_t *channel_map);
static void encoder_progress_callback(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);
static FLAC__StreamDecoderReadStatus flac_decoder_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderSeekStatus flac_decoder_seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus flac_decoder_tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus flac_decoder_length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool flac_decoder_eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus flac_decoder_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void flac_decoder_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void flac_decoder_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static FLAC__bool parse_cuesheet(FLAC__StreamMetadata **cuesheet, const char *cuesheet_filename, const char *inbasefilename, FLAC__bool is_cdda, FLAC__uint64 lead_out_offset, FLAC__bool treat_warnings_as_errors);
static void print_stats(const EncoderSession *encoder_session);
static void print_error_with_init_status(const EncoderSession *e, const char *message, FLAC__StreamEncoderInitStatus init_status);
static void print_error_with_state(const EncoderSession *e, const char *message);
static void print_verify_error(EncoderSession *e);
static FLAC__bool read_little_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_little_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_big_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_big_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_sane_extended(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool fskip_ahead(FILE *f, FLAC__uint64 offset);
static unsigned count_channel_mask_bits(FLAC__uint32 mask);
#if 0
static FLAC__uint32 limit_channel_mask(FLAC__uint32 mask, unsigned channels);
#endif

/*
 * public routines
 */
int flac__encode_aif(FILE *infile, off_t infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, wav_encode_options_t options, FLAC__bool is_aifc)
{
	EncoderSession encoder_session;
	FLAC__uint16 x;
	FLAC__uint32 xx;
	unsigned int channels= 0U, bps= 0U, shift= 0U, sample_rate= 0U, sample_frames= 0U;
	size_t channel_map[FLAC__MAX_CHANNELS];
	FLAC__bool got_comm_chunk= false, got_ssnd_chunk= false;
	int info_align_carry= -1, info_align_zero= -1;
	FLAC__bool is_big_endian_pcm = true;

	(void)infilesize; /* silence compiler warning about unused parameter */
	(void)lookahead; /* silence compiler warning about unused parameter */
	(void)lookahead_length; /* silence compiler warning about unused parameter */

	if(!
		EncoderSession_construct(
			&encoder_session,
#if FLAC__HAS_OGG
			options.common.use_ogg,
#else
			/*use_ogg=*/false,
#endif
			options.common.verify,
			options.common.treat_warnings_as_errors,
			options.common.continue_through_decode_errors,
			infile,
			infilename,
			outfilename
		)
	)
		return 1;

	/* initialize default channel map that preserves channel order */
	{
		size_t i;
		for(i = 0; i < sizeof(channel_map)/sizeof(channel_map[0]); i++)
			channel_map[i] = i;
	}

	if(options.foreign_metadata) {
		const char *error;
		if(!flac__foreign_metadata_read_from_aiff(options.foreign_metadata, infilename, &error)) {
			flac__utils_printf(stderr, 1, "%s: ERROR reading foreign metadata: %s\n", encoder_session.inbasefilename, error);
			return EncoderSession_finish_error(&encoder_session);
		}
	}

	/* lookahead[] already has "FORMxxxxAIFF", do sub-chunks */

	while(1) {
		size_t c= 0U;
		char chunk_id[5] = { '\0', '\0', '\0', '\0', '\0' }; /* one extra byte for terminating NUL so we can also treat it like a C string */

		/* chunk identifier; really conservative about behavior of fread() and feof() */
		if(feof(infile) || ((c= fread(chunk_id, 1U, 4U, infile)), c==0U && feof(infile)))
			break;
		else if(c<4U || feof(infile)) {
			flac__utils_printf(stderr, 1, "%s: ERROR: incomplete chunk identifier\n", encoder_session.inbasefilename);
			return EncoderSession_finish_error(&encoder_session);
		}

		if(got_comm_chunk==false && !memcmp(chunk_id, "COMM", 4)) { /* common chunk */
			unsigned long skip;
			const FLAC__uint32 minimum_comm_size = (is_aifc? 22 : 18);

			/* COMM chunk size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(xx<minimum_comm_size) {
				flac__utils_printf(stderr, 1, "%s: ERROR: non-standard %s 'COMM' chunk has length = %u\n", encoder_session.inbasefilename, is_aifc? "AIFF-C" : "AIFF", (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(!is_aifc && xx!=minimum_comm_size) {
				flac__utils_printf(stderr, 1, "%s: WARNING: non-standard %s 'COMM' chunk has length = %u, expected %u\n", encoder_session.inbasefilename, is_aifc? "AIFF-C" : "AIFF", (unsigned int)xx, minimum_comm_size);
				if(encoder_session.treat_warnings_as_errors)
					return EncoderSession_finish_error(&encoder_session);
			}
			skip= (xx-minimum_comm_size)+(xx & 1U);

			/* number of channels */
			if(!read_big_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(x==0U || x>FLAC__MAX_CHANNELS) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported number channels %u\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(x>2U && !options.common.channel_map_none) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported number channels %u for AIFF\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && x!=2U) {
				flac__utils_printf(stderr, 1, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			channels= x;

			/* number of sample frames */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			sample_frames= xx;

			/* bits per sample */
			if(!read_big_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(x<4U || x>24U) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported bits-per-sample %u\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && x!=16U) {
				flac__utils_printf(stderr, 1, "%s: ERROR: file has %u bits-per-sample, must be 16 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			bps= x;
			shift= (bps%8)? 8-(bps%8) : 0; /* SSND data is always byte-aligned, left-justified but format_input() will double-check */
			bps+= shift;

			/* sample rate */
			if(!read_sane_extended(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(!FLAC__format_sample_rate_is_valid(xx)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported sample rate %u\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && xx!=44100U) {
				flac__utils_printf(stderr, 1, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			sample_rate= xx;

			/* check compression type for AIFF-C */
			if(is_aifc) {
				if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
					return EncoderSession_finish_error(&encoder_session);
				if(xx == 0x736F7774) /* "sowt" */
					is_big_endian_pcm = false;
				else if(xx == 0x4E4F4E45) /* "NONE" */
					; /* nothing to do, we already default to big-endian */
				else {
					flac__utils_printf(stderr, 1, "%s: ERROR: can't handle AIFF-C compression type \"%c%c%c%c\"\n", encoder_session.inbasefilename, (char)(xx>>24), (char)((xx>>16)&8), (char)((xx>>8)&8), (char)(xx&8));
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			/* set channel mapping */
			/* FLAC order follows SMPTE and WAVEFORMATEXTENSIBLE but with fewer channels, which are: */
			/* front left, front right, center, LFE, back left, back right, surround left, surround right */
			/* specs say the channel ordering is:
			 *                             1     2   3   4   5   6
			 * ___________________________________________________
			 * 2         stereo            l     r
			 * 3                           l     r   c
			 * 4                           l     c   r   S
			 * quad (ambiguous with 4ch)  Fl    Fr   Bl  Br
			 * 5                          Fl     Fr  Fc  Sl  Sr
			 * 6                           l     lc  c   r   rc  S
			 * l:left r:right c:center Fl:front-left Fr:front-right Bl:back-left Br:back-right Lc:left-center Rc:right-center S:surround
			 * so we only have unambiguous mappings for 2, 3, and 5 channels
			 */
			if(
				options.common.channel_map_none ||
				channels == 1 || /* 1 channel: (mono) */
				channels == 2 || /* 2 channels: left, right */
				channels == 3 || /* 3 channels: left, right, center */
				channels == 5    /* 5 channels: front left, front right, center, surround left, surround right */
			) {
				/* keep default channel order */
			}
			else {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported number channels %u for AIFF\n", encoder_session.inbasefilename, channels);
				return EncoderSession_finish_error(&encoder_session);
			}

			/* skip any extra data in the COMM chunk */
			if(!fskip_ahead(infile, skip)) {
				flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping over extra COMM data\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}

			/*
			 * now that we know the sample rate, canonicalize the
			 * --skip string to a number of samples:
			 */
			flac__utils_canonicalize_skip_until_specification(&options.common.skip_specification, sample_rate);
			FLAC__ASSERT(options.common.skip_specification.value.samples >= 0);
			encoder_session.skip = (FLAC__uint64)options.common.skip_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.skip == 0);

			got_comm_chunk= true;
		}
		else if(got_ssnd_chunk==false && !memcmp(chunk_id, "SSND", 4)) { /* sound data chunk */
			unsigned int offset= 0U, block_size= 0U, align_remainder= 0U, data_bytes;
			const size_t bytes_per_frame= channels*(bps>>3);
			FLAC__uint64 total_samples_in_input, trim = 0;
			FLAC__bool pad= false;

			if(got_comm_chunk==false) {
				flac__utils_printf(stderr, 1, "%s: ERROR: got 'SSND' chunk before 'COMM' chunk\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}

			/* SSND chunk size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(options.common.ignore_chunk_sizes) {
				FLAC__ASSERT(!options.common.sector_align);
				data_bytes = (unsigned)(-(int)bytes_per_frame); /* max out data_bytes; we'll use EOF as signal to stop reading */
			}
			else {
				data_bytes= xx;
				data_bytes-= 8U; /* discount the offset and block size fields */
			}
			pad= (data_bytes & 1U) ? true : false;

			/* offset */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			offset= xx;
			data_bytes-= offset;

			/* block size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(xx!=0U) {
				flac__utils_printf(stderr, 1, "%s: ERROR: block size is %u; must be 0\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			block_size= xx;

			/* skip any SSND offset bytes */
			FLAC__ASSERT(offset<=LONG_MAX);
			if(!fskip_ahead(infile, offset)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: skipping offset in SSND chunk\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}
			if(data_bytes!=(sample_frames*bytes_per_frame)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: SSND chunk size inconsistent with sample frame count\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}

			/* *options.common.align_reservoir_samples will be 0 unless --sector-align is used */
			FLAC__ASSERT(options.common.sector_align || *options.common.align_reservoir_samples == 0);
			total_samples_in_input = data_bytes / bytes_per_frame + *options.common.align_reservoir_samples;

			/*
			 * now that we know the input size, canonicalize the
			 * --until string to an absolute sample number:
			 */
			if(!canonicalize_until_specification(&options.common.until_specification, encoder_session.inbasefilename, sample_rate, encoder_session.skip, total_samples_in_input))
				return EncoderSession_finish_error(&encoder_session);
			encoder_session.until = (FLAC__uint64)options.common.until_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.until == 0);

			if(encoder_session.skip>0U) {
				if(!fskip_ahead(infile, encoder_session.skip*bytes_per_frame)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			data_bytes-= (unsigned int)encoder_session.skip*bytes_per_frame; /*@@@ WATCHOUT: 4GB limit */
			if(options.common.ignore_chunk_sizes) {
				encoder_session.total_samples_to_encode= 0;
				flac__utils_printf(stderr, 2, "(No runtime statistics possible; please wait for encoding to finish...)\n");
				FLAC__ASSERT(0 == encoder_session.until);
			}
			else {
				encoder_session.total_samples_to_encode= total_samples_in_input - encoder_session.skip;
			}
			if(encoder_session.until > 0) {
				trim = total_samples_in_input - encoder_session.until;
				FLAC__ASSERT(total_samples_in_input > 0);
				FLAC__ASSERT(!options.common.sector_align);
				data_bytes-= (unsigned int)trim*bytes_per_frame;
				encoder_session.total_samples_to_encode-= trim;
			}
			if(options.common.sector_align) {
				align_remainder= (unsigned int)(encoder_session.total_samples_to_encode % 588U);
				if(options.common.is_last_file)
					encoder_session.total_samples_to_encode+= (588U-align_remainder); /* will pad with zeroes */
				else
					encoder_session.total_samples_to_encode-= align_remainder; /* will stop short and carry over to next file */
			}

			/* +54 for the size of the AIFF headers; this is just an estimate for the progress indicator and doesn't need to be exact */
			encoder_session.unencoded_size= encoder_session.total_samples_to_encode*bytes_per_frame+54;

			if(!EncoderSession_init_encoder(&encoder_session, options.common, /*channel_mask=*/0, channels, bps-shift, sample_rate, options.foreign_metadata, /*flac_decoder_data=*/0))
				return EncoderSession_finish_error(&encoder_session);

			/* first do any samples in the reservoir */
			if(options.common.sector_align && *options.common.align_reservoir_samples>0U) {

				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 *const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			/* decrement the data_bytes counter if we need to align the file */
			if(options.common.sector_align) {
				if(options.common.is_last_file)
					*options.common.align_reservoir_samples= 0U;
				else {
					*options.common.align_reservoir_samples= align_remainder;
					data_bytes-= (*options.common.align_reservoir_samples)*bytes_per_frame;
				}
			}

			/* now do from the file */
			while(data_bytes>0) {
				size_t bytes_read= fread(ucbuffer_, 1U, min(data_bytes, CHUNK_OF_SAMPLES*bytes_per_frame), infile);

				if(bytes_read==0U) {
					if(ferror(infile)) {
						flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else if(feof(infile)) {
						if(options.common.ignore_chunk_sizes) {
							flac__utils_printf(stderr, 1, "%s: INFO: hit EOF with --ignore-chunk-sizes, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.samples_written);
						}
						else {
							flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
							if(encoder_session.treat_warnings_as_errors)
								return EncoderSession_finish_error(&encoder_session);
						}
						data_bytes= 0;
					}
				}
				else {
					if(bytes_read % bytes_per_frame != 0U) {
						flac__utils_printf(stderr, 1, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else {
						unsigned int frames= bytes_read/bytes_per_frame;
						if(!format_input(input_, frames, is_big_endian_pcm, /*is_unsigned_samples=*/false, channels, bps, shift, channel_map))
							return EncoderSession_finish_error(&encoder_session);

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 *const *)input_, frames)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
						else
							data_bytes-= bytes_read;
					}
				}
			}

			if(trim>0) {
				FLAC__ASSERT(!options.common.sector_align);
				if(!fskip_ahead(infile, trim*bytes_per_frame)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			/* now read unaligned samples into reservoir or pad with zeroes if necessary */
			if(options.common.sector_align) {
				if(options.common.is_last_file) {
					unsigned int pad_frames= 588U-align_remainder;

					if(pad_frames<588U) {
						unsigned int i;

						info_align_zero= pad_frames;
						for(i= 0U; i<channels; ++i)
							memset(input_[i], 0, sizeof(input_[0][0])*pad_frames);

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 *const *)input_, pad_frames)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
					}
				}
				else {
					if(*options.common.align_reservoir_samples > 0) {
						size_t bytes_read= fread(ucbuffer_, 1U, (*options.common.align_reservoir_samples)*bytes_per_frame, infile);

						FLAC__ASSERT(CHUNK_OF_SAMPLES>=588U);
						if(bytes_read==0U && ferror(infile)) {
							flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						else if(bytes_read != (*options.common.align_reservoir_samples) * bytes_per_frame) {
							flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; read %u bytes; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned int)bytes_read, (unsigned int)encoder_session.total_samples_to_encode, (unsigned int)encoder_session.samples_written);
							if(encoder_session.treat_warnings_as_errors)
								return EncoderSession_finish_error(&encoder_session);
						}
						else {
							info_align_carry= *options.common.align_reservoir_samples;
							if(!format_input(options.common.align_reservoir, *options.common.align_reservoir_samples, is_big_endian_pcm, /*is_unsigned_samples=*/false, channels, bps, shift, channel_map))
								return EncoderSession_finish_error(&encoder_session);
						}
					}
				}
			}

			if(pad==true) {
				unsigned char tmp;

				if(fread(&tmp, 1U, 1U, infile)<1U) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read of SSND pad byte\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			got_ssnd_chunk= true;
		}
		else { /* other chunk */
			if(!options.foreign_metadata) {
				if(!memcmp(chunk_id, "COMM", 4))
					flac__utils_printf(stderr, 1, "%s: WARNING: skipping extra 'COMM' chunk (use --keep-foreign-metadata to keep)\n", encoder_session.inbasefilename);
				else if(!memcmp(chunk_id, "SSND", 4))
					flac__utils_printf(stderr, 1, "%s: WARNING: skipping extra 'SSND' chunk (use --keep-foreign-metadata to keep)\n", encoder_session.inbasefilename);
				else if(!options.foreign_metadata)
					flac__utils_printf(stderr, 1, "%s: WARNING: skipping unknown chunk '%s' (use --keep-foreign-metadata to keep)\n", encoder_session.inbasefilename, chunk_id);
				if(encoder_session.treat_warnings_as_errors)
					return EncoderSession_finish_error(&encoder_session);
			}

			/* chunk size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else {
				unsigned long skip= xx+(xx & 1U);

				FLAC__ASSERT(skip<=LONG_MAX);
				if(!fskip_ahead(infile, skip)) {
					fprintf(stderr, "%s: ERROR during read while skipping over unknown chunk\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}
		}
	}

	if(got_ssnd_chunk==false && sample_frames!=0U) {
		flac__utils_printf(stderr, 1, "%s: ERROR: missing SSND chunk\n", encoder_session.inbasefilename);
		return EncoderSession_finish_error(&encoder_session);
	}

	return EncoderSession_finish_ok(&encoder_session, info_align_carry, info_align_zero, options.foreign_metadata);
}

int flac__encode_wav(FILE *infile, off_t infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, wav_encode_options_t options)
{
	EncoderSession encoder_session;
	FLAC__bool is_unsigned_samples = false;
	unsigned channels = 0, bps = 0, sample_rate = 0, shift = 0;
	size_t bytes_read;
	size_t channel_map[FLAC__MAX_CHANNELS];
	FLAC__uint16 x, format; /* format is the wFormatTag word from the 'fmt ' chunk */
	FLAC__uint32 xx, channel_mask = 0;
	FLAC__bool got_fmt_chunk = false, got_data_chunk = false;
	unsigned align_remainder = 0;
	int info_align_carry = -1, info_align_zero = -1;

	(void)infilesize;
	(void)lookahead;
	(void)lookahead_length;

	if(!
		EncoderSession_construct(
			&encoder_session,
#if FLAC__HAS_OGG
			options.common.use_ogg,
#else
			/*use_ogg=*/false,
#endif
			options.common.verify,
			options.common.treat_warnings_as_errors,
			options.common.continue_through_decode_errors,
			infile,
			infilename,
			outfilename
		)
	)
		return 1;

	/* initialize default channel map that preserves channel order */
	{
		size_t i;
		for(i = 0; i < sizeof(channel_map)/sizeof(channel_map[0]); i++)
			channel_map[i] = i;
	}

	if(options.foreign_metadata) {
		const char *error;
		if(!flac__foreign_metadata_read_from_wave(options.foreign_metadata, infilename, &error)) {
			flac__utils_printf(stderr, 1, "%s: ERROR reading foreign metadata: %s\n", encoder_session.inbasefilename, error);
			return EncoderSession_finish_error(&encoder_session);
		}
	}

	/*
	 * lookahead[] already has "RIFFxxxxWAVE", do sub-chunks
	 */
	while(!feof(infile)) {
		if(!read_little_endian_uint32(infile, &xx, true, encoder_session.inbasefilename))
			return EncoderSession_finish_error(&encoder_session);
		if(feof(infile))
			break;
		if(xx == 0x20746d66 && !got_fmt_chunk) { /* "fmt " */
			unsigned block_align, data_bytes;

			/* see
			 *   http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
			 *   http://windowssdk.msdn.microsoft.com/en-us/library/ms713497.aspx
			 *   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/audio_r/hh/Audio_r/aud-prop_d40f094e-44f9-4baa-8a15-03e4fb369501.xml.asp
			 *
			 * WAVEFORMAT is
			 * 4 byte: subchunk size
			 * 2 byte: format type: 1 for WAVE_FORMAT_PCM, 65534 for WAVE_FORMAT_EXTENSIBLE
			 * 2 byte: # channels
			 * 4 byte: sample rate (Hz)
			 * 4 byte: avg bytes per sec
			 * 2 byte: block align
			 * 2 byte: bits per sample (not necessarily all significant)
			 * WAVEFORMATEX adds
			 * 2 byte: extension size in bytes (usually 0 for WAVEFORMATEX and 22 for WAVEFORMATEXTENSIBLE with PCM)
			 * WAVEFORMATEXTENSIBLE adds
			 * 2 byte: valid bits per sample
			 * 4 byte: channel mask
			 * 16 byte: subformat GUID, first 2 bytes have format type, 1 being PCM
			 *
			 * Current spec says WAVEFORMATEX with PCM must have bps == 8 or 16, or any multiple of 8 for WAVEFORMATEXTENSIBLE.
			 * Lots of old broken WAVEs/apps have don't follow it, e.g. 20 bps but a block align of 3/6 for mono/stereo.
			 *
			 * Block align for WAVE_FORMAT_PCM or WAVE_FORMAT_EXTENSIBLE is also supposed to be channels*bps/8
			 *
			 * If the channel mask has more set bits than # of channels, the extra MSBs are ignored.
			 * If the channel mask has less set bits than # of channels, the extra channels are unassigned to any speaker.
			 *
			 * Data is supposed to be unsigned for bps <= 8 else signed.
			 */

			/* fmt sub-chunk size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			data_bytes = xx;
			if(data_bytes < 16) {
				flac__utils_printf(stderr, 1, "%s: ERROR: found non-standard 'fmt ' sub-chunk which has length = %u\n", encoder_session.inbasefilename, data_bytes);
				return EncoderSession_finish_error(&encoder_session);
			}
			/* format code */
			if(!read_little_endian_uint16(infile, &format, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(format != 1 /*WAVE_FORMAT_PCM*/ && format != 65534 /*WAVE_FORMAT_EXTENSIBLE*/) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported format type %u\n", encoder_session.inbasefilename, (unsigned)format);
				return EncoderSession_finish_error(&encoder_session);
			}
			/* number of channels */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			channels = (unsigned)x;
			if(channels == 0 || channels > FLAC__MAX_CHANNELS) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported number of channels %u\n", encoder_session.inbasefilename, channels);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && channels != 2) {
				flac__utils_printf(stderr, 1, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_session.inbasefilename, channels);
				return EncoderSession_finish_error(&encoder_session);
			}
			/* sample rate */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			sample_rate = xx;
			if(!FLAC__format_sample_rate_is_valid(sample_rate)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported sample rate %u\n", encoder_session.inbasefilename, sample_rate);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && sample_rate != 44100) {
				flac__utils_printf(stderr, 1, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_session.inbasefilename, sample_rate);
				return EncoderSession_finish_error(&encoder_session);
			}
			/* avg bytes per second (ignored) */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			/* block align */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			block_align = (unsigned)x;
			/* bits per sample */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			bps = (unsigned)x;
			is_unsigned_samples = (bps <= 8);
			if(format == 1) {
				if(bps != 8 && bps != 16) {
					if(bps == 24 || bps == 32) {
						/* let these slide with a warning since they're unambiguous */
						flac__utils_printf(stderr, 1, "%s: WARNING: legacy WAVE file has format type %u but bits-per-sample=%u\n", encoder_session.inbasefilename, (unsigned)format, bps);
						if(encoder_session.treat_warnings_as_errors)
							return EncoderSession_finish_error(&encoder_session);
					}
					else {
						/* @@@ we could add an option to specify left- or right-justified blocks so we knew how to set 'shift' */
						flac__utils_printf(stderr, 1, "%s: ERROR: legacy WAVE file has format type %u but bits-per-sample=%u\n", encoder_session.inbasefilename, (unsigned)format, bps);
						return EncoderSession_finish_error(&encoder_session);
					}
				}
#if 0 /* @@@ reinstate once we can get an answer about whether the samples are left- or right-justified */
				if((bps+7)/8 * channels == block_align) {
					if(bps % 8) {
						/* assume legacy file is byte aligned with some LSBs zero; this is double-checked in format_input() */
						flac__utils_printf(stderr, 1, "%s: WARNING: legacy WAVE file (format type %d) has block alignment=%u, bits-per-sample=%u, channels=%u\n", encoder_session.inbasefilename, (unsigned)format, block_align, bps, channels);
						if(encoder_session.treat_warnings_as_errors)
							return EncoderSession_finish_error(&encoder_session);
						shift = 8 - (bps % 8);
						bps += shift;
					}
					else
						shift = 0;
				}
				else {
					flac__utils_printf(stderr, 1, "%s: ERROR: illegal WAVE file (format type %d) has block alignment=%u, bits-per-sample=%u, channels=%u\n", encoder_session.inbasefilename, (unsigned)format, block_align, bps, channels);
					return EncoderSession_finish_error(&encoder_session);
				}
#else
				shift = 0;
#endif
				if(channels > 2 && !options.common.channel_map_none) {
					flac__utils_printf(stderr, 1, "%s: ERROR: WAVE has >2 channels but is not WAVE_FORMAT_EXTENSIBLE; cannot assign channels\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				FLAC__ASSERT(data_bytes >= 16);
				data_bytes -= 16;
			}
			else {
				if(data_bytes < 40) {
					flac__utils_printf(stderr, 1, "%s: ERROR: invalid WAVEFORMATEXTENSIBLE chunk with size %u\n", encoder_session.inbasefilename, data_bytes);
					return EncoderSession_finish_error(&encoder_session);
				}
				/* cbSize */
				if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
					return EncoderSession_finish_error(&encoder_session);
				if(x < 22) {
					flac__utils_printf(stderr, 1, "%s: ERROR: invalid WAVEFORMATEXTENSIBLE chunk with cbSize %u\n", encoder_session.inbasefilename, (unsigned)x);
					return EncoderSession_finish_error(&encoder_session);
				}
				/* valid bps */
				if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
					return EncoderSession_finish_error(&encoder_session);
				if((unsigned)x > bps) {
					flac__utils_printf(stderr, 1, "%s: ERROR: invalid WAVEFORMATEXTENSIBLE chunk with wValidBitsPerSample (%u) > wBitsPerSample (%u)\n", encoder_session.inbasefilename, (unsigned)x, bps);
					return EncoderSession_finish_error(&encoder_session);
				}
				shift = bps - (unsigned)x;
				/* channel mask */
				if(!read_little_endian_uint32(infile, &channel_mask, false, encoder_session.inbasefilename))
					return EncoderSession_finish_error(&encoder_session);
				/* for mono/stereo and unassigned channels, we fake the mask */
				if(channel_mask == 0) {
					if(channels == 1)
						channel_mask = 0x0001;
					else if(channels == 2)
						channel_mask = 0x0003;
				}
				/* set channel mapping */
				/* FLAC order follows SMPTE and WAVEFORMATEXTENSIBLE but with fewer channels, which are: */
				/* front left, front right, center, LFE, back left, back right, surround left, surround right */
				/* the default mapping is sufficient for 1-6 channels and 7-8 are currently unspecified anyway */
#if 0
				/* @@@ example for dolby/vorbis order, for reference later in case it becomes important */
				if(
					options.common.channel_map_none ||
					channel_mask == 0x0001 || /* 1 channel: (mono) */
					channel_mask == 0x0003 || /* 2 channels: front left, front right */
					channel_mask == 0x0033 || /* 4 channels: front left, front right, back left, back right */
					channel_mask == 0x0603    /* 4 channels: front left, front right, side left, side right */
				) {
					/* keep default channel order */
				}
				else if(
					channel_mask == 0x0007 || /* 3 channels: front left, front right, front center */
					channel_mask == 0x0037 || /* 5 channels: front left, front right, front center, back left, back right */
					channel_mask == 0x0607    /* 5 channels: front left, front right, front center, side left, side right */
				) {
					/* to dolby order: front left, center, front right [, surround left, surround right ] */
					channel_map[1] = 2;
					channel_map[2] = 1;
				}
				else if(
					channel_mask == 0x003f || /* 6 channels: front left, front right, front center, LFE, back left, back right */
					channel_mask == 0x060f    /* 6 channels: front left, front right, front center, LFE, side left, side right */
				) {
					/* to dolby order: front left, center, front right, surround left, surround right, LFE */
					channel_map[1] = 2;
					channel_map[2] = 1;
					channel_map[3] = 5;
					channel_map[4] = 3;
					channel_map[5] = 4;
				}
#else
				if(
					options.common.channel_map_none ||
					channel_mask == 0x0001 || /* 1 channel: (mono) */
					channel_mask == 0x0003 || /* 2 channels: front left, front right */
					channel_mask == 0x0007 || /* 3 channels: front left, front right, front center */
					channel_mask == 0x0033 || /* 4 channels: front left, front right, back left, back right */
					channel_mask == 0x0603 || /* 4 channels: front left, front right, side left, side right */
					channel_mask == 0x0037 || /* 5 channels: front left, front right, front center, back left, back right */
					channel_mask == 0x0607 || /* 5 channels: front left, front right, front center, side left, side right */
					channel_mask == 0x003f || /* 6 channels: front left, front right, front center, LFE, back left, back right */
					channel_mask == 0x060f    /* 6 channels: front left, front right, front center, LFE, side left, side right */
				) {
					/* keep default channel order */
				}
#endif
				else {
					flac__utils_printf(stderr, 1, "%s: ERROR: WAVEFORMATEXTENSIBLE chunk with unsupported channel mask=0x%04X\n", encoder_session.inbasefilename, (unsigned)channel_mask);
					return EncoderSession_finish_error(&encoder_session);
				}
				if(!options.common.channel_map_none) {
					if(count_channel_mask_bits(channel_mask) < channels) {
						flac__utils_printf(stderr, 1, "%s: ERROR: WAVEFORMATEXTENSIBLE chunk: channel mask 0x%04X has unassigned channels (#channels=%u)\n", encoder_session.inbasefilename, (unsigned)channel_mask, channels);
						return EncoderSession_finish_error(&encoder_session);
					}
#if 0
					/* supporting this is too difficult with channel mapping; e.g. what if mask is 0x003f but #channels=4?
					 * there would be holes in the order that would have to be filled in, or the mask would have to be
					 * limited and the logic above rerun to see if it still fits into the FLAC mapping.
					 */
					else if(count_channel_mask_bits(channel_mask) > channels)
						channel_mask = limit_channel_mask(channel_mask, channels);
#else
					else if(count_channel_mask_bits(channel_mask) > channels) {
						flac__utils_printf(stderr, 1, "%s: ERROR: WAVEFORMATEXTENSIBLE chunk: channel mask 0x%04X has extra bits for non-existant channels (#channels=%u)\n", encoder_session.inbasefilename, (unsigned)channel_mask, channels);
						return EncoderSession_finish_error(&encoder_session);
					}
#endif
				}
				/* first part of GUID */
				if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
					return EncoderSession_finish_error(&encoder_session);
				if(x != 1) {
					flac__utils_printf(stderr, 1, "%s: ERROR: unsupported WAVEFORMATEXTENSIBLE chunk with non-PCM format %u\n", encoder_session.inbasefilename, (unsigned)x);
					return EncoderSession_finish_error(&encoder_session);
				}
				data_bytes -= 26;
			}

			if(bps-shift < 4 || bps-shift > 24) {
				flac__utils_printf(stderr, 1, "%s: ERROR: unsupported bits-per-sample %u\n", encoder_session.inbasefilename, bps-shift);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && bps-shift != 16) {
				flac__utils_printf(stderr, 1, "%s: ERROR: file has %u bits-per-sample, must be 16 for --sector-align\n", encoder_session.inbasefilename, bps-shift);
				return EncoderSession_finish_error(&encoder_session);
			}

			/* skip any extra data in the fmt sub-chunk */
			if(!fskip_ahead(infile, data_bytes)) {
				flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping over extra 'fmt' data\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}

			/*
			 * now that we know the sample rate, canonicalize the
			 * --skip string to a number of samples:
			 */
			flac__utils_canonicalize_skip_until_specification(&options.common.skip_specification, sample_rate);
			FLAC__ASSERT(options.common.skip_specification.value.samples >= 0);
			encoder_session.skip = (FLAC__uint64)options.common.skip_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.skip == 0);

			got_fmt_chunk = true;
		}
		else if(xx == 0x61746164 && !got_data_chunk && got_fmt_chunk) { /* "data" */
			FLAC__uint64 total_samples_in_input, trim = 0;
			FLAC__bool pad = false;
			const size_t bytes_per_wide_sample = channels * (bps >> 3);
			unsigned data_bytes;

			/* data size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(options.common.ignore_chunk_sizes) {
				FLAC__ASSERT(!options.common.sector_align);
				data_bytes = (unsigned)(-(int)bytes_per_wide_sample); /* max out data_bytes; we'll use EOF as signal to stop reading */
			}
			else {
				data_bytes = xx;
				if(0 == data_bytes) {
					flac__utils_printf(stderr, 1, "%s: ERROR: 'data' subchunk has size of 0\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}
			pad = (data_bytes & 1U) ? true : false;

			/* *options.common.align_reservoir_samples will be 0 unless --sector-align is used */
			FLAC__ASSERT(options.common.sector_align || *options.common.align_reservoir_samples == 0);
			total_samples_in_input = data_bytes / bytes_per_wide_sample + *options.common.align_reservoir_samples;

			/*
			 * now that we know the input size, canonicalize the
			 * --until string to an absolute sample number:
			 */
			if(!canonicalize_until_specification(&options.common.until_specification, encoder_session.inbasefilename, sample_rate, encoder_session.skip, total_samples_in_input))
				return EncoderSession_finish_error(&encoder_session);
			encoder_session.until = (FLAC__uint64)options.common.until_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.until == 0);

			if(encoder_session.skip > 0) {
				if(!fskip_ahead(infile, encoder_session.skip * bytes_per_wide_sample)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			data_bytes -= (unsigned)encoder_session.skip * bytes_per_wide_sample; /*@@@ WATCHOUT: 4GB limit */
			if(options.common.ignore_chunk_sizes) {
				encoder_session.total_samples_to_encode = 0;
				flac__utils_printf(stderr, 2, "(No runtime statistics possible; please wait for encoding to finish...)\n");
				FLAC__ASSERT(0 == encoder_session.until);
			}
			else {
				encoder_session.total_samples_to_encode = total_samples_in_input - encoder_session.skip;
			}
			if(encoder_session.until > 0) {
				trim = total_samples_in_input - encoder_session.until;
				FLAC__ASSERT(total_samples_in_input > 0);
				FLAC__ASSERT(!options.common.sector_align);
				data_bytes -= (unsigned int)trim * bytes_per_wide_sample;
				encoder_session.total_samples_to_encode -= trim;
			}
			if(options.common.sector_align) {
				align_remainder = (unsigned)(encoder_session.total_samples_to_encode % 588);
				if(options.common.is_last_file)
					encoder_session.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
				else
					encoder_session.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
			}

			/* +44 for the size of the WAV headers; this is just an estimate for the progress indicator and doesn't need to be exact */
			encoder_session.unencoded_size = encoder_session.total_samples_to_encode * bytes_per_wide_sample + 44;

			if(!EncoderSession_init_encoder(&encoder_session, options.common, channel_mask, channels, bps-shift, sample_rate, options.foreign_metadata, /*flac_decoder_data=*/0))
				return EncoderSession_finish_error(&encoder_session);

			/*
			 * first do any samples in the reservoir
			 */
			if(options.common.sector_align && *options.common.align_reservoir_samples > 0) {
				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			/*
			 * decrement the data_bytes counter if we need to align the file
			 */
			if(options.common.sector_align) {
				if(options.common.is_last_file) {
					*options.common.align_reservoir_samples = 0;
				}
				else {
					*options.common.align_reservoir_samples = align_remainder;
					data_bytes -= (*options.common.align_reservoir_samples) * bytes_per_wide_sample;
				}
			}

			/*
			 * now do from the file
			 */
			while(data_bytes > 0) {
				bytes_read = fread(ucbuffer_, sizeof(unsigned char), min(data_bytes, CHUNK_OF_SAMPLES * bytes_per_wide_sample), infile);
				if(bytes_read == 0) {
					if(ferror(infile)) {
						flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else if(feof(infile)) {
						if(options.common.ignore_chunk_sizes) {
							flac__utils_printf(stderr, 1, "%s: INFO: hit EOF with --ignore-chunk-sizes, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.samples_written);
						}
						else {
							flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
							if(encoder_session.treat_warnings_as_errors)
								return EncoderSession_finish_error(&encoder_session);
						}
						data_bytes = 0;
					}
				}
				else {
					if(bytes_read % bytes_per_wide_sample != 0) {
						flac__utils_printf(stderr, 1, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else {
						unsigned wide_samples = bytes_read / bytes_per_wide_sample;
						if(!format_input(input_, wide_samples, /*is_big_endian=*/false, is_unsigned_samples, channels, bps, shift, channel_map))
							return EncoderSession_finish_error(&encoder_session);

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
						data_bytes -= bytes_read;
					}
				}
			}

			if(trim > 0) {
				FLAC__ASSERT(!options.common.sector_align);
				if(!fskip_ahead(infile, trim * bytes_per_wide_sample)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			/*
			 * now read unaligned samples into reservoir or pad with zeroes if necessary
			 */
			if(options.common.sector_align) {
				if(options.common.is_last_file) {
					unsigned wide_samples = 588 - align_remainder;
					if(wide_samples < 588) {
						unsigned channel;

						info_align_zero = wide_samples;
						for(channel = 0; channel < channels; channel++)
							memset(input_[channel], 0, sizeof(input_[0][0]) * wide_samples);

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
					}
				}
				else {
					if(*options.common.align_reservoir_samples > 0) {
						FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
						bytes_read = fread(ucbuffer_, sizeof(unsigned char), (*options.common.align_reservoir_samples) * bytes_per_wide_sample, infile);
						if(bytes_read == 0 && ferror(infile)) {
							flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						else if(bytes_read != (*options.common.align_reservoir_samples) * bytes_per_wide_sample) {
							flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; read %u bytes; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)bytes_read, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
							if(encoder_session.treat_warnings_as_errors)
								return EncoderSession_finish_error(&encoder_session);
						}
						else {
							info_align_carry = *options.common.align_reservoir_samples;
							if(!format_input(options.common.align_reservoir, *options.common.align_reservoir_samples, /*is_big_endian=*/false, is_unsigned_samples, channels, bps, shift, channel_map))
								return EncoderSession_finish_error(&encoder_session);
						}
					}
				}
			}

			if(pad == true) {
				unsigned char tmp;

				if(fread(&tmp, 1U, 1U, infile) < 1U) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read of data pad byte\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			got_data_chunk = true;
		}
		else {
			if(xx == 0x61746164 && !got_fmt_chunk) { /* "data" */
				flac__utils_printf(stderr, 1, "%s: ERROR: got 'data' sub-chunk before 'fmt' sub-chunk\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}

			if(!options.foreign_metadata) {
				if(xx == 0x20746d66 && got_fmt_chunk) /* "fmt " */
					flac__utils_printf(stderr, 1, "%s: WARNING: skipping extra 'fmt ' sub-chunk (use --keep-foreign-metadata to keep)\n", encoder_session.inbasefilename);
				else if(xx == 0x61746164) /* "data" */
					flac__utils_printf(stderr, 1, "%s: WARNING: skipping extra 'data' sub-chunk (use --keep-foreign-metadata to keep)\n", encoder_session.inbasefilename);
				else
					flac__utils_printf(stderr, 1, "%s: WARNING: skipping unknown sub-chunk '%c%c%c%c' (use --keep-foreign-metadata to keep)\n", encoder_session.inbasefilename, (char)(xx&255), (char)((xx>>8)&255), (char)((xx>>16)&255), (char)(xx>>24));
				if(encoder_session.treat_warnings_as_errors)
					return EncoderSession_finish_error(&encoder_session);
			}

			/* sub-chunk size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else {
				unsigned long skip = xx+(xx & 1U);

				FLAC__ASSERT(skip<=LONG_MAX);
				if(!fskip_ahead(infile, skip)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping over unsupported sub-chunk\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}
		}
	}

	return EncoderSession_finish_ok(&encoder_session, info_align_carry, info_align_zero, options.foreign_metadata);
}

int flac__encode_raw(FILE *infile, off_t infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, raw_encode_options_t options)
{
	EncoderSession encoder_session;
	size_t bytes_read;
	const size_t bytes_per_wide_sample = options.channels * (options.bps >> 3);
	unsigned align_remainder = 0;
	int info_align_carry = -1, info_align_zero = -1;
	FLAC__uint64 total_samples_in_input = 0;

	FLAC__ASSERT(!options.common.sector_align || options.channels == 2);
	FLAC__ASSERT(!options.common.sector_align || options.bps == 16);
	FLAC__ASSERT(!options.common.sector_align || options.sample_rate == 44100);
	FLAC__ASSERT(!options.common.sector_align || infilesize >= 0);
	FLAC__ASSERT(!options.common.replay_gain || options.channels <= 2);
	FLAC__ASSERT(!options.common.replay_gain || grabbag__replaygain_is_valid_sample_frequency(options.sample_rate));

	if(!
		EncoderSession_construct(
			&encoder_session,
#if FLAC__HAS_OGG
			options.common.use_ogg,
#else
			/*use_ogg=*/false,
#endif
			options.common.verify,
			options.common.treat_warnings_as_errors,
			options.common.continue_through_decode_errors,
			infile,
			infilename,
			outfilename
		)
	)
		return 1;

	/*
	 * now that we know the sample rate, canonicalize the
	 * --skip string to a number of samples:
	 */
	flac__utils_canonicalize_skip_until_specification(&options.common.skip_specification, options.sample_rate);
	FLAC__ASSERT(options.common.skip_specification.value.samples >= 0);
	encoder_session.skip = (FLAC__uint64)options.common.skip_specification.value.samples;
	FLAC__ASSERT(!options.common.sector_align || encoder_session.skip == 0);

	if(infilesize < 0)
		total_samples_in_input = 0;
	else {
		/* *options.common.align_reservoir_samples will be 0 unless --sector-align is used */
		FLAC__ASSERT(options.common.sector_align || *options.common.align_reservoir_samples == 0);
		total_samples_in_input = (FLAC__uint64)infilesize / bytes_per_wide_sample + *options.common.align_reservoir_samples;
	}

	/*
	 * now that we know the input size, canonicalize the
	 * --until strings to a number of samples:
	 */
	if(!canonicalize_until_specification(&options.common.until_specification, encoder_session.inbasefilename, options.sample_rate, encoder_session.skip, total_samples_in_input))
		return EncoderSession_finish_error(&encoder_session);
	encoder_session.until = (FLAC__uint64)options.common.until_specification.value.samples;
	FLAC__ASSERT(!options.common.sector_align || encoder_session.until == 0);

	infilesize -= (off_t)encoder_session.skip * bytes_per_wide_sample;
	encoder_session.total_samples_to_encode = total_samples_in_input - encoder_session.skip;
	if(encoder_session.until > 0) {
		const FLAC__uint64 trim = total_samples_in_input - encoder_session.until;
		FLAC__ASSERT(total_samples_in_input > 0);
		FLAC__ASSERT(!options.common.sector_align);
		infilesize -= (off_t)trim * bytes_per_wide_sample;
		encoder_session.total_samples_to_encode -= trim;
	}
	if(infilesize >= 0 && options.common.sector_align) {
		FLAC__ASSERT(encoder_session.skip == 0);
		align_remainder = (unsigned)(encoder_session.total_samples_to_encode % 588);
		if(options.common.is_last_file)
			encoder_session.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
		else
			encoder_session.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
	}
	encoder_session.unencoded_size = encoder_session.total_samples_to_encode * bytes_per_wide_sample;

	if(encoder_session.total_samples_to_encode <= 0)
		flac__utils_printf(stderr, 2, "(No runtime statistics possible; please wait for encoding to finish...)\n");

	if(encoder_session.skip > 0) {
		unsigned skip_bytes = bytes_per_wide_sample * (unsigned)encoder_session.skip;
		if(skip_bytes > lookahead_length) {
			skip_bytes -= lookahead_length;
			lookahead_length = 0;
			if(!fskip_ahead(infile, skip_bytes)) {
				flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}
		}
		else {
			lookahead += skip_bytes;
			lookahead_length -= skip_bytes;
		}
	}

	if(!EncoderSession_init_encoder(&encoder_session, options.common, /*channel_mask=*/0, options.channels, options.bps, options.sample_rate, /*foreign_metadata=*/0, /*flac_decoder_data=*/0))
		return EncoderSession_finish_error(&encoder_session);

	/*
	 * first do any samples in the reservoir
	 */
	if(options.common.sector_align && *options.common.align_reservoir_samples > 0) {
		if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
			print_error_with_state(&encoder_session, "ERROR during encoding");
			return EncoderSession_finish_error(&encoder_session);
		}
	}

	/*
	 * decrement infilesize if we need to align the file
	 */
	if(options.common.sector_align) {
		FLAC__ASSERT(infilesize >= 0);
		if(options.common.is_last_file) {
			*options.common.align_reservoir_samples = 0;
		}
		else {
			*options.common.align_reservoir_samples = align_remainder;
			infilesize -= (off_t)((*options.common.align_reservoir_samples) * bytes_per_wide_sample);
			FLAC__ASSERT(infilesize >= 0);
		}
	}

	/*
	 * now do from the file
	 */
	if(infilesize < 0) {
		while(!feof(infile)) {
			if(lookahead_length > 0) {
				FLAC__ASSERT(lookahead_length < CHUNK_OF_SAMPLES * bytes_per_wide_sample);
				memcpy(ucbuffer_, lookahead, lookahead_length);
				bytes_read = fread(ucbuffer_+lookahead_length, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample - lookahead_length, infile) + lookahead_length;
				if(ferror(infile)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				lookahead_length = 0;
			}
			else
				bytes_read = fread(ucbuffer_, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample, infile);

			if(bytes_read == 0) {
				if(ferror(infile)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}
			else if(bytes_read % bytes_per_wide_sample != 0) {
				flac__utils_printf(stderr, 1, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}
			else {
				unsigned wide_samples = bytes_read / bytes_per_wide_sample;
				if(!format_input(input_, wide_samples, options.is_big_endian, options.is_unsigned_samples, options.channels, options.bps, /*shift=*/0, /*channel_map=*/0))
					return EncoderSession_finish_error(&encoder_session);

				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}
		}
	}
	else {
		const FLAC__uint64 max_input_bytes = infilesize;
		FLAC__uint64 total_input_bytes_read = 0;
		while(total_input_bytes_read < max_input_bytes) {
			{
				size_t wanted = (CHUNK_OF_SAMPLES * bytes_per_wide_sample);
				wanted = (size_t) min((FLAC__uint64)wanted, max_input_bytes - total_input_bytes_read);

				if(lookahead_length > 0) {
					FLAC__ASSERT(lookahead_length <= wanted);
					memcpy(ucbuffer_, lookahead, lookahead_length);
					wanted -= lookahead_length;
					bytes_read = lookahead_length;
					if(wanted > 0) {
						bytes_read += fread(ucbuffer_+lookahead_length, sizeof(unsigned char), wanted, infile);
						if(ferror(infile)) {
							flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
					}
					lookahead_length = 0;
				}
				else
					bytes_read = fread(ucbuffer_, sizeof(unsigned char), wanted, infile);
			}

			if(bytes_read == 0) {
				if(ferror(infile)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				else if(feof(infile)) {
					flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
					if(encoder_session.treat_warnings_as_errors)
						return EncoderSession_finish_error(&encoder_session);
					total_input_bytes_read = max_input_bytes;
				}
			}
			else {
				if(bytes_read % bytes_per_wide_sample != 0) {
					flac__utils_printf(stderr, 1, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				else {
					unsigned wide_samples = bytes_read / bytes_per_wide_sample;
					if(!format_input(input_, wide_samples, options.is_big_endian, options.is_unsigned_samples, options.channels, options.bps, /*shift=*/0, /*channel_map=*/0))
						return EncoderSession_finish_error(&encoder_session);

					if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
						print_error_with_state(&encoder_session, "ERROR during encoding");
						return EncoderSession_finish_error(&encoder_session);
					}
					total_input_bytes_read += bytes_read;
				}
			}
		}
	}

	/*
	 * now read unaligned samples into reservoir or pad with zeroes if necessary
	 */
	if(options.common.sector_align) {
		if(options.common.is_last_file) {
			unsigned wide_samples = 588 - align_remainder;
			if(wide_samples < 588) {
				unsigned channel;

				info_align_zero = wide_samples;
				for(channel = 0; channel < options.channels; channel++)
					memset(input_[channel], 0, sizeof(input_[0][0]) * wide_samples);

				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}
		}
		else {
			if(*options.common.align_reservoir_samples > 0) {
				FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
				bytes_read = fread(ucbuffer_, sizeof(unsigned char), (*options.common.align_reservoir_samples) * bytes_per_wide_sample, infile);
				if(bytes_read == 0 && ferror(infile)) {
					flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				else if(bytes_read != (*options.common.align_reservoir_samples) * bytes_per_wide_sample) {
					flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; read %u bytes; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)bytes_read, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
					if(encoder_session.treat_warnings_as_errors)
						return EncoderSession_finish_error(&encoder_session);
				}
				else {
					info_align_carry = *options.common.align_reservoir_samples;
					if(!format_input(options.common.align_reservoir, *options.common.align_reservoir_samples, options.is_big_endian, options.is_unsigned_samples, options.channels, options.bps, /*shift=*/0, /*channel_map=*/0))
						return EncoderSession_finish_error(&encoder_session);
				}
			}
		}
	}

	return EncoderSession_finish_ok(&encoder_session, info_align_carry, info_align_zero, /*foreign_metadata=*/0);
}

int flac__encode_flac(FILE *infile, off_t infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, flac_encode_options_t options, FLAC__bool input_is_ogg)
{
	EncoderSession encoder_session;
	FLAC__StreamDecoder *decoder = 0;
	FLACDecoderData decoder_data;
	size_t i;
	int retval;

	if(!
		EncoderSession_construct(
			&encoder_session,
#if FLAC__HAS_OGG
			options.common.use_ogg,
#else
			/*use_ogg=*/false,
#endif
			options.common.verify,
			options.common.treat_warnings_as_errors,
			options.common.continue_through_decode_errors,
			infile,
			infilename,
			outfilename
		)
	)
		return 1;

	decoder_data.encoder_session = &encoder_session;
	decoder_data.filesize = (infilesize == (off_t)(-1)? 0 : infilesize);
	decoder_data.lookahead = lookahead;
	decoder_data.lookahead_length = lookahead_length;
	decoder_data.num_metadata_blocks = 0;
	decoder_data.samples_left_to_process = 0;
	decoder_data.fatal_error = false;

	/*
	 * set up FLAC decoder for the input
	 */
	if (0 == (decoder = FLAC__stream_decoder_new())) {
		flac__utils_printf(stderr, 1, "%s: ERROR: creating decoder for FLAC input\n", encoder_session.inbasefilename);
		return EncoderSession_finish_error(&encoder_session);
	}
	if (!(
		FLAC__stream_decoder_set_md5_checking(decoder, false) &&
		FLAC__stream_decoder_set_metadata_respond_all(decoder)
	)) {
		flac__utils_printf(stderr, 1, "%s: ERROR: setting up decoder for FLAC input\n", encoder_session.inbasefilename);
		goto fubar1; /*@@@ yuck */
	}

	if (input_is_ogg) {
		if (FLAC__stream_decoder_init_ogg_stream(decoder, flac_decoder_read_callback, flac_decoder_seek_callback, flac_decoder_tell_callback, flac_decoder_length_callback, flac_decoder_eof_callback, flac_decoder_write_callback, flac_decoder_metadata_callback, flac_decoder_error_callback, /*client_data=*/&decoder_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
			flac__utils_printf(stderr, 1, "%s: ERROR: initializing decoder for Ogg FLAC input, state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(decoder));
			goto fubar1; /*@@@ yuck */
		}
	}
	else if (FLAC__stream_decoder_init_stream(decoder, flac_decoder_read_callback, flac_decoder_seek_callback, flac_decoder_tell_callback, flac_decoder_length_callback, flac_decoder_eof_callback, flac_decoder_write_callback, flac_decoder_metadata_callback, flac_decoder_error_callback, /*client_data=*/&decoder_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		flac__utils_printf(stderr, 1, "%s: ERROR: initializing decoder for FLAC input, state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(decoder));
		goto fubar1; /*@@@ yuck */
	}

	if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder) || decoder_data.fatal_error) {
		if (decoder_data.fatal_error)
			flac__utils_printf(stderr, 1, "%s: ERROR: out of memory or too many metadata blocks while reading metadata in FLAC input\n", encoder_session.inbasefilename);
		else
			flac__utils_printf(stderr, 1, "%s: ERROR: reading metadata in FLAC input, state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(decoder));
		goto fubar1; /*@@@ yuck */
	}

	if (decoder_data.num_metadata_blocks == 0) {
		flac__utils_printf(stderr, 1, "%s: ERROR: reading metadata in FLAC input, got no metadata blocks\n", encoder_session.inbasefilename);
		goto fubar2; /*@@@ yuck */
	}
	else if (decoder_data.metadata_blocks[0]->type != FLAC__METADATA_TYPE_STREAMINFO) {
		flac__utils_printf(stderr, 1, "%s: ERROR: reading metadata in FLAC input, first metadata block is not STREAMINFO\n", encoder_session.inbasefilename);
		goto fubar2; /*@@@ yuck */
	}
	else if (decoder_data.metadata_blocks[0]->data.stream_info.total_samples == 0) {
		flac__utils_printf(stderr, 1, "%s: ERROR: FLAC input has STREAMINFO with unknown total samples which is not supported\n", encoder_session.inbasefilename);
		goto fubar2; /*@@@ yuck */
	}

	/*
	 * now that we have the STREAMINFO and know the sample rate,
	 * canonicalize the --skip string to a number of samples:
	 */
	flac__utils_canonicalize_skip_until_specification(&options.common.skip_specification, decoder_data.metadata_blocks[0]->data.stream_info.sample_rate);
	FLAC__ASSERT(options.common.skip_specification.value.samples >= 0);
	encoder_session.skip = (FLAC__uint64)options.common.skip_specification.value.samples;
	FLAC__ASSERT(!options.common.sector_align); /* --sector-align with FLAC input is not supported */

	{
		FLAC__uint64 total_samples_in_input, trim = 0;

		total_samples_in_input = decoder_data.metadata_blocks[0]->data.stream_info.total_samples;

		/*
		 * now that we know the input size, canonicalize the
		 * --until string to an absolute sample number:
		 */
		if(!canonicalize_until_specification(&options.common.until_specification, encoder_session.inbasefilename, decoder_data.metadata_blocks[0]->data.stream_info.sample_rate, encoder_session.skip, total_samples_in_input))
			goto fubar2; /*@@@ yuck */
		encoder_session.until = (FLAC__uint64)options.common.until_specification.value.samples;

		encoder_session.total_samples_to_encode = total_samples_in_input - encoder_session.skip;
		if(encoder_session.until > 0) {
			trim = total_samples_in_input - encoder_session.until;
			FLAC__ASSERT(total_samples_in_input > 0);
			encoder_session.total_samples_to_encode -= trim;
		}

		encoder_session.unencoded_size = decoder_data.filesize;

		/* (channel mask will get copied over from the source VORBIS_COMMENT if it exists) */
		if(!EncoderSession_init_encoder(&encoder_session, options.common, /*channel_mask=*/0, decoder_data.metadata_blocks[0]->data.stream_info.channels, decoder_data.metadata_blocks[0]->data.stream_info.bits_per_sample, decoder_data.metadata_blocks[0]->data.stream_info.sample_rate, /*foreign_metadata=*/0, &decoder_data))
			goto fubar2; /*@@@ yuck */

		/*
		 * have to wait until the FLAC encoder is set up for writing
		 * before any seeking in the input FLAC file, because the seek
		 * itself will usually call the decoder's write callback, and
		 * our decoder's write callback passes samples to our FLAC
		 * encoder
		 */
		decoder_data.samples_left_to_process = encoder_session.total_samples_to_encode;
		if(encoder_session.skip > 0) {
			if(!FLAC__stream_decoder_seek_absolute(decoder, encoder_session.skip)) {
				flac__utils_printf(stderr, 1, "%s: ERROR while skipping samples, FLAC decoder state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(decoder));
				goto fubar2; /*@@@ yuck */
			}
		}

		/*
		 * now do samples from the file
		 */
		while(!decoder_data.fatal_error && decoder_data.samples_left_to_process > 0) {
			/* We can also hit the end of stream without samples_left_to_process
			 * going to 0 if there are errors and continue_through_decode_errors
			 * is on, so we want to break in that case too:
			 */
			if(encoder_session.continue_through_decode_errors && FLAC__stream_decoder_get_state(decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
				break;
			if(!FLAC__stream_decoder_process_single(decoder)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: while decoding FLAC input, state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(decoder));
				goto fubar2; /*@@@ yuck */
			}
		}
		if(decoder_data.fatal_error) {
			flac__utils_printf(stderr, 1, "%s: ERROR: while decoding FLAC input, state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(decoder));
			goto fubar2; /*@@@ yuck */
		}
	}

	FLAC__stream_decoder_delete(decoder);
	retval = EncoderSession_finish_ok(&encoder_session, -1, -1, /*foreign_metadata=*/0);
	/* have to wail until encoder is completely finished before deleting because of the final step of writing the seekpoint offsets */
	for(i = 0; i < decoder_data.num_metadata_blocks; i++)
		FLAC__metadata_object_delete(decoder_data.metadata_blocks[i]);
	return retval;

fubar2:
	for(i = 0; i < decoder_data.num_metadata_blocks; i++)
		FLAC__metadata_object_delete(decoder_data.metadata_blocks[i]);
fubar1:
	FLAC__stream_decoder_delete(decoder);
	return EncoderSession_finish_error(&encoder_session);
}

FLAC__bool EncoderSession_construct(EncoderSession *e, FLAC__bool use_ogg, FLAC__bool verify, FLAC__bool treat_warnings_as_errors, FLAC__bool continue_through_decode_errors, FILE *infile, const char *infilename, const char *outfilename)
{
	unsigned i;
	FLAC__uint32 test = 1;

	/*
	 * initialize globals
	 */

	is_big_endian_host_ = (*((FLAC__byte*)(&test)))? false : true;

	for(i = 0; i < FLAC__MAX_CHANNELS; i++)
		input_[i] = &(in_[i][0]);


	/*
	 * initialize instance
	 */

#if FLAC__HAS_OGG
	e->use_ogg = use_ogg;
#else
	(void)use_ogg;
#endif
	e->verify = verify;
	e->treat_warnings_as_errors = treat_warnings_as_errors;
	e->continue_through_decode_errors = continue_through_decode_errors;

	e->is_stdout = (0 == strcmp(outfilename, "-"));
	e->outputfile_opened = false;

	e->inbasefilename = grabbag__file_get_basename(infilename);
	e->infilename = infilename;
	e->outfilename = outfilename;

	e->skip = 0; /* filled in later after the sample_rate is known */
	e->unencoded_size = 0;
	e->total_samples_to_encode = 0;
	e->bytes_written = 0;
	e->samples_written = 0;
	e->stats_mask = 0;

	e->encoder = 0;

	e->fin = infile;
	e->seek_table_template = 0;

	if(0 == (e->seek_table_template = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE))) {
		flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for seek table\n", e->inbasefilename);
		return false;
	}

	e->encoder = FLAC__stream_encoder_new();
	if(0 == e->encoder) {
		flac__utils_printf(stderr, 1, "%s: ERROR creating the encoder instance\n", e->inbasefilename);
		EncoderSession_destroy(e);
		return false;
	}

	return true;
}

void EncoderSession_destroy(EncoderSession *e)
{
	if(e->fin != stdin)
		fclose(e->fin);

	if(0 != e->encoder) {
		FLAC__stream_encoder_delete(e->encoder);
		e->encoder = 0;
	}

	if(0 != e->seek_table_template) {
		FLAC__metadata_object_delete(e->seek_table_template);
		e->seek_table_template = 0;
	}
}

int EncoderSession_finish_ok(EncoderSession *e, int info_align_carry, int info_align_zero, foreign_metadata_t *foreign_metadata)
{
	FLAC__StreamEncoderState fse_state = FLAC__STREAM_ENCODER_OK;
	int ret = 0;
	FLAC__bool verify_error = false;

	if(e->encoder) {
		fse_state = FLAC__stream_encoder_get_state(e->encoder);
		ret = FLAC__stream_encoder_finish(e->encoder)? 0 : 1;
		verify_error =
			fse_state == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA ||
			FLAC__stream_encoder_get_state(e->encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA
		;
	}
	/* all errors except verify errors should interrupt the stats */
	if(ret && !verify_error)
		print_error_with_state(e, "ERROR during encoding");
	else if(e->total_samples_to_encode > 0) {
		print_stats(e);
		flac__utils_printf(stderr, 2, "\n");
	}

	if(verify_error) {
		print_verify_error(e);
		ret = 1;
	}
	else {
		if(info_align_carry >= 0) {
			flac__utils_printf(stderr, 1, "%s: INFO: sector alignment causing %d samples to be carried over\n", e->inbasefilename, info_align_carry);
		}
		if(info_align_zero >= 0) {
			flac__utils_printf(stderr, 1, "%s: INFO: sector alignment causing %d zero samples to be appended\n", e->inbasefilename, info_align_zero);
		}
	}

	/*@@@@@@ should this go here or somewhere else? */
	if(ret == 0 && foreign_metadata) {
		const char *error;
		if(!flac__foreign_metadata_write_to_flac(foreign_metadata, e->infilename, e->outfilename, &error)) {
			flac__utils_printf(stderr, 1, "%s: ERROR: updating foreign metadata in FLAC file: %s\n", e->inbasefilename, error);
			ret = 1;
		}
	}

	EncoderSession_destroy(e);

	return ret;
}

int EncoderSession_finish_error(EncoderSession *e)
{
	FLAC__ASSERT(e->encoder);

	if(e->total_samples_to_encode > 0)
		flac__utils_printf(stderr, 2, "\n");

	if(FLAC__stream_encoder_get_state(e->encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA)
		print_verify_error(e);
	else if(e->outputfile_opened)
		/* only want to delete the file if we opened it; otherwise it could be an existing file and our overwrite failed */
		unlink(e->outfilename);

	EncoderSession_destroy(e);

	return 1;
}

typedef struct {
	unsigned num_metadata;
	FLAC__bool *needs_delete;
	FLAC__StreamMetadata **metadata;
	FLAC__StreamMetadata *cuesheet; /* always needs to be deleted */
} static_metadata_t;

static void static_metadata_init(static_metadata_t *m)
{
	m->num_metadata = 0;
	m->needs_delete = 0;
	m->metadata = 0;
	m->cuesheet = 0;
}

static void static_metadata_clear(static_metadata_t *m)
{
	unsigned i;
	for(i = 0; i < m->num_metadata; i++)
		if(m->needs_delete[i])
			FLAC__metadata_object_delete(m->metadata[i]);
	if(m->metadata)
		free(m->metadata);
	if(m->needs_delete)
		free(m->needs_delete);
	if(m->cuesheet)
		FLAC__metadata_object_delete(m->cuesheet);
	static_metadata_init(m);
}

static FLAC__bool static_metadata_append(static_metadata_t *m, FLAC__StreamMetadata *d, FLAC__bool needs_delete)
{
	void *x;
	if(0 == (x = safe_realloc_muladd2_(m->metadata, sizeof(*m->metadata), /*times (*/m->num_metadata, /*+*/1/*)*/)))
		return false;
	m->metadata = (FLAC__StreamMetadata**)x;
	if(0 == (x = safe_realloc_muladd2_(m->needs_delete, sizeof(*m->needs_delete), /*times (*/m->num_metadata, /*+*/1/*)*/)))
		return false;
	m->needs_delete = (FLAC__bool*)x;
	m->metadata[m->num_metadata] = d;
	m->needs_delete[m->num_metadata] = needs_delete;
	m->num_metadata++;
	return true;
}

FLAC__bool EncoderSession_init_encoder(EncoderSession *e, encode_options_t options, FLAC__uint32 channel_mask, unsigned channels, unsigned bps, unsigned sample_rate, const foreign_metadata_t *foreign_metadata, FLACDecoderData *flac_decoder_data)
{
	FLAC__StreamMetadata padding;
	FLAC__StreamMetadata **metadata = 0;
	static_metadata_t static_metadata;
	unsigned num_metadata = 0, i;
	FLAC__StreamEncoderInitStatus init_status;
	const FLAC__bool is_cdda = (channels == 1 || channels == 2) && (bps == 16) && (sample_rate == 44100);
	char apodizations[2000];

	FLAC__ASSERT(sizeof(options.pictures)/sizeof(options.pictures[0]) <= 64);

	static_metadata_init(&static_metadata);

	e->replay_gain = options.replay_gain;
	e->channels = channels;
	e->bits_per_sample = bps;
	e->sample_rate = sample_rate;

	apodizations[0] = '\0';

	if(e->replay_gain) {
		if(channels != 1 && channels != 2) {
			flac__utils_printf(stderr, 1, "%s: ERROR, number of channels (%u) must be 1 or 2 for --replay-gain\n", e->inbasefilename, channels);
			return false;
		}
		if(!grabbag__replaygain_is_valid_sample_frequency(sample_rate)) {
			flac__utils_printf(stderr, 1, "%s: ERROR, invalid sample rate (%u) for --replay-gain\n", e->inbasefilename, sample_rate);
			return false;
		}
		if(options.is_first_file) {
			if(!grabbag__replaygain_init(sample_rate)) {
				flac__utils_printf(stderr, 1, "%s: ERROR initializing ReplayGain stage\n", e->inbasefilename);
				return false;
			}
		}
	}

	if(!parse_cuesheet(&static_metadata.cuesheet, options.cuesheet_filename, e->inbasefilename, is_cdda, e->total_samples_to_encode, e->treat_warnings_as_errors))
		return false;

	if(!convert_to_seek_table_template(options.requested_seek_points, options.num_requested_seek_points, options.cued_seekpoints? static_metadata.cuesheet : 0, e)) {
		flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for seek table\n", e->inbasefilename);
		static_metadata_clear(&static_metadata);
		return false;
	}

	/* build metadata */
	if(flac_decoder_data) {
		/*
		 * we're encoding from FLAC so we will use the FLAC file's
		 * metadata as the basis for the encoded file
		 */
		{
			/*
			 * first handle pictures: simple append any --pictures
			 * specified.
			 */
			for(i = 0; i < options.num_pictures; i++) {
				FLAC__StreamMetadata *pic = FLAC__metadata_object_clone(options.pictures[i]);
				if(0 == pic) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for PICTURE block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks++] = pic;
			}
		}
		{
			/*
			 * next handle vorbis comment: if any tags were specified
			 * or there is no existing vorbis comment, we create a
			 * new vorbis comment (discarding any existing one); else
			 * we keep the existing one.  also need to make sure to
			 * propagate any channel mask tag.
			 */
			/* @@@ change to append -T values from options.vorbis_comment if input has VC already? */
			size_t i, j;
			FLAC__bool vc_found = false;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
					vc_found = true;
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_VORBIS_COMMENT && options.vorbis_comment->data.vorbis_comment.num_comments > 0) {
					(void) flac__utils_get_channel_mask_tag(flac_decoder_data->metadata_blocks[i], &channel_mask);
					flac__utils_printf(stderr, 1, "%s: WARNING, replacing tags from input FLAC file with those given on the command-line\n", e->inbasefilename);
					if(e->treat_warnings_as_errors) {
						static_metadata_clear(&static_metadata);
						return false;
					}
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if((!vc_found || options.vorbis_comment->data.vorbis_comment.num_comments > 0) && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
				/* prepend ours */
				FLAC__StreamMetadata *vc = FLAC__metadata_object_clone(options.vorbis_comment);
				if(0 == vc || (channel_mask && !flac__utils_set_channel_mask_tag(vc, channel_mask))) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for VORBIS_COMMENT block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				for(i = flac_decoder_data->num_metadata_blocks; i > 1; i--)
					flac_decoder_data->metadata_blocks[i] = flac_decoder_data->metadata_blocks[i-1];
				flac_decoder_data->metadata_blocks[1] = vc;
				flac_decoder_data->num_metadata_blocks++;
			}
		}
		{
			/*
			 * next handle cuesheet: if --cuesheet was specified, use
			 * it; else if file has existing CUESHEET and cuesheet's
			 * lead-out offset is correct, keep it; else no CUESHEET
			 */
			size_t i, j;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				FLAC__bool existing_cuesheet_is_bad = false;
				/* check if existing cuesheet matches the input audio */
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_CUESHEET && 0 == static_metadata.cuesheet) {
					const FLAC__StreamMetadata_CueSheet *cs = &flac_decoder_data->metadata_blocks[i]->data.cue_sheet;
					if(e->total_samples_to_encode == 0) {
						flac__utils_printf(stderr, 1, "%s: WARNING, cuesheet in input FLAC file cannot be kept if input size is not known, dropping it...\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
						existing_cuesheet_is_bad = true;
					}
					else if(e->total_samples_to_encode != cs->tracks[cs->num_tracks-1].offset) {
						flac__utils_printf(stderr, 1, "%s: WARNING, lead-out offset of cuesheet in input FLAC file does not match input length, dropping existing cuesheet...\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
						existing_cuesheet_is_bad = true;
					}
				}
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_CUESHEET && (existing_cuesheet_is_bad || 0 != static_metadata.cuesheet)) {
					if(0 != static_metadata.cuesheet) {
						flac__utils_printf(stderr, 1, "%s: WARNING, replacing cuesheet in input FLAC file with the one given on the command-line\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
					}
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if(0 != static_metadata.cuesheet && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
				/* prepend ours */
				FLAC__StreamMetadata *cs = FLAC__metadata_object_clone(static_metadata.cuesheet);
				if(0 == cs) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for CUESHEET block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				for(i = flac_decoder_data->num_metadata_blocks; i > 1; i--)
					flac_decoder_data->metadata_blocks[i] = flac_decoder_data->metadata_blocks[i-1];
				flac_decoder_data->metadata_blocks[1] = cs;
				flac_decoder_data->num_metadata_blocks++;
			}
		}
		{
			/*
			 * next handle seektable: if -S- was specified, no
			 * SEEKTABLE; else if -S was specified, use it/them;
			 * else if file has existing SEEKTABLE and input size is
			 * preserved (no --skip/--until/etc specified), keep it;
			 * else use default seektable options
			 *
			 * note: meanings of num_requested_seek_points:
			 *  -1 : no -S option given, default to some value
			 *   0 : -S- given (no seektable)
			 *  >0 : one or more -S options given
			 */
			size_t i, j;
			FLAC__bool existing_seektable = false;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_SEEKTABLE)
					existing_seektable = true;
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_SEEKTABLE && (e->total_samples_to_encode != flac_decoder_data->metadata_blocks[0]->data.stream_info.total_samples || options.num_requested_seek_points >= 0)) {
					if(options.num_requested_seek_points > 0) {
						flac__utils_printf(stderr, 1, "%s: WARNING, replacing seektable in input FLAC file with the one given on the command-line\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
					}
					else if(options.num_requested_seek_points == 0)
						; /* no warning, silently delete existing SEEKTABLE since user specified --no-seektable (-S-) */
					else {
						flac__utils_printf(stderr, 1, "%s: WARNING, can't use existing seektable in input FLAC since the input size is changing or unknown, dropping existing SEEKTABLE block...\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
					}
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
					existing_seektable = false;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if((options.num_requested_seek_points > 0 || (options.num_requested_seek_points < 0 && !existing_seektable)) && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
				/* prepend ours */
				FLAC__StreamMetadata *st = FLAC__metadata_object_clone(e->seek_table_template);
				if(0 == st) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for SEEKTABLE block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				for(i = flac_decoder_data->num_metadata_blocks; i > 1; i--)
					flac_decoder_data->metadata_blocks[i] = flac_decoder_data->metadata_blocks[i-1];
				flac_decoder_data->metadata_blocks[1] = st;
				flac_decoder_data->num_metadata_blocks++;
			}
		}
		{
			/*
			 * finally handle padding: if --no-padding was specified,
			 * then delete all padding; else if -P was specified,
			 * use that instead of existing padding (if any); else
			 * if existing file has padding, move all existing
			 * padding blocks to one padding block at the end; else
			 * use default padding.
			 */
			int p = -1;
			size_t i, j;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_PADDING) {
					if(p < 0)
						p = 0;
					p += flac_decoder_data->metadata_blocks[i]->length;
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if(options.padding > 0)
				p = options.padding;
			if(p < 0)
				p = e->total_samples_to_encode / e->sample_rate < 20*60? FLAC_ENCODE__DEFAULT_PADDING : FLAC_ENCODE__DEFAULT_PADDING*8;
			if(options.padding != 0) {
				if(p > 0 && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
					flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
					if(0 == flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks]) {
						flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for PADDING block\n", e->inbasefilename);
						static_metadata_clear(&static_metadata);
						return false;
					}
					flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks]->is_last = false; /* the encoder will set this for us */
					flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks]->length = p;
					flac_decoder_data->num_metadata_blocks++;
				}
			}
		}
		metadata = &flac_decoder_data->metadata_blocks[1]; /* don't include STREAMINFO */
		num_metadata = flac_decoder_data->num_metadata_blocks - 1;
	}
	else {
		/*
		 * we're not encoding from FLAC so we will build the metadata
		 * from scratch
		 */
		if(e->seek_table_template->data.seek_table.num_points > 0) {
			e->seek_table_template->is_last = false; /* the encoder will set this for us */
			static_metadata_append(&static_metadata, e->seek_table_template, /*needs_delete=*/false);
		}
		if(0 != static_metadata.cuesheet)
			static_metadata_append(&static_metadata, static_metadata.cuesheet, /*needs_delete=*/false);
		if(channel_mask) {
			if(!flac__utils_set_channel_mask_tag(options.vorbis_comment, channel_mask)) {
				flac__utils_printf(stderr, 1, "%s: ERROR adding channel mask tag\n", e->inbasefilename);
				static_metadata_clear(&static_metadata);
				return false;
			}
		}
		static_metadata_append(&static_metadata, options.vorbis_comment, /*needs_delete=*/false);
		for(i = 0; i < options.num_pictures; i++)
			static_metadata_append(&static_metadata, options.pictures[i], /*needs_delete=*/false);
		if(foreign_metadata) {
			for(i = 0; i < foreign_metadata->num_blocks; i++) {
				FLAC__StreamMetadata *p = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
				if(!p) {
					flac__utils_printf(stderr, 1, "%s: ERROR: out of memory\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				static_metadata_append(&static_metadata, p, /*needs_delete=*/true);
				static_metadata.metadata[static_metadata.num_metadata-1]->length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8 + foreign_metadata->blocks[i].size;
/*fprintf(stderr,"@@@@@@ add PADDING=%u\n",static_metadata.metadata[static_metadata.num_metadata-1]->length);*/
			}
		}
		if(options.padding != 0) {
			padding.is_last = false; /* the encoder will set this for us */
			padding.type = FLAC__METADATA_TYPE_PADDING;
			padding.length = (unsigned)(options.padding>0? options.padding : (e->total_samples_to_encode / e->sample_rate < 20*60? FLAC_ENCODE__DEFAULT_PADDING : FLAC_ENCODE__DEFAULT_PADDING*8));
			static_metadata_append(&static_metadata, &padding, /*needs_delete=*/false);
		}
		metadata = static_metadata.metadata;
		num_metadata = static_metadata.num_metadata;
	}

	/* check for a few things that have not already been checked.  the
	 * FLAC__stream_encoder_init*() will check it but only return
	 * FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_METADATA so we check some
	 * up front to give a better error message.
	 */
	if(!verify_metadata(e, metadata, num_metadata)) {
		static_metadata_clear(&static_metadata);
		return false;
	}

	FLAC__stream_encoder_set_verify(e->encoder, options.verify);
	FLAC__stream_encoder_set_streamable_subset(e->encoder, !options.lax);
	FLAC__stream_encoder_set_channels(e->encoder, channels);
	FLAC__stream_encoder_set_bits_per_sample(e->encoder, bps);
	FLAC__stream_encoder_set_sample_rate(e->encoder, sample_rate);
	for(i = 0; i < options.num_compression_settings; i++) {
		switch(options.compression_settings[i].type) {
			case CST_BLOCKSIZE:
				FLAC__stream_encoder_set_blocksize(e->encoder, options.compression_settings[i].value.t_unsigned);
				break;
			case CST_COMPRESSION_LEVEL:
				FLAC__stream_encoder_set_compression_level(e->encoder, options.compression_settings[i].value.t_unsigned);
				apodizations[0] = '\0';
				break;
			case CST_DO_MID_SIDE:
				FLAC__stream_encoder_set_do_mid_side_stereo(e->encoder, options.compression_settings[i].value.t_bool);
				break;
			case CST_LOOSE_MID_SIDE:
				FLAC__stream_encoder_set_loose_mid_side_stereo(e->encoder, options.compression_settings[i].value.t_bool);
				break;
			case CST_APODIZATION:
				if(strlen(apodizations)+strlen(options.compression_settings[i].value.t_string)+2 >= sizeof(apodizations)) {
					flac__utils_printf(stderr, 1, "%s: ERROR: too many apodization functions requested\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				else {
					strcat(apodizations, options.compression_settings[i].value.t_string);
					strcat(apodizations, ";");
				}
				break;
			case CST_MAX_LPC_ORDER:
				FLAC__stream_encoder_set_max_lpc_order(e->encoder, options.compression_settings[i].value.t_unsigned);
				break;
			case CST_QLP_COEFF_PRECISION:
				FLAC__stream_encoder_set_qlp_coeff_precision(e->encoder, options.compression_settings[i].value.t_unsigned);
				break;
			case CST_DO_QLP_COEFF_PREC_SEARCH:
				FLAC__stream_encoder_set_do_qlp_coeff_prec_search(e->encoder, options.compression_settings[i].value.t_bool);
				break;
			case CST_DO_ESCAPE_CODING:
				FLAC__stream_encoder_set_do_escape_coding(e->encoder, options.compression_settings[i].value.t_bool);
				break;
			case CST_DO_EXHAUSTIVE_MODEL_SEARCH:
				FLAC__stream_encoder_set_do_exhaustive_model_search(e->encoder, options.compression_settings[i].value.t_bool);
				break;
			case CST_MIN_RESIDUAL_PARTITION_ORDER:
				FLAC__stream_encoder_set_min_residual_partition_order(e->encoder, options.compression_settings[i].value.t_unsigned);
				break;
			case CST_MAX_RESIDUAL_PARTITION_ORDER:
				FLAC__stream_encoder_set_max_residual_partition_order(e->encoder, options.compression_settings[i].value.t_unsigned);
				break;
			case CST_RICE_PARAMETER_SEARCH_DIST:
				FLAC__stream_encoder_set_rice_parameter_search_dist(e->encoder, options.compression_settings[i].value.t_unsigned);
				break;
		}
	}
	if(*apodizations)
		FLAC__stream_encoder_set_apodization(e->encoder, apodizations);
	FLAC__stream_encoder_set_total_samples_estimate(e->encoder, e->total_samples_to_encode);
	FLAC__stream_encoder_set_metadata(e->encoder, (num_metadata > 0)? metadata : 0, num_metadata);

	FLAC__stream_encoder_disable_constant_subframes(e->encoder, options.debug.disable_constant_subframes);
	FLAC__stream_encoder_disable_fixed_subframes(e->encoder, options.debug.disable_fixed_subframes);
	FLAC__stream_encoder_disable_verbatim_subframes(e->encoder, options.debug.disable_verbatim_subframes);
	if(!options.debug.do_md5) {
		flac__utils_printf(stderr, 1, "%s: WARNING, MD5 computation disabled, resulting file will not have MD5 sum\n", e->inbasefilename);
		if(e->treat_warnings_as_errors) {
			static_metadata_clear(&static_metadata);
			return false;
		}
		FLAC__stream_encoder_set_do_md5(e->encoder, false);
	}

#if FLAC__HAS_OGG
	if(e->use_ogg) {
		FLAC__stream_encoder_set_ogg_serial_number(e->encoder, options.serial_number);

		init_status = FLAC__stream_encoder_init_ogg_file(e->encoder, e->is_stdout? 0 : e->outfilename, encoder_progress_callback, /*client_data=*/e);
	}
	else
#endif
	{
		init_status = FLAC__stream_encoder_init_file(e->encoder, e->is_stdout? 0 : e->outfilename, encoder_progress_callback, /*client_data=*/e);
	}

	if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		print_error_with_init_status(e, "ERROR initializing encoder", init_status);
		if(FLAC__stream_encoder_get_state(e->encoder) != FLAC__STREAM_ENCODER_IO_ERROR)
			e->outputfile_opened = true;
		static_metadata_clear(&static_metadata);
		return false;
	}
	else
		e->outputfile_opened = true;

	e->stats_mask =
		(FLAC__stream_encoder_get_do_exhaustive_model_search(e->encoder) && FLAC__stream_encoder_get_do_qlp_coeff_prec_search(e->encoder))? 0x07 :
		(FLAC__stream_encoder_get_do_exhaustive_model_search(e->encoder) || FLAC__stream_encoder_get_do_qlp_coeff_prec_search(e->encoder))? 0x0f :
		0x3f;

	static_metadata_clear(&static_metadata);

	return true;
}

FLAC__bool EncoderSession_process(EncoderSession *e, const FLAC__int32 * const buffer[], unsigned samples)
{
	if(e->replay_gain) {
		if(!grabbag__replaygain_analyze(buffer, e->channels==2, e->bits_per_sample, samples)) {
			flac__utils_printf(stderr, 1, "%s: WARNING, error while calculating ReplayGain\n", e->inbasefilename);
			if(e->treat_warnings_as_errors)
				return false;
		}
	}

	return FLAC__stream_encoder_process(e->encoder, buffer, samples);
}

FLAC__bool convert_to_seek_table_template(const char *requested_seek_points, int num_requested_seek_points, FLAC__StreamMetadata *cuesheet, EncoderSession *e)
{
	const FLAC__bool only_placeholders = e->is_stdout;
	FLAC__bool has_real_points;

	if(num_requested_seek_points == 0 && 0 == cuesheet)
		return true;

	if(num_requested_seek_points < 0) {
#if FLAC__HAS_OGG
		/*@@@@@@ workaround ogg bug: too many seekpoints makes table not fit in one page */
		if(e->use_ogg && e->total_samples_to_encode > 0 && e->total_samples_to_encode / e->sample_rate / 10 > 230)
			requested_seek_points = "230x;";
		else 
#endif
			requested_seek_points = "10s;";
		num_requested_seek_points = 1;
	}

	if(num_requested_seek_points > 0) {
		if(!grabbag__seektable_convert_specification_to_template(requested_seek_points, only_placeholders, e->total_samples_to_encode, e->sample_rate, e->seek_table_template, &has_real_points))
			return false;
	}

	if(0 != cuesheet) {
		unsigned i, j;
		const FLAC__StreamMetadata_CueSheet *cs = &cuesheet->data.cue_sheet;
		for(i = 0; i < cs->num_tracks; i++) {
			const FLAC__StreamMetadata_CueSheet_Track *tr = cs->tracks+i;
			for(j = 0; j < tr->num_indices; j++) {
				if(!FLAC__metadata_object_seektable_template_append_point(e->seek_table_template, tr->offset + tr->indices[j].offset))
					return false;
				has_real_points = true;
			}
		}
		if(has_real_points)
			if(!FLAC__metadata_object_seektable_template_sort(e->seek_table_template, /*compact=*/true))
				return false;
	}

	if(has_real_points) {
		if(e->is_stdout) {
			flac__utils_printf(stderr, 1, "%s: WARNING, cannot write back seekpoints when encoding to stdout\n", e->inbasefilename);
			if(e->treat_warnings_as_errors)
				return false;
		}
	}

	return true;
}

FLAC__bool canonicalize_until_specification(utils__SkipUntilSpecification *spec, const char *inbasefilename, unsigned sample_rate, FLAC__uint64 skip, FLAC__uint64 total_samples_in_input)
{
	/* convert from mm:ss.sss to sample number if necessary */
	flac__utils_canonicalize_skip_until_specification(spec, sample_rate);

	/* special case: if "--until=-0", use the special value '0' to mean "end-of-stream" */
	if(spec->is_relative && spec->value.samples == 0) {
		spec->is_relative = false;
		return true;
	}

	/* in any other case the total samples in the input must be known */
	if(total_samples_in_input == 0) {
		flac__utils_printf(stderr, 1, "%s: ERROR, cannot use --until when input length is unknown\n", inbasefilename);
		return false;
	}

	FLAC__ASSERT(spec->value_is_samples);

	/* convert relative specifications to absolute */
	if(spec->is_relative) {
		if(spec->value.samples <= 0)
			spec->value.samples += (FLAC__int64)total_samples_in_input;
		else
			spec->value.samples += skip;
		spec->is_relative = false;
	}

	/* error check */
	if(spec->value.samples < 0) {
		flac__utils_printf(stderr, 1, "%s: ERROR, --until value is before beginning of input\n", inbasefilename);
		return false;
	}
	if((FLAC__uint64)spec->value.samples <= skip) {
		flac__utils_printf(stderr, 1, "%s: ERROR, --until value is before --skip point\n", inbasefilename);
		return false;
	}
	if((FLAC__uint64)spec->value.samples > total_samples_in_input) {
		flac__utils_printf(stderr, 1, "%s: ERROR, --until value is after end of input\n", inbasefilename);
		return false;
	}

	return true;
}

FLAC__bool verify_metadata(const EncoderSession *e, FLAC__StreamMetadata **metadata, unsigned num_metadata)
{
	FLAC__bool metadata_picture_has_type1 = false;
	FLAC__bool metadata_picture_has_type2 = false;
	unsigned i;

	FLAC__ASSERT(0 != metadata);
	for(i = 0; i < num_metadata; i++) {
		const FLAC__StreamMetadata *m = metadata[i];
		if(m->type == FLAC__METADATA_TYPE_SEEKTABLE) {
			if(!FLAC__format_seektable_is_legal(&m->data.seek_table)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: SEEKTABLE metadata block is invalid\n", e->inbasefilename);
				return false;
			}
		}
		else if(m->type == FLAC__METADATA_TYPE_CUESHEET) {
			if(!FLAC__format_cuesheet_is_legal(&m->data.cue_sheet, m->data.cue_sheet.is_cd, /*violation=*/0)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: CUESHEET metadata block is invalid\n", e->inbasefilename);
				return false;
			}
		}
		else if(m->type == FLAC__METADATA_TYPE_PICTURE) {
			const char *error = 0;
			if(!FLAC__format_picture_is_legal(&m->data.picture, &error)) {
				flac__utils_printf(stderr, 1, "%s: ERROR: PICTURE metadata block is invalid: %s\n", e->inbasefilename, error);
				return false;
			}
			if(m->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD) {
				if(metadata_picture_has_type1) {
					flac__utils_printf(stderr, 1, "%s: ERROR: there may only be one picture of type 1 (32x32 icon) in the file\n", e->inbasefilename);
					return false;
				}
				metadata_picture_has_type1 = true;
			}
			else if(m->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON) {
				if(metadata_picture_has_type2) {
					flac__utils_printf(stderr, 1, "%s: ERROR: there may only be one picture of type 2 (icon) in the file\n", e->inbasefilename);
					return false;
				}
				metadata_picture_has_type2 = true;
			}
		}
	}

	return true;
}

FLAC__bool format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps, unsigned shift, size_t *channel_map)
{
	unsigned wide_sample, sample, channel, byte;
	FLAC__int32 *out[FLAC__MAX_CHANNELS];

	if(0 == channel_map) {
		for(channel = 0; channel < channels; channel++)
			out[channel] = dest[channel];
	}
	else {
		for(channel = 0; channel < channels; channel++)
			out[channel] = dest[channel_map[channel]];
	}

	if(bps == 8) {
		if(is_unsigned_samples) {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					out[channel][wide_sample] = (FLAC__int32)ucbuffer_[sample] - 0x80;
		}
		else {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					out[channel][wide_sample] = (FLAC__int32)scbuffer_[sample];
		}
	}
	else if(bps == 16) {
		if(is_big_endian != is_big_endian_host_) {
			unsigned char tmp;
			const unsigned bytes = wide_samples * channels * (bps >> 3);
			for(byte = 0; byte < bytes; byte += 2) {
				tmp = ucbuffer_[byte];
				ucbuffer_[byte] = ucbuffer_[byte+1];
				ucbuffer_[byte+1] = tmp;
			}
		}
		if(is_unsigned_samples) {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					out[channel][wide_sample] = (FLAC__int32)usbuffer_[sample] - 0x8000;
		}
		else {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					out[channel][wide_sample] = (FLAC__int32)ssbuffer_[sample];
		}
	}
	else if(bps == 24) {
		if(!is_big_endian) {
			unsigned char tmp;
			const unsigned bytes = wide_samples * channels * (bps >> 3);
			for(byte = 0; byte < bytes; byte += 3) {
				tmp = ucbuffer_[byte];
				ucbuffer_[byte] = ucbuffer_[byte+2];
				ucbuffer_[byte+2] = tmp;
			}
		}
		if(is_unsigned_samples) {
			for(byte = sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++) {
					out[channel][wide_sample]  = ucbuffer_[byte++]; out[channel][wide_sample] <<= 8;
					out[channel][wide_sample] |= ucbuffer_[byte++]; out[channel][wide_sample] <<= 8;
					out[channel][wide_sample] |= ucbuffer_[byte++];
					out[channel][wide_sample] -= 0x800000;
				}
		}
		else {
			for(byte = sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++) {
					out[channel][wide_sample]  = scbuffer_[byte++]; out[channel][wide_sample] <<= 8;
					out[channel][wide_sample] |= ucbuffer_[byte++]; out[channel][wide_sample] <<= 8;
					out[channel][wide_sample] |= ucbuffer_[byte++];
				}
		}
	}
	else {
		FLAC__ASSERT(0);
	}
	if(shift > 0) {
		FLAC__int32 mask = (1<<shift)-1;
		for(wide_sample = 0; wide_sample < wide_samples; wide_sample++)
			for(channel = 0; channel < channels; channel++) {
				if(out[channel][wide_sample] & mask) {
					flac__utils_printf(stderr, 1, "ERROR during read, sample data (channel#%u sample#%u = %d) has non-zero least-significant bits\n  WAVE/AIFF header said the last %u bits are not significant and should be zero.\n", channel, wide_sample, out[channel][wide_sample], shift);
					return false;
				}
				out[channel][wide_sample] >>= shift;
			}
	}
	return true;
}

void encoder_progress_callback(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
{
	EncoderSession *encoder_session = (EncoderSession*)client_data;

	(void)encoder, (void)total_frames_estimate;

	encoder_session->bytes_written = bytes_written;
	encoder_session->samples_written = samples_written;

	if(encoder_session->total_samples_to_encode > 0 && !((frames_written-1) & encoder_session->stats_mask))
		print_stats(encoder_session);
}

FLAC__StreamDecoderReadStatus flac_decoder_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	size_t n = 0;
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	(void)decoder;

	if (data->fatal_error)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	/* use up lookahead first */
	if (data->lookahead_length) {
		n = min(data->lookahead_length, *bytes);
		memcpy(buffer, data->lookahead, n);
		buffer += n;
		data->lookahead += n;
		data->lookahead_length -= n;
	}

	/* get the rest from file */
	if (*bytes > n) {
		*bytes = n + fread(buffer, 1, *bytes-n, data->encoder_session->fin);
		if(ferror(data->encoder_session->fin))
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		else if(0 == *bytes)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus flac_decoder_seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	(void)decoder;

	if(fseeko(data->encoder_session->fin, (off_t)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus flac_decoder_tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	off_t pos;
	(void)decoder;

	if((pos = ftello(data->encoder_session->fin)) < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__StreamDecoderLengthStatus flac_decoder_length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	(void)decoder;

	if(0 == data->filesize)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = (FLAC__uint64)data->filesize;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool flac_decoder_eof_callback(const FLAC__StreamDecoder *decoder, void *client_data)
{
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	(void)decoder;

	return feof(data->encoder_session->fin)? true : false;
}

FLAC__StreamDecoderWriteStatus flac_decoder_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	FLAC__uint64 n = min(data->samples_left_to_process, frame->header.blocksize);
	(void)decoder;

	if(!EncoderSession_process(data->encoder_session, buffer, (unsigned)n)) {
		print_error_with_state(data->encoder_session, "ERROR during encoding");
		data->fatal_error = true;
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	data->samples_left_to_process -= n;
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flac_decoder_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	(void)decoder;

	if (data->fatal_error)
		return;

	if (
		data->num_metadata_blocks == sizeof(data->metadata_blocks)/sizeof(data->metadata_blocks[0]) ||
		0 == (data->metadata_blocks[data->num_metadata_blocks] = FLAC__metadata_object_clone(metadata))
	)
		data->fatal_error = true;
	else
		data->num_metadata_blocks++;
}

void flac_decoder_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLACDecoderData *data = (FLACDecoderData*)client_data;
	(void)decoder;

	flac__utils_printf(stderr, 1, "%s: ERROR got %s while decoding FLAC input\n", data->encoder_session->inbasefilename, FLAC__StreamDecoderErrorStatusString[status]);
	if(!data->encoder_session->continue_through_decode_errors)
		data->fatal_error = true;
}

FLAC__bool parse_cuesheet(FLAC__StreamMetadata **cuesheet, const char *cuesheet_filename, const char *inbasefilename, FLAC__bool is_cdda, FLAC__uint64 lead_out_offset, FLAC__bool treat_warnings_as_errors)
{
	FILE *f;
	unsigned last_line_read;
	const char *error_message;

	if(0 == cuesheet_filename)
		return true;

	if(lead_out_offset == 0) {
		flac__utils_printf(stderr, 1, "%s: ERROR cannot import cuesheet when the number of input samples to encode is unknown\n", inbasefilename);
		return false;
	}

	if(0 == (f = fopen(cuesheet_filename, "r"))) {
		flac__utils_printf(stderr, 1, "%s: ERROR opening cuesheet \"%s\" for reading: %s\n", inbasefilename, cuesheet_filename, strerror(errno));
		return false;
	}

	*cuesheet = grabbag__cuesheet_parse(f, &error_message, &last_line_read, is_cdda, lead_out_offset);

	fclose(f);

	if(0 == *cuesheet) {
		flac__utils_printf(stderr, 1, "%s: ERROR parsing cuesheet \"%s\" on line %u: %s\n", inbasefilename, cuesheet_filename, last_line_read, error_message);
		return false;
	}

	if(!FLAC__format_cuesheet_is_legal(&(*cuesheet)->data.cue_sheet, /*check_cd_da_subset=*/false, &error_message)) {
		flac__utils_printf(stderr, 1, "%s: ERROR parsing cuesheet \"%s\": %s\n", inbasefilename, cuesheet_filename, error_message);
		return false;
	}

	/* if we're expecting CDDA, warn about non-compliance */
	if(is_cdda && !FLAC__format_cuesheet_is_legal(&(*cuesheet)->data.cue_sheet, /*check_cd_da_subset=*/true, &error_message)) {
		flac__utils_printf(stderr, 1, "%s: WARNING cuesheet \"%s\" is not audio CD compliant: %s\n", inbasefilename, cuesheet_filename, error_message);
		if(treat_warnings_as_errors)
			return false;
		(*cuesheet)->data.cue_sheet.is_cd = false;
	}

	return true;
}

void print_stats(const EncoderSession *encoder_session)
{
	const FLAC__uint64 samples_written = min(encoder_session->total_samples_to_encode, encoder_session->samples_written);
#if defined _MSC_VER || defined __MINGW32__
	/* with MSVC you have to spoon feed it the casting */
	const double progress = (double)(FLAC__int64)samples_written / (double)(FLAC__int64)encoder_session->total_samples_to_encode;
	const double ratio = (double)(FLAC__int64)encoder_session->bytes_written / ((double)(FLAC__int64)encoder_session->unencoded_size * min(1.0, progress));
#else
	const double progress = (double)samples_written / (double)encoder_session->total_samples_to_encode;
	const double ratio = (double)encoder_session->bytes_written / ((double)encoder_session->unencoded_size * min(1.0, progress));
#endif

	FLAC__ASSERT(encoder_session->total_samples_to_encode > 0);

	if(samples_written == encoder_session->total_samples_to_encode) {
		flac__utils_printf(stderr, 2, "\r%s:%s wrote %u bytes, ratio=%0.3f",
			encoder_session->inbasefilename,
			encoder_session->verify? " Verify OK," : "",
			(unsigned)encoder_session->bytes_written,
			ratio
		);
	}
	else {
		flac__utils_printf(stderr, 2, "\r%s: %u%% complete, ratio=%0.3f", encoder_session->inbasefilename, (unsigned)floor(progress * 100.0 + 0.5), ratio);
	}
}

void print_error_with_init_status(const EncoderSession *e, const char *message, FLAC__StreamEncoderInitStatus init_status)
{
	const int ilen = strlen(e->inbasefilename) + 1;
	const char *state_string = "";

	flac__utils_printf(stderr, 1, "\n%s: %s\n", e->inbasefilename, message);

	flac__utils_printf(stderr, 1, "%*s init_status = %s\n", ilen, "", FLAC__StreamEncoderInitStatusString[init_status]);

	if(init_status == FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR) {
		state_string = FLAC__stream_encoder_get_resolved_state_string(e->encoder);

		flac__utils_printf(stderr, 1, "%*s state = %s\n", ilen, "", state_string);

		/* print out some more info for some errors: */
		if(0 == strcmp(state_string, FLAC__StreamEncoderStateString[FLAC__STREAM_ENCODER_CLIENT_ERROR])) {
			flac__utils_printf(stderr, 1,
				"\n"
				"An error occurred while writing; the most common cause is that the disk is full.\n"
			);
		}
		else if(0 == strcmp(state_string, FLAC__StreamEncoderStateString[FLAC__STREAM_ENCODER_IO_ERROR])) {
			flac__utils_printf(stderr, 1,
				"\n"
				"An error occurred opening the output file; it is likely that the output\n"
				"directory does not exist or is not writable, the output file already exists and\n"
				"is not writable, or the disk is full.\n"
			);
		}
	}
	else if(init_status == FLAC__STREAM_ENCODER_INIT_STATUS_NOT_STREAMABLE) {
		flac__utils_printf(stderr, 1,
			"\n"
			"The encoding parameters specified do not conform to the FLAC Subset and may not\n"
			"be streamable or playable in hardware devices.  If you really understand the\n"
			"consequences, you can add --lax to the command-line options to encode with\n"
			"these parameters anyway.  See http://flac.sourceforge.net/format.html#subset\n"
		);
	}
}

void print_error_with_state(const EncoderSession *e, const char *message)
{
	const int ilen = strlen(e->inbasefilename) + 1;
	const char *state_string;

	flac__utils_printf(stderr, 1, "\n%s: %s\n", e->inbasefilename, message);

	state_string = FLAC__stream_encoder_get_resolved_state_string(e->encoder);

	flac__utils_printf(stderr, 1, "%*s state = %s\n", ilen, "", state_string);

	/* print out some more info for some errors: */
	if(0 == strcmp(state_string, FLAC__StreamEncoderStateString[FLAC__STREAM_ENCODER_CLIENT_ERROR])) {
		flac__utils_printf(stderr, 1,
			"\n"
			"An error occurred while writing; the most common cause is that the disk is full.\n"
		);
	}
}

void print_verify_error(EncoderSession *e)
{
	FLAC__uint64 absolute_sample;
	unsigned frame_number;
	unsigned channel;
	unsigned sample;
	FLAC__int32 expected;
	FLAC__int32 got;

	FLAC__stream_encoder_get_verify_decoder_error_stats(e->encoder, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);

	flac__utils_printf(stderr, 1, "%s: ERROR: mismatch in decoded data, verify FAILED!\n", e->inbasefilename);
	flac__utils_printf(stderr, 1, "       Absolute sample=%u, frame=%u, channel=%u, sample=%u, expected %d, got %d\n", (unsigned)absolute_sample, frame_number, channel, sample, expected, got);
	flac__utils_printf(stderr, 1, "       In all known cases, verify errors are caused by hardware problems,\n");
	flac__utils_printf(stderr, 1, "       usually overclocking or bad RAM.  Delete %s\n", e->outfilename);
	flac__utils_printf(stderr, 1, "       and repeat the flac command exactly as before.  If it does not give a\n");
	flac__utils_printf(stderr, 1, "       verify error in the exact same place each time you try it, then there is\n");
	flac__utils_printf(stderr, 1, "       a problem with your hardware; please see the FAQ:\n");
	flac__utils_printf(stderr, 1, "           http://flac.sourceforge.net/faq.html#tools__hardware_prob\n");
	flac__utils_printf(stderr, 1, "       If it does fail in the exact same place every time, keep\n");
	flac__utils_printf(stderr, 1, "       %s and submit a bug report to:\n", e->outfilename);
	flac__utils_printf(stderr, 1, "           https://sourceforge.net/bugs/?func=addbug&group_id=13478\n");
	flac__utils_printf(stderr, 1, "       Make sure to upload the FLAC file and use the \"Monitor\" feature to\n");
	flac__utils_printf(stderr, 1, "       monitor the bug status.\n");
	flac__utils_printf(stderr, 1, "Verify FAILED!  Do not trust %s\n", e->outfilename);
}

FLAC__bool read_little_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn)
{
	size_t bytes_read = fread(val, 1, 2, f);

	if(bytes_read == 0) {
		if(!eof_ok) {
			flac__utils_printf(stderr, 1, "%s: ERROR: unexpected EOF\n", fn);
			return false;
		}
		else
			return true;
	}
	else if(bytes_read < 2) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}
	else {
		if(is_big_endian_host_) {
			FLAC__byte tmp, *b = (FLAC__byte*)val;
			tmp = b[1]; b[1] = b[0]; b[0] = tmp;
		}
		return true;
	}
}

FLAC__bool read_little_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn)
{
	size_t bytes_read = fread(val, 1, 4, f);

	if(bytes_read == 0) {
		if(!eof_ok) {
			flac__utils_printf(stderr, 1, "%s: ERROR: unexpected EOF\n", fn);
			return false;
		}
		else
			return true;
	}
	else if(bytes_read < 4) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}
	else {
		if(is_big_endian_host_) {
			FLAC__byte tmp, *b = (FLAC__byte*)val;
			tmp = b[3]; b[3] = b[0]; b[0] = tmp;
			tmp = b[2]; b[2] = b[1]; b[1] = tmp;
		}
		return true;
	}
}

FLAC__bool read_big_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn)
{
	unsigned char buf[4];
	size_t bytes_read= fread(buf, 1, 2, f);

	if(bytes_read==0U && eof_ok)
		return true;
	else if(bytes_read<2U) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}

	/* this is independent of host endianness */
	*val= (FLAC__uint16)(buf[0])<<8 | buf[1];

	return true;
}

FLAC__bool read_big_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn)
{
	unsigned char buf[4];
	size_t bytes_read= fread(buf, 1, 4, f);

	if(bytes_read==0U && eof_ok)
		return true;
	else if(bytes_read<4U) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}

	/* this is independent of host endianness */
	*val= (FLAC__uint32)(buf[0])<<24 | (FLAC__uint32)(buf[1])<<16 |
		(FLAC__uint32)(buf[2])<<8 | buf[3];

	return true;
}

FLAC__bool read_sane_extended(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn)
	/* Read an IEEE 754 80-bit (aka SANE) extended floating point value from 'f',
	 * convert it into an integral value and store in 'val'.  Return false if only
	 * between 1 and 9 bytes remain in 'f', if 0 bytes remain in 'f' and 'eof_ok' is
	 * false, or if the value is negative, between zero and one, or too large to be
	 * represented by 'val'; return true otherwise.
	 */
{
	unsigned int i;
	unsigned char buf[10];
	size_t bytes_read= fread(buf, 1U, 10U, f);
	FLAC__int16 e= ((FLAC__uint16)(buf[0])<<8 | (FLAC__uint16)(buf[1]))-0x3FFF;
	FLAC__int16 shift= 63-e;
	FLAC__uint64 p= 0U;

	if(bytes_read==0U && eof_ok)
		return true;
	else if(bytes_read<10U) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}
	else if((buf[0]>>7)==1U || e<0 || e>63) {
		flac__utils_printf(stderr, 1, "%s: ERROR: invalid floating-point value\n", fn);
		return false;
	}

	for(i= 0U; i<8U; ++i)
		p|= (FLAC__uint64)(buf[i+2])<<(56U-i*8);
	*val= (FLAC__uint32)((p>>shift)+(p>>(shift-1) & 0x1));

	return true;
}

FLAC__bool fskip_ahead(FILE *f, FLAC__uint64 offset)
{
	static unsigned char dump[8192];

#ifdef _MSC_VER
	if(f == stdin) {
		/* MS' stdio impl can't even seek forward on stdin, have to use pure non-fseek() version: */
		while(offset > 0) {
			const long need = (long)min(offset, sizeof(dump));
			if((long)fread(dump, 1, need, f) < need)
				return false;
			offset -= need;
		}
	}
	else
#endif
	{
		while(offset > 0) {
			long need = (long)min(offset, LONG_MAX);
			if(fseeko(f, need, SEEK_CUR) < 0) {
				need = (long)min(offset, sizeof(dump));
				if((long)fread(dump, 1, need, f) < need)
					return false;
			}
			offset -= need;
		}
	}
	return true;
}

unsigned count_channel_mask_bits(FLAC__uint32 mask)
{
	unsigned count = 0;
	while(mask) {
		if(mask & 1)
			count++;
		mask >>= 1;
	}
	return count;
}

#if 0
FLAC__uint32 limit_channel_mask(FLAC__uint32 mask, unsigned channels)
{
	FLAC__uint32 x = 0x80000000;
	unsigned count = count_channel_mask_bits(mask);
	while(x && count > channels) {
		if(mask & x) {
			mask &= ~x;
			count--;
		}
		x >>= 1;
	}
	FLAC__ASSERT(count_channel_mask_bits(mask) == channels);
	return mask;
}
#endif
