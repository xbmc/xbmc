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

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined _MSC_VER && !defined __MINGW32__
/* unlink is in stdio.h in VC++ */
#include <unistd.h> /* for unlink() */
#endif
#include "FLAC/all.h"
#include "share/alloc.h"
#include "share/grabbag.h"
#include "analyze.h"
#include "decode.h"
#include "encode.h"
#include "local_string_utils.h" /* for flac__strlcat() and flac__strlcpy() */
#include "utils.h"
#include "vorbiscomment.h"

#if defined _MSC_VER || defined __MINGW32__ || defined __EMX__
#define FLAC__STRCASECMP stricmp
#else
#define FLAC__STRCASECMP strcasecmp
#endif

#if 0
/*[JEC] was:#if HAVE_GETOPT_LONG*/
/*[JEC] see flac/include/share/getopt.h as to why the change */
#  include <getopt.h>
#else
#  include "share/getopt.h"
#endif

typedef enum { RAW, WAV, AIF, FLAC, OGGFLAC } FileFormat;

static int do_it(void);

static FLAC__bool init_options(void);
static int parse_options(int argc, char *argv[]);
static int parse_option(int short_option, const char *long_option, const char *option_argument);
static void free_options(void);
static void add_compression_setting_bool(compression_setting_type_t type, FLAC__bool value);
static void add_compression_setting_string(compression_setting_type_t type, const char *value);
static void add_compression_setting_unsigned(compression_setting_type_t type, unsigned value);

static int usage_error(const char *message, ...);
static void short_usage(void);
static void show_version(void);
static void show_help(void);
static void show_explain(void);
static void format_mistake(const char *infilename, FileFormat wrong, FileFormat right);

static int encode_file(const char *infilename, FLAC__bool is_first_file, FLAC__bool is_last_file);
static int decode_file(const char *infilename);

static const char *get_encoded_outfilename(const char *infilename);
static const char *get_decoded_outfilename(const char *infilename);
static const char *get_outfilename(const char *infilename, const char *suffix);

static void die(const char *message);
static int conditional_fclose(FILE *f);
static char *local_strdup(const char *source);
#ifdef _MSC_VER
/* There's no strtoll() in MSVC6 so we just write a specialized one */
static FLAC__int64 local__strtoll(const char *src, char **endptr);
#endif


/*
 * share__getopt format struct; note that for long options with no
 * short option equivalent we just set the 'val' field to 0.
 */
static struct share__option long_options_[] = {
	/*
	 * general options
	 */
	{ "help"                  , share__no_argument, 0, 'h' },
	{ "explain"               , share__no_argument, 0, 'H' },
	{ "version"               , share__no_argument, 0, 'v' },
	{ "decode"                , share__no_argument, 0, 'd' },
	{ "analyze"               , share__no_argument, 0, 'a' },
	{ "test"                  , share__no_argument, 0, 't' },
	{ "stdout"                , share__no_argument, 0, 'c' },
	{ "silent"                , share__no_argument, 0, 's' },
	{ "totally-silent"        , share__no_argument, 0, 0 },
	{ "warnings-as-errors"    , share__no_argument, 0, 'w' },
	{ "force"                 , share__no_argument, 0, 'f' },
	{ "delete-input-file"     , share__no_argument, 0, 0 },
	{ "keep-foreign-metadata" , share__no_argument, 0, 0 },
	{ "output-prefix"         , share__required_argument, 0, 0 },
	{ "output-name"           , share__required_argument, 0, 'o' },
	{ "skip"                  , share__required_argument, 0, 0 },
	{ "until"                 , share__required_argument, 0, 0 },
	{ "channel-map"           , share__required_argument, 0, 0 }, /* undocumented */

	/*
	 * decoding options
	 */
	{ "decode-through-errors", share__no_argument, 0, 'F' },
	{ "cue"                  , share__required_argument, 0, 0 },
	{ "apply-replaygain-which-is-not-lossless", share__optional_argument, 0, 0 }, /* undocumented */

	/*
	 * encoding options
	 */
	{ "cuesheet"                  , share__required_argument, 0, 0 },
	{ "no-cued-seekpoints"        , share__no_argument, 0, 0 },
	{ "picture"                   , share__required_argument, 0, 0 },
	{ "tag"                       , share__required_argument, 0, 'T' },
	{ "tag-from-file"             , share__required_argument, 0, 0 },
	{ "compression-level-0"       , share__no_argument, 0, '0' },
	{ "compression-level-1"       , share__no_argument, 0, '1' },
	{ "compression-level-2"       , share__no_argument, 0, '2' },
	{ "compression-level-3"       , share__no_argument, 0, '3' },
	{ "compression-level-4"       , share__no_argument, 0, '4' },
	{ "compression-level-5"       , share__no_argument, 0, '5' },
	{ "compression-level-6"       , share__no_argument, 0, '6' },
	{ "compression-level-7"       , share__no_argument, 0, '7' },
	{ "compression-level-8"       , share__no_argument, 0, '8' },
	{ "compression-level-9"       , share__no_argument, 0, '9' },
	{ "best"                      , share__no_argument, 0, '8' },
	{ "fast"                      , share__no_argument, 0, '0' },
	{ "verify"                    , share__no_argument, 0, 'V' },
	{ "force-aiff-format"         , share__no_argument, 0, 0 },
	{ "force-raw-format"          , share__no_argument, 0, 0 },
	{ "lax"                       , share__no_argument, 0, 0 },
	{ "replay-gain"               , share__no_argument, 0, 0 },
	{ "ignore-chunk-sizes"        , share__no_argument, 0, 0 },
	{ "sector-align"              , share__no_argument, 0, 0 },
	{ "seekpoint"                 , share__required_argument, 0, 'S' },
	{ "padding"                   , share__required_argument, 0, 'P' },
#if FLAC__HAS_OGG
	{ "ogg"                       , share__no_argument, 0, 0 },
	{ "serial-number"             , share__required_argument, 0, 0 },
#endif
	{ "blocksize"                 , share__required_argument, 0, 'b' },
	{ "exhaustive-model-search"   , share__no_argument, 0, 'e' },
	{ "max-lpc-order"             , share__required_argument, 0, 'l' },
	{ "apodization"               , share__required_argument, 0, 'A' },
	{ "mid-side"                  , share__no_argument, 0, 'm' },
	{ "adaptive-mid-side"         , share__no_argument, 0, 'M' },
	{ "qlp-coeff-precision-search", share__no_argument, 0, 'p' },
	{ "qlp-coeff-precision"       , share__required_argument, 0, 'q' },
	{ "rice-partition-order"      , share__required_argument, 0, 'r' },
	{ "endian"                    , share__required_argument, 0, 0 },
	{ "channels"                  , share__required_argument, 0, 0 },
	{ "bps"                       , share__required_argument, 0, 0 },
	{ "sample-rate"               , share__required_argument, 0, 0 },
	{ "sign"                      , share__required_argument, 0, 0 },
	{ "input-size"                , share__required_argument, 0, 0 },

	/*
	 * analysis options
	 */
	{ "residual-gnuplot", share__no_argument, 0, 0 },
	{ "residual-text", share__no_argument, 0, 0 },

	/*
	 * negatives
	 */
	{ "no-decode-through-errors"  , share__no_argument, 0, 0 },
	{ "no-silent"                 , share__no_argument, 0, 0 },
	{ "no-force"                  , share__no_argument, 0, 0 },
	{ "no-seektable"              , share__no_argument, 0, 0 },
	{ "no-delete-input-file"      , share__no_argument, 0, 0 },
	{ "no-keep-foreign-metadata"  , share__no_argument, 0, 0 },
	{ "no-replay-gain"            , share__no_argument, 0, 0 },
	{ "no-ignore-chunk-sizes"     , share__no_argument, 0, 0 },
	{ "no-sector-align"           , share__no_argument, 0, 0 },
	{ "no-utf8-convert"           , share__no_argument, 0, 0 },
	{ "no-lax"                    , share__no_argument, 0, 0 },
#if FLAC__HAS_OGG
	{ "no-ogg"                    , share__no_argument, 0, 0 },
#endif
	{ "no-exhaustive-model-search", share__no_argument, 0, 0 },
	{ "no-mid-side"               , share__no_argument, 0, 0 },
	{ "no-adaptive-mid-side"      , share__no_argument, 0, 0 },
	{ "no-qlp-coeff-prec-search"  , share__no_argument, 0, 0 },
	{ "no-padding"                , share__no_argument, 0, 0 },
	{ "no-verify"                 , share__no_argument, 0, 0 },
	{ "no-warnings-as-errors"     , share__no_argument, 0, 0 },
	{ "no-residual-gnuplot"       , share__no_argument, 0, 0 },
	{ "no-residual-text"          , share__no_argument, 0, 0 },
	/*
	 * undocumented debugging options for the test suite
	 */
	{ "disable-constant-subframes", share__no_argument, 0, 0 },
	{ "disable-fixed-subframes"   , share__no_argument, 0, 0 },
	{ "disable-verbatim-subframes", share__no_argument, 0, 0 },
	{ "no-md5-sum"                , share__no_argument, 0, 0 },

	{0, 0, 0, 0}
};


/*
 * global to hold command-line option values
 */

static struct {
	FLAC__bool show_help;
	FLAC__bool show_explain;
	FLAC__bool show_version;
	FLAC__bool mode_decode;
	FLAC__bool verify;
	FLAC__bool treat_warnings_as_errors;
	FLAC__bool force_file_overwrite;
	FLAC__bool continue_through_decode_errors;
	replaygain_synthesis_spec_t replaygain_synthesis_spec;
	FLAC__bool lax;
	FLAC__bool test_only;
	FLAC__bool analyze;
	FLAC__bool use_ogg;
	FLAC__bool has_serial_number; /* true iff --serial-number was used */
	long serial_number; /* this is the Ogg serial number and is unused for native FLAC */
	FLAC__bool force_to_stdout;
	FLAC__bool force_aiff_format;
	FLAC__bool force_raw_format;
	FLAC__bool delete_input;
	FLAC__bool keep_foreign_metadata;
	FLAC__bool replay_gain;
	FLAC__bool ignore_chunk_sizes;
	FLAC__bool sector_align;
	FLAC__bool utf8_convert; /* true by default, to convert tag strings from locale to utf-8, false if --no-utf8-convert used */
	const char *cmdline_forced_outfilename;
	const char *output_prefix;
	analysis_options aopts;
	int padding; /* -1 => no -P options were given, 0 => -P- was given, else -P value */
	size_t num_compression_settings;
	compression_setting_t compression_settings[64]; /* bad MAGIC NUMBER but buffer overflow is checked */
	const char *skip_specification;
	const char *until_specification;
	const char *cue_specification;
	int format_is_big_endian;
	int format_is_unsigned_samples;
	int format_channels;
	int format_bps;
	int format_sample_rate;
	off_t format_input_size;
	char requested_seek_points[5000]; /* bad MAGIC NUMBER but buffer overflow is checked */
	int num_requested_seek_points; /* -1 => no -S options were given, 0 => -S- was given */
	const char *cuesheet_filename;
	FLAC__bool cued_seekpoints;
	FLAC__bool channel_map_none; /* --channel-map=none specified, eventually will expand to take actual channel map */

	unsigned num_files;
	char **filenames;

	FLAC__StreamMetadata *vorbis_comment;
	FLAC__StreamMetadata *pictures[64];
	unsigned num_pictures;

	struct {
		FLAC__bool disable_constant_subframes;
		FLAC__bool disable_fixed_subframes;
		FLAC__bool disable_verbatim_subframes;
		FLAC__bool do_md5;
	} debug;
} option_values;


/*
 * miscellaneous globals
 */

static FLAC__int32 align_reservoir_0[588], align_reservoir_1[588]; /* for carrying over samples from --sector-align */
static FLAC__int32 *align_reservoir[2] = { align_reservoir_0, align_reservoir_1 };
static unsigned align_reservoir_samples = 0; /* 0 .. 587 */


int main(int argc, char *argv[])
{
	int retval = 0;

#ifdef __EMX__
	_response(&argc, &argv);
	_wildcard(&argc, &argv);
#endif

	srand((unsigned)time(0));
	setlocale(LC_ALL, "");
	if(!init_options()) {
		flac__utils_printf(stderr, 1, "ERROR: allocating memory\n");
		retval = 1;
	}
	else {
		if((retval = parse_options(argc, argv)) == 0)
			retval = do_it();
	}

	free_options();

	return retval;
}

int do_it(void)
{
	int retval = 0;

	if(option_values.show_version) {
		show_version();
		return 0;
	}
	else if(option_values.show_explain) {
		show_explain();
		return 0;
	}
	else if(option_values.show_help) {
		show_help();
		return 0;
	}
	else {
		if(option_values.num_files == 0) {
			if(flac__utils_verbosity_ >= 1)
				short_usage();
			return 0;
		}

		/*
		 * tweak options; validate the values
		 */
		if(!option_values.mode_decode) {
			if(0 != option_values.cue_specification)
				return usage_error("ERROR: --cue is not allowed in test mode\n");
		}
		else {
			if(option_values.test_only) {
				if(0 != option_values.skip_specification)
					return usage_error("ERROR: --skip is not allowed in test mode\n");
				if(0 != option_values.until_specification)
					return usage_error("ERROR: --until is not allowed in test mode\n");
				if(0 != option_values.cue_specification)
					return usage_error("ERROR: --cue is not allowed in test mode\n");
				if(0 != option_values.analyze)
					return usage_error("ERROR: analysis mode (-a/--analyze) and test mode (-t/--test) cannot be used together\n");
			}
		}

		if(0 != option_values.cue_specification && (0 != option_values.skip_specification || 0 != option_values.until_specification))
			return usage_error("ERROR: --cue may not be combined with --skip or --until\n");

		if(option_values.format_channels >= 0) {
			if(option_values.format_channels == 0 || (unsigned)option_values.format_channels > FLAC__MAX_CHANNELS)
				return usage_error("ERROR: invalid number of channels '%u', must be > 0 and <= %u\n", option_values.format_channels, FLAC__MAX_CHANNELS);
		}
		if(option_values.format_bps >= 0) {
			if(option_values.format_bps != 8 && option_values.format_bps != 16 && option_values.format_bps != 24)
				return usage_error("ERROR: invalid bits per sample '%u' (must be 8/16/24)\n", option_values.format_bps);
		}
		if(option_values.format_sample_rate >= 0) {
			if(!FLAC__format_sample_rate_is_valid(option_values.format_sample_rate))
				return usage_error("ERROR: invalid sample rate '%u', must be > 0 and <= %u\n", option_values.format_sample_rate, FLAC__MAX_SAMPLE_RATE);
		}
		if(option_values.force_raw_format && option_values.force_aiff_format)
			return usage_error("ERROR: only one of --force-raw-format and --force-aiff-format allowed\n");
		if(option_values.mode_decode) {
			if(!option_values.force_raw_format) {
				if(option_values.format_is_big_endian >= 0)
					return usage_error("ERROR: --endian only allowed with --force-raw-format\n");
				if(option_values.format_is_unsigned_samples >= 0)
					return usage_error("ERROR: --sign only allowed with --force-raw-format\n");
			}
			if(option_values.format_channels >= 0)
				return usage_error("ERROR: --channels not allowed with --decode\n");
			if(option_values.format_bps >= 0)
				return usage_error("ERROR: --bps not allowed with --decode\n");
			if(option_values.format_sample_rate >= 0)
				return usage_error("ERROR: --sample-rate not allowed with --decode\n");
		}

		if(option_values.ignore_chunk_sizes) {
			if(option_values.mode_decode)
				return usage_error("ERROR: --ignore-chunk-sizes only allowed for encoding\n");
			if(0 != option_values.sector_align)
				return usage_error("ERROR: --ignore-chunk-sizes not allowed with --sector-align\n");
			if(0 != option_values.until_specification)
				return usage_error("ERROR: --ignore-chunk-sizes not allowed with --until\n");
			if(0 != option_values.cue_specification)
				return usage_error("ERROR: --ignore-chunk-sizes not allowed with --cue\n");
			if(0 != option_values.cuesheet_filename)
				return usage_error("ERROR: --ignore-chunk-sizes not allowed with --cuesheet\n");
		}
		if(option_values.sector_align) {
			if(option_values.mode_decode)
				return usage_error("ERROR: --sector-align only allowed for encoding\n");
			if(0 != option_values.skip_specification)
				return usage_error("ERROR: --sector-align not allowed with --skip\n");
			if(0 != option_values.until_specification)
				return usage_error("ERROR: --sector-align not allowed with --until\n");
			if(0 != option_values.cue_specification)
				return usage_error("ERROR: --sector-align not allowed with --cue\n");
			if(option_values.format_channels >= 0 && option_values.format_channels != 2)
				return usage_error("ERROR: --sector-align can only be done with stereo input\n");
			if(option_values.format_bps >= 0 && option_values.format_bps != 16)
				return usage_error("ERROR: --sector-align can only be done with 16-bit samples\n");
			if(option_values.format_sample_rate >= 0 && option_values.format_sample_rate != 44100)
				return usage_error("ERROR: --sector-align can only be done with a sample rate of 44100\n");
		}
		if(option_values.replay_gain) {
			if(option_values.force_to_stdout)
				return usage_error("ERROR: --replay-gain not allowed with -c/--stdout\n");
			if(option_values.mode_decode)
				return usage_error("ERROR: --replay-gain only allowed for encoding\n");
			if(option_values.format_channels > 2)
				return usage_error("ERROR: --replay-gain can only be done with mono/stereo input\n");
			if(option_values.format_sample_rate >= 0 && !grabbag__replaygain_is_valid_sample_frequency(option_values.format_sample_rate))
				return usage_error("ERROR: invalid sample rate used with --replay-gain\n");
			/*
			 * We want to reserve padding space for the ReplayGain
			 * tags that we will set later, to avoid rewriting the
			 * whole file.
			 */
			if(
				(option_values.padding >= 0 && option_values.padding < (int)GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED) ||
				(option_values.padding < 0 && FLAC_ENCODE__DEFAULT_PADDING < (int)GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED)
			) {
				flac__utils_printf(stderr, 1, "NOTE: --replay-gain may leave a small PADDING block even with --no-padding\n");
				option_values.padding = GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED;
			}
			else {
				option_values.padding += GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED;
			}
		}
		if(option_values.num_files > 1 && option_values.cmdline_forced_outfilename) {
			return usage_error("ERROR: -o/--output-name cannot be used with multiple files\n");
		}
		if(option_values.cmdline_forced_outfilename && option_values.output_prefix) {
			return usage_error("ERROR: --output-prefix conflicts with -o/--output-name\n");
		}
		if(!option_values.mode_decode && 0 != option_values.cuesheet_filename && option_values.num_files > 1) {
			return usage_error("ERROR: --cuesheet cannot be used when encoding multiple files\n");
		}
		if(option_values.keep_foreign_metadata) {
			/* we're not going to try and support the re-creation of broken WAVE files */
			if(option_values.ignore_chunk_sizes)
				return usage_error("ERROR: using --keep-foreign-metadata cannot be used with --ignore-chunk-sizes\n");
			if(option_values.test_only)
				return usage_error("ERROR: --keep-foreign-metadata is not allowed in test mode\n");
			if(option_values.analyze)
				return usage_error("ERROR: --keep-foreign-metadata is not allowed in analyis mode\n");
			/*@@@@@@*/
			if(option_values.delete_input)
				return usage_error("ERROR: using --delete-input-file with --keep-foreign-metadata has been disabled until more testing has been done.\n");
			flac__utils_printf(stderr, 1, "NOTE: --keep-foreign-metadata is a new feature; make sure to test the output file before deleting the original.\n");
		}
	}

	flac__utils_printf(stderr, 2, "\n");
	flac__utils_printf(stderr, 2, "flac %s, Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson\n", FLAC__VERSION_STRING);
	flac__utils_printf(stderr, 2, "flac comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are\n");
	flac__utils_printf(stderr, 2, "welcome to redistribute it under certain conditions.  Type `flac' for details.\n\n");

	if(option_values.mode_decode) {
		FLAC__bool first = true;

		if(option_values.num_files == 0) {
			retval = decode_file("-");
		}
		else {
			unsigned i;
			if(option_values.num_files > 1)
				option_values.cmdline_forced_outfilename = 0;
			for(i = 0, retval = 0; i < option_values.num_files; i++) {
				if(0 == strcmp(option_values.filenames[i], "-") && !first)
					continue;
				retval |= decode_file(option_values.filenames[i]);
				first = false;
			}
		}
	}
	else { /* encode */
		FLAC__bool first = true;

		if(option_values.ignore_chunk_sizes)
			flac__utils_printf(stderr, 1, "INFO: Make sure you know what you're doing when using --ignore-chunk-sizes.\n      Improper use can cause flac to encode non-audio data as audio.\n");

		if(option_values.num_files == 0) {
			retval = encode_file("-", first, true);
		}
		else {
			unsigned i;
			if(option_values.num_files > 1)
				option_values.cmdline_forced_outfilename = 0;
			for(i = 0, retval = 0; i < option_values.num_files; i++) {
				if(0 == strcmp(option_values.filenames[i], "-") && !first)
					continue;
				retval |= encode_file(option_values.filenames[i], first, i == (option_values.num_files-1));
				first = false;
			}
			if(option_values.replay_gain && retval == 0) {
				float album_gain, album_peak;
				grabbag__replaygain_get_album(&album_gain, &album_peak);
				for(i = 0; i < option_values.num_files; i++) {
					const char *error, *outfilename = get_encoded_outfilename(option_values.filenames[i]);
					if(0 == outfilename) {
						flac__utils_printf(stderr, 1, "ERROR: filename too long: %s", option_values.filenames[i]);
						return 1;
					}
					if(0 == strcmp(option_values.filenames[i], "-")) {
						FLAC__ASSERT(0);
						/* double protection */
						flac__utils_printf(stderr, 1, "internal error\n");
						return 2;
					}
					if(0 != (error = grabbag__replaygain_store_to_file_album(outfilename, album_gain, album_peak, /*preserve_modtime=*/true))) {
						flac__utils_printf(stderr, 1, "%s: ERROR writing ReplayGain album tags (%s)\n", outfilename, error);
						retval = 1;
					}
				}
			}
		}
	}

	return retval;
}

FLAC__bool init_options(void)
{
	option_values.show_help = false;
	option_values.show_explain = false;
	option_values.mode_decode = false;
	option_values.verify = false;
	option_values.treat_warnings_as_errors = false;
	option_values.force_file_overwrite = false;
	option_values.continue_through_decode_errors = false;
	option_values.replaygain_synthesis_spec.apply = false;
	option_values.replaygain_synthesis_spec.use_album_gain = true;
	option_values.replaygain_synthesis_spec.limiter = RGSS_LIMIT__HARD;
	option_values.replaygain_synthesis_spec.noise_shaping = NOISE_SHAPING_LOW;
	option_values.replaygain_synthesis_spec.preamp = 0.0;
	option_values.lax = false;
	option_values.test_only = false;
	option_values.analyze = false;
	option_values.use_ogg = false;
	option_values.has_serial_number = false;
	option_values.serial_number = 0;
	option_values.force_to_stdout = false;
	option_values.force_aiff_format = false;
	option_values.force_raw_format = false;
	option_values.delete_input = false;
	option_values.keep_foreign_metadata = false;
	option_values.replay_gain = false;
	option_values.ignore_chunk_sizes = false;
	option_values.sector_align = false;
	option_values.utf8_convert = true;
	option_values.cmdline_forced_outfilename = 0;
	option_values.output_prefix = 0;
	option_values.aopts.do_residual_text = false;
	option_values.aopts.do_residual_gnuplot = false;
	option_values.padding = -1;
	option_values.num_compression_settings = 1;
	option_values.compression_settings[0].type = CST_COMPRESSION_LEVEL;
	option_values.compression_settings[0].value.t_unsigned = 5;
	option_values.skip_specification = 0;
	option_values.until_specification = 0;
	option_values.cue_specification = 0;
	option_values.format_is_big_endian = -1;
	option_values.format_is_unsigned_samples = -1;
	option_values.format_channels = -1;
	option_values.format_bps = -1;
	option_values.format_sample_rate = -1;
	option_values.format_input_size = (off_t)(-1);
	option_values.requested_seek_points[0] = '\0';
	option_values.num_requested_seek_points = -1;
	option_values.cuesheet_filename = 0;
	option_values.cued_seekpoints = true;
	option_values.channel_map_none = false;

	option_values.num_files = 0;
	option_values.filenames = 0;

	if(0 == (option_values.vorbis_comment = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)))
		return false;
	option_values.num_pictures = 0;

	option_values.debug.disable_constant_subframes = false;
	option_values.debug.disable_fixed_subframes = false;
	option_values.debug.disable_verbatim_subframes = false;
	option_values.debug.do_md5 = true;

	return true;
}

int parse_options(int argc, char *argv[])
{
	int short_option;
	int option_index = 1;
	FLAC__bool had_error = false;
	const char *short_opts = "0123456789aA:b:cdefFhHl:mMo:pP:q:r:sS:tT:vVw";

	while ((short_option = share__getopt_long(argc, argv, short_opts, long_options_, &option_index)) != -1) {
		switch (short_option) {
			case 0: /* long option with no equivalent short option */
				had_error |= (parse_option(short_option, long_options_[option_index].name, share__optarg) != 0);
				break;
			case '?':
			case ':':
				had_error = true;
				break;
			default: /* short option */
				had_error |= (parse_option(short_option, 0, share__optarg) != 0);
				break;
		}
	}

	if(had_error) {
		return 1;
	}

	FLAC__ASSERT(share__optind <= argc);

	option_values.num_files = argc - share__optind;

	if(option_values.num_files > 0) {
		unsigned i = 0;
		if(0 == (option_values.filenames = (char**)malloc(sizeof(char*) * option_values.num_files)))
			die("out of memory allocating space for file names list");
		while(share__optind < argc)
			option_values.filenames[i++] = local_strdup(argv[share__optind++]);
	}

	return 0;
}

int parse_option(int short_option, const char *long_option, const char *option_argument)
{
	const char *violation;
	char *p;
	int i;

	if(short_option == 0) {
		FLAC__ASSERT(0 != long_option);
		if(0 == strcmp(long_option, "totally-silent")) {
			flac__utils_verbosity_ = 0;
		}
		else if(0 == strcmp(long_option, "delete-input-file")) {
			option_values.delete_input = true;
		}
		else if(0 == strcmp(long_option, "keep-foreign-metadata")) {
			option_values.keep_foreign_metadata = true;
		}
		else if(0 == strcmp(long_option, "output-prefix")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.output_prefix = option_argument;
		}
		else if(0 == strcmp(long_option, "skip")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.skip_specification = option_argument;
		}
		else if(0 == strcmp(long_option, "until")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.until_specification = option_argument;
		}
		else if(0 == strcmp(long_option, "input-size")) {
			FLAC__ASSERT(0 != option_argument);
			{
				char *end;
#ifdef _MSC_VER
				FLAC__int64 i;
				i = local__strtoll(option_argument, &end);
#else
				long long i;
				i = strtoll(option_argument, &end, 10);
#endif
				if(0 == strlen(option_argument) || *end)
					return usage_error("ERROR: --%s must be a number\n", long_option);
				option_values.format_input_size = (off_t)i;
				if(option_values.format_input_size != i) /* check if off_t is smaller than long long */
					return usage_error("ERROR: --%s too large; this build of flac does not support filesizes over 2GB\n", long_option);
				if(option_values.format_input_size <= 0)
					return usage_error("ERROR: --%s must be > 0\n", long_option);
			}
		}
		else if(0 == strcmp(long_option, "cue")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.cue_specification = option_argument;
		}
		else if(0 == strcmp(long_option, "apply-replaygain-which-is-not-lossless")) {
			option_values.replaygain_synthesis_spec.apply = true;
			if (0 != option_argument) {
				char *p;
				option_values.replaygain_synthesis_spec.limiter = RGSS_LIMIT__NONE;
				option_values.replaygain_synthesis_spec.noise_shaping = NOISE_SHAPING_NONE;
				option_values.replaygain_synthesis_spec.preamp = strtod(option_argument, &p);
				for ( ; *p; p++) {
					if (*p == 'a')
						option_values.replaygain_synthesis_spec.use_album_gain = true;
					else if (*p == 't')
						option_values.replaygain_synthesis_spec.use_album_gain = false;
					else if (*p == 'l')
						option_values.replaygain_synthesis_spec.limiter = RGSS_LIMIT__PEAK;
					else if (*p == 'L')
						option_values.replaygain_synthesis_spec.limiter = RGSS_LIMIT__HARD;
					else if (*p == 'n' && p[1] >= '0' && p[1] <= '3') {
						option_values.replaygain_synthesis_spec.noise_shaping = p[1] - '0';
						p++;
					}
					else
						return usage_error("ERROR: bad specification string \"%s\" for --%s\n", option_argument, long_option);
				}
			}
		}
		else if(0 == strcmp(long_option, "channel-map")) {
			if (0 == option_argument || strcmp(option_argument, "none"))
				return usage_error("ERROR: only --channel-map=none currently supported\n");
			option_values.channel_map_none = true;
		}
		else if(0 == strcmp(long_option, "cuesheet")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.cuesheet_filename = option_argument;
		}
		else if(0 == strcmp(long_option, "picture")) {
			const unsigned max_pictures = sizeof(option_values.pictures)/sizeof(option_values.pictures[0]);
			FLAC__ASSERT(0 != option_argument);
			if(option_values.num_pictures >= max_pictures)
				return usage_error("ERROR: too many --picture arguments, only %u allowed\n", max_pictures);
			if(0 == (option_values.pictures[option_values.num_pictures] = grabbag__picture_parse_specification(option_argument, &violation)))
				return usage_error("ERROR: (--picture) %s\n", violation);
			option_values.num_pictures++;
		}
		else if(0 == strcmp(long_option, "tag-from-file")) {
			FLAC__ASSERT(0 != option_argument);
			if(!flac__vorbiscomment_add(option_values.vorbis_comment, option_argument, /*value_from_file=*/true, /*raw=*/!option_values.utf8_convert, &violation))
				return usage_error("ERROR: (--tag-from-file) %s\n", violation);
		}
		else if(0 == strcmp(long_option, "no-cued-seekpoints")) {
			option_values.cued_seekpoints = false;
		}
		else if(0 == strcmp(long_option, "force-aiff-format")) {
			option_values.force_aiff_format = true;
		}
		else if(0 == strcmp(long_option, "force-raw-format")) {
			option_values.force_raw_format = true;
		}
		else if(0 == strcmp(long_option, "lax")) {
			option_values.lax = true;
		}
		else if(0 == strcmp(long_option, "replay-gain")) {
			option_values.replay_gain = true;
		}
		else if(0 == strcmp(long_option, "ignore-chunk-sizes")) {
			option_values.ignore_chunk_sizes = true;
		}
		else if(0 == strcmp(long_option, "sector-align")) {
			option_values.sector_align = true;
		}
#if FLAC__HAS_OGG
		else if(0 == strcmp(long_option, "ogg")) {
			option_values.use_ogg = true;
		}
		else if(0 == strcmp(long_option, "serial-number")) {
			option_values.has_serial_number = true;
			option_values.serial_number = atol(option_argument);
		}
#endif
		else if(0 == strcmp(long_option, "endian")) {
			FLAC__ASSERT(0 != option_argument);
			if(0 == strncmp(option_argument, "big", strlen(option_argument)))
				option_values.format_is_big_endian = true;
			else if(0 == strncmp(option_argument, "little", strlen(option_argument)))
				option_values.format_is_big_endian = false;
			else
				return usage_error("ERROR: argument to --endian must be \"big\" or \"little\"\n");
		}
		else if(0 == strcmp(long_option, "channels")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.format_channels = atoi(option_argument);
		}
		else if(0 == strcmp(long_option, "bps")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.format_bps = atoi(option_argument);
		}
		else if(0 == strcmp(long_option, "sample-rate")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.format_sample_rate = atoi(option_argument);
		}
		else if(0 == strcmp(long_option, "sign")) {
			FLAC__ASSERT(0 != option_argument);
			if(0 == strncmp(option_argument, "signed", strlen(option_argument)))
				option_values.format_is_unsigned_samples = false;
			else if(0 == strncmp(option_argument, "unsigned", strlen(option_argument)))
				option_values.format_is_unsigned_samples = true;
			else
				return usage_error("ERROR: argument to --sign must be \"signed\" or \"unsigned\"\n");
		}
		else if(0 == strcmp(long_option, "residual-gnuplot")) {
			option_values.aopts.do_residual_gnuplot = true;
		}
		else if(0 == strcmp(long_option, "residual-text")) {
			option_values.aopts.do_residual_text = true;
		}
		/*
		 * negatives
		 */
		else if(0 == strcmp(long_option, "no-decode-through-errors")) {
			option_values.continue_through_decode_errors = false;
		}
		else if(0 == strcmp(long_option, "no-silent")) {
			flac__utils_verbosity_ = 2;
		}
		else if(0 == strcmp(long_option, "no-force")) {
			option_values.force_file_overwrite = false;
		}
		else if(0 == strcmp(long_option, "no-seektable")) {
			option_values.num_requested_seek_points = 0;
			option_values.requested_seek_points[0] = '\0';
		}
		else if(0 == strcmp(long_option, "no-delete-input-file")) {
			option_values.delete_input = false;
		}
		else if(0 == strcmp(long_option, "no-keep-foreign-metadata")) {
			option_values.keep_foreign_metadata = false;
		}
		else if(0 == strcmp(long_option, "no-replay-gain")) {
			option_values.replay_gain = false;
		}
		else if(0 == strcmp(long_option, "no-ignore-chunk-sizes")) {
			option_values.ignore_chunk_sizes = false;
		}
		else if(0 == strcmp(long_option, "no-sector-align")) {
			option_values.sector_align = false;
		}
		else if(0 == strcmp(long_option, "no-utf8-convert")) {
			option_values.utf8_convert = false;
		}
		else if(0 == strcmp(long_option, "no-lax")) {
			option_values.lax = false;
		}
#if FLAC__HAS_OGG
		else if(0 == strcmp(long_option, "no-ogg")) {
			option_values.use_ogg = false;
		}
#endif
		else if(0 == strcmp(long_option, "no-exhaustive-model-search")) {
			add_compression_setting_bool(CST_DO_EXHAUSTIVE_MODEL_SEARCH, false);
		}
		else if(0 == strcmp(long_option, "no-mid-side")) {
			add_compression_setting_bool(CST_DO_MID_SIDE, false);
			add_compression_setting_bool(CST_LOOSE_MID_SIDE, false);
		}
		else if(0 == strcmp(long_option, "no-adaptive-mid-side")) {
			add_compression_setting_bool(CST_DO_MID_SIDE, false);
			add_compression_setting_bool(CST_LOOSE_MID_SIDE, false);
		}
		else if(0 == strcmp(long_option, "no-qlp-coeff-prec-search")) {
			add_compression_setting_bool(CST_DO_QLP_COEFF_PREC_SEARCH, false);
		}
		else if(0 == strcmp(long_option, "no-padding")) {
			option_values.padding = 0;
		}
		else if(0 == strcmp(long_option, "no-verify")) {
			option_values.verify = false;
		}
		else if(0 == strcmp(long_option, "no-warnings-as-errors")) {
			option_values.treat_warnings_as_errors = false;
		}
		else if(0 == strcmp(long_option, "no-residual-gnuplot")) {
			option_values.aopts.do_residual_gnuplot = false;
		}
		else if(0 == strcmp(long_option, "no-residual-text")) {
			option_values.aopts.do_residual_text = false;
		}
		else if(0 == strcmp(long_option, "disable-constant-subframes")) {
			option_values.debug.disable_constant_subframes = true;
		}
		else if(0 == strcmp(long_option, "disable-fixed-subframes")) {
			option_values.debug.disable_fixed_subframes = true;
		}
		else if(0 == strcmp(long_option, "disable-verbatim-subframes")) {
			option_values.debug.disable_verbatim_subframes = true;
		}
		else if(0 == strcmp(long_option, "no-md5-sum")) {
			option_values.debug.do_md5 = false;
		}
	}
	else {
		switch(short_option) {
			case 'h':
				option_values.show_help = true;
				break;
			case 'H':
				option_values.show_explain = true;
				break;
			case 'v':
				option_values.show_version = true;
				break;
			case 'd':
				option_values.mode_decode = true;
				break;
			case 'a':
				option_values.mode_decode = true;
				option_values.analyze = true;
				break;
			case 't':
				option_values.mode_decode = true;
				option_values.test_only = true;
				break;
			case 'c':
				option_values.force_to_stdout = true;
				break;
			case 's':
				flac__utils_verbosity_ = 1;
				break;
			case 'f':
				option_values.force_file_overwrite = true;
				break;
			case 'o':
				FLAC__ASSERT(0 != option_argument);
				option_values.cmdline_forced_outfilename = option_argument;
				break;
			case 'F':
				option_values.continue_through_decode_errors = true;
				break;
			case 'T':
				FLAC__ASSERT(0 != option_argument);
				if(!flac__vorbiscomment_add(option_values.vorbis_comment, option_argument, /*value_from_file=*/false, /*raw=*/!option_values.utf8_convert, &violation))
					return usage_error("ERROR: (-T/--tag) %s\n", violation);
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				add_compression_setting_unsigned(CST_COMPRESSION_LEVEL, short_option-'0');
				break;
			case '9':
				return usage_error("ERROR: compression level '9' is reserved\n");
			case 'V':
				option_values.verify = true;
				break;
			case 'w':
				option_values.treat_warnings_as_errors = true;
				break;
			case 'S':
				FLAC__ASSERT(0 != option_argument);
				if(0 == strcmp(option_argument, "-")) {
					option_values.num_requested_seek_points = 0;
					option_values.requested_seek_points[0] = '\0';
				}
				else {
					if(option_values.num_requested_seek_points < 0)
						option_values.num_requested_seek_points = 0;
					option_values.num_requested_seek_points++;
					if(strlen(option_values.requested_seek_points)+strlen(option_argument)+2 >= sizeof(option_values.requested_seek_points)) {
						return usage_error("ERROR: too many seekpoints requested\n");
					}
					else {
						strcat(option_values.requested_seek_points, option_argument);
						strcat(option_values.requested_seek_points, ";");
					}
				}
				break;
			case 'P':
				FLAC__ASSERT(0 != option_argument);
				option_values.padding = atoi(option_argument);
				if(option_values.padding < 0)
					return usage_error("ERROR: argument to -%c must be >= 0; for no padding use -%c-\n", short_option, short_option);
				break;
			case 'b':
				FLAC__ASSERT(0 != option_argument);
				i = atoi(option_argument);
				if((i < (int)FLAC__MIN_BLOCK_SIZE || i > (int)FLAC__MAX_BLOCK_SIZE))
					return usage_error("ERROR: invalid blocksize (-%c) '%d', must be >= %u and <= %u\n", short_option, i, FLAC__MIN_BLOCK_SIZE, FLAC__MAX_BLOCK_SIZE);
				add_compression_setting_unsigned(CST_BLOCKSIZE, (unsigned)i);
				break;
			case 'e':
				add_compression_setting_bool(CST_DO_EXHAUSTIVE_MODEL_SEARCH, true);
				break;
			case 'E':
				add_compression_setting_bool(CST_DO_ESCAPE_CODING, true);
				break;
			case 'l':
				FLAC__ASSERT(0 != option_argument);
				i = atoi(option_argument);
				if((i < 0 || i > (int)FLAC__MAX_LPC_ORDER))
					return usage_error("ERROR: invalid LPC order (-%c) '%d', must be >= %u and <= %u\n", short_option, i, 0, FLAC__MAX_LPC_ORDER);
				add_compression_setting_unsigned(CST_MAX_LPC_ORDER, (unsigned)i);
				break;
			case 'A':
				FLAC__ASSERT(0 != option_argument);
				add_compression_setting_string(CST_APODIZATION, option_argument);
				break;
			case 'm':
				add_compression_setting_bool(CST_DO_MID_SIDE, true);
				add_compression_setting_bool(CST_LOOSE_MID_SIDE, false);
				break;
			case 'M':
				add_compression_setting_bool(CST_DO_MID_SIDE, true);
				add_compression_setting_bool(CST_LOOSE_MID_SIDE, true);
				break;
			case 'p':
				add_compression_setting_bool(CST_DO_QLP_COEFF_PREC_SEARCH, true);
				break;
			case 'q':
				FLAC__ASSERT(0 != option_argument);
				i = atoi(option_argument);
				if(i < 0 || (i > 0 && (i < (int)FLAC__MIN_QLP_COEFF_PRECISION || i > (int)FLAC__MAX_QLP_COEFF_PRECISION)))
					return usage_error("ERROR: invalid value '%d' for qlp coeff precision (-%c), must be 0 or between %u and %u, inclusive\n", i, short_option, FLAC__MIN_QLP_COEFF_PRECISION, FLAC__MAX_QLP_COEFF_PRECISION);
				add_compression_setting_unsigned(CST_QLP_COEFF_PRECISION, (unsigned)i);
				break;
			case 'r':
				FLAC__ASSERT(0 != option_argument);
				p = strchr(option_argument, ',');
				if(0 == p) {
					add_compression_setting_unsigned(CST_MIN_RESIDUAL_PARTITION_ORDER, 0);
					i = atoi(option_argument);
					if(i < 0)
						return usage_error("ERROR: invalid value '%d' for residual partition order (-%c), must be between 0 and %u, inclusive\n", i, short_option, FLAC__MAX_RICE_PARTITION_ORDER);
					add_compression_setting_unsigned(CST_MAX_RESIDUAL_PARTITION_ORDER, (unsigned)i);
				}
				else {
					i = atoi(option_argument);
					if(i < 0)
						return usage_error("ERROR: invalid value '%d' for min residual partition order (-%c), must be between 0 and %u, inclusive\n", i, short_option, FLAC__MAX_RICE_PARTITION_ORDER);
					add_compression_setting_unsigned(CST_MIN_RESIDUAL_PARTITION_ORDER, (unsigned)i);
					i = atoi(++p);
					if(i < 0)
						return usage_error("ERROR: invalid value '%d' for max residual partition order (-%c), must be between 0 and %u, inclusive\n", i, short_option, FLAC__MAX_RICE_PARTITION_ORDER);
					add_compression_setting_unsigned(CST_MAX_RESIDUAL_PARTITION_ORDER, (unsigned)i);
				}
				break;
			case 'R':
				i = atoi(option_argument);
				if(i < 0)
					return usage_error("ERROR: invalid value '%d' for Rice parameter search distance (-%c), must be >= 0\n", i, short_option);
				add_compression_setting_unsigned(CST_RICE_PARAMETER_SEARCH_DIST, (unsigned)i);
				break;
			default:
				FLAC__ASSERT(0);
		}
	}

	return 0;
}

void free_options(void)
{
	unsigned i;
	if(0 != option_values.filenames) {
		for(i = 0; i < option_values.num_files; i++) {
			if(0 != option_values.filenames[i])
				free(option_values.filenames[i]);
		}
		free(option_values.filenames);
	}
	if(0 != option_values.vorbis_comment)
		FLAC__metadata_object_delete(option_values.vorbis_comment);
	for(i = 0; i < option_values.num_pictures; i++)
		FLAC__metadata_object_delete(option_values.pictures[i]);
}

void add_compression_setting_bool(compression_setting_type_t type, FLAC__bool value)
{
	if(option_values.num_compression_settings >= sizeof(option_values.compression_settings)/sizeof(option_values.compression_settings[0]))
		die("too many compression settings");
	option_values.compression_settings[option_values.num_compression_settings].type = type;
	option_values.compression_settings[option_values.num_compression_settings].value.t_bool = value;
	option_values.num_compression_settings++;
}

void add_compression_setting_string(compression_setting_type_t type, const char *value)
{
	if(option_values.num_compression_settings >= sizeof(option_values.compression_settings)/sizeof(option_values.compression_settings[0]))
		die("too many compression settings");
	option_values.compression_settings[option_values.num_compression_settings].type = type;
	option_values.compression_settings[option_values.num_compression_settings].value.t_string = value;
	option_values.num_compression_settings++;
}

void add_compression_setting_unsigned(compression_setting_type_t type, unsigned value)
{
	if(option_values.num_compression_settings >= sizeof(option_values.compression_settings)/sizeof(option_values.compression_settings[0]))
		die("too many compression settings");
	option_values.compression_settings[option_values.num_compression_settings].type = type;
	option_values.compression_settings[option_values.num_compression_settings].value.t_unsigned = value;
	option_values.num_compression_settings++;
}

int usage_error(const char *message, ...)
{
	if(flac__utils_verbosity_ >= 1) {
		va_list args;

		FLAC__ASSERT(0 != message);

		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

		printf("Type \"flac\" for a usage summary or \"flac --help\" for all options\n");
	}

	return 1;
}

void show_version(void)
{
	printf("flac %s\n", FLAC__VERSION_STRING);
}

static void usage_header(void)
{
	printf("===============================================================================\n");
	printf("flac - Command-line FLAC encoder/decoder version %s\n", FLAC__VERSION_STRING);
	printf("Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson\n");
	printf("\n");
	printf("This program is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU General Public License\n");
	printf("as published by the Free Software Foundation; either version 2\n");
	printf("of the License, or (at your option) any later version.\n");
	printf("\n");
	printf("This program is distributed in the hope that it will be useful,\n");
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	printf("GNU General Public License for more details.\n");
	printf("\n");
	printf("You should have received a copy of the GNU General Public License\n");
	printf("along with this program; if not, write to the Free Software\n");
	printf("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
	printf("===============================================================================\n");
}

static void usage_summary(void)
{
	printf("Usage:\n");
	printf("\n");
	printf(" Encoding: flac [<general-options>] [<encoding/format-options>] [INPUTFILE [...]]\n");
	printf(" Decoding: flac -d [<general-options>] [<format-options>] [FLACFILE [...]]\n");
	printf("  Testing: flac -t [<general-options>] [FLACFILE [...]]\n");
	printf("Analyzing: flac -a [<general-options>] [<analysis-options>] [FLACFILE [...]]\n");
	printf("\n");
}

void short_usage(void)
{
	usage_header();
	printf("\n");
	printf("This is the short help; for all options use 'flac --help'; for even more\n");
	printf("instructions use 'flac --explain'\n");
	printf("\n");
	printf("To encode:\n");
	printf("  flac [-#] [INPUTFILE [...]]\n");
	printf("\n");
	printf("  -# is -0 (fastest compression) to -8 (highest compression); -5 is the default\n");
	printf("\n");
	printf("To decode:\n");
	printf("  flac -d [INPUTFILE [...]]\n");
	printf("\n");
	printf("To test:\n");
	printf("  flac -t [INPUTFILE [...]]\n");
}

void show_help(void)
{
	usage_header();
	usage_summary();
	printf("general options:\n");
	printf("  -v, --version                Show the flac version number\n");
	printf("  -h, --help                   Show this screen\n");
	printf("  -H, --explain                Show detailed explanation of usage and options\n");
	printf("  -d, --decode                 Decode (the default behavior is to encode)\n");
	printf("  -t, --test                   Same as -d except no decoded file is written\n");
	printf("  -a, --analyze                Same as -d except an analysis file is written\n");
	printf("  -c, --stdout                 Write output to stdout\n");
	printf("  -s, --silent                 Do not write runtime encode/decode statistics\n");
	printf("      --totally-silent         Do not print anything, including errors\n");
	printf("      --no-utf8-convert        Do not convert tags from local charset to UTF-8\n");
	printf("  -w, --warnings-as-errors     Treat all warnings as errors\n");
	printf("  -f, --force                  Force overwriting of output files\n");
	printf("  -o, --output-name=FILENAME   Force the output file name\n");
	printf("      --output-prefix=STRING   Prepend STRING to output names\n");
	printf("      --delete-input-file      Deletes after a successful encode/decode\n");
	printf("      --keep-foreign-metadata  Save/restore WAVE or AIFF non-audio chunks\n");
	printf("      --skip={#|mm:ss.ss}      Skip the given initial samples for each input\n");
	printf("      --until={#|[+|-]mm:ss.ss}  Stop at the given sample for each input file\n");
#if FLAC__HAS_OGG
	printf("      --ogg                    Use Ogg as transport layer\n");
	printf("      --serial-number          Serial number to use for the FLAC stream\n");
#endif
	printf("analysis options:\n");
	printf("      --residual-text          Include residual signal in text output\n");
	printf("      --residual-gnuplot       Generate gnuplot files of residual distribution\n");
	printf("decoding options:\n");
	printf("  -F, --decode-through-errors  Continue decoding through stream errors\n");
	printf("      --cue=[#.#][-[#.#]]      Set the beginning and ending cuepoints to decode\n");
	printf("encoding options:\n");
	printf("  -V, --verify                 Verify a correct encoding\n");
	printf("      --lax                    Allow encoder to generate non-Subset files\n");
#if 0 /*@@@ currently undocumented */
	printf("      --ignore-chunk-sizes     Ignore data chunk sizes in WAVE/AIFF files\n");
#endif
	printf("      --sector-align           Align multiple files on sector boundaries\n");
	printf("      --replay-gain            Calculate ReplayGain & store in FLAC tags\n");
	printf("      --cuesheet=FILENAME      Import cuesheet and store in CUESHEET block\n");
	printf("      --picture=SPECIFICATION  Import picture and store in PICTURE block\n");
	printf("  -T, --tag=FIELD=VALUE        Add a FLAC tag; may appear multiple times\n");
	printf("      --tag-from-file=FIELD=FILENAME   Like --tag but gets value from file\n");
	printf("  -S, --seekpoint={#|X|#x|#s}  Add seek point(s)\n");
	printf("  -P, --padding=#              Write a PADDING block of length #\n");
	printf("  -0, --compression-level-0, --fast  Synonymous with -l 0 -b 1152 -r 3\n");
	printf("  -1, --compression-level-1          Synonymous with -l 0 -b 1152 -M -r 3\n");
	printf("  -2, --compression-level-2          Synonymous with -l 0 -b 1152 -m -r 3\n");
	printf("  -3, --compression-level-3          Synonymous with -l 6 -b 4096 -r 4\n");
	printf("  -4, --compression-level-4          Synonymous with -l 8 -b 4096 -M -r 4\n");
	printf("  -5, --compression-level-5          Synonymous with -l 8 -b 4096 -m -r 5\n");
	printf("  -6, --compression-level-6          Synonymous with -l 8 -b 4096 -m -r 6\n");
	printf("  -7, --compression-level-7          Synonymous with -l 8 -b 4096 -m -e -r 6\n");
	printf("  -8, --compression-level-8, --best  Synonymous with -l 12 -b 4096 -m -e -r 6\n");
	printf("  -b, --blocksize=#                  Specify blocksize in samples\n");
	printf("  -m, --mid-side                     Try mid-side coding for each frame\n");
	printf("  -M, --adaptive-mid-side            Adaptive mid-side coding for all frames\n");
	printf("  -e, --exhaustive-model-search      Do exhaustive model search (expensive!)\n");
	printf("  -A, --apodization=\"function\"       Window audio data with given the function\n");
	printf("  -l, --max-lpc-order=#              Max LPC order; 0 => only fixed predictors\n");
	printf("  -p, --qlp-coeff-precision-search   Exhaustively search LP coeff quantization\n");
	printf("  -q, --qlp-coeff-precision=#        Specify precision in bits\n");
	printf("  -r, --rice-partition-order=[#,]#   Set [min,]max residual partition order\n");
	printf("format options:\n");
	printf("      --endian={big|little}    Set byte order for samples\n");
	printf("      --channels=#             Number of channels\n");
	printf("      --bps=#                  Number of bits per sample\n");
	printf("      --sample-rate=#          Sample rate in Hz\n");
	printf("      --sign={signed|unsigned} Sign of samples\n");
	printf("      --input-size=#           Size of the raw input in bytes\n");
	printf("      --force-aiff-format      Force decoding to AIFF format\n");
	printf("      --force-raw-format       Treat input or output as raw samples\n");
	printf("negative options:\n");
	printf("      --no-adaptive-mid-side\n");
	printf("      --no-decode-through-errors\n");
	printf("      --no-delete-input-file\n");
	printf("      --no-keep-foreign-metadata\n");
	printf("      --no-exhaustive-model-search\n");
	printf("      --no-lax\n");
	printf("      --no-mid-side\n");
#if FLAC__HAS_OGG
	printf("      --no-ogg\n");
#endif
	printf("      --no-padding\n");
	printf("      --no-qlp-coeff-prec-search\n");
	printf("      --no-replay-gain\n");
	printf("      --no-residual-gnuplot\n");
	printf("      --no-residual-text\n");
#if 0 /*@@@ currently undocumented */
	printf("      --no-ignore-chunk-sizes\n");
#endif
	printf("      --no-sector-align\n");
	printf("      --no-seektable\n");
	printf("      --no-silent\n");
	printf("      --no-force\n");
	printf("      --no-verify\n");
	printf("      --no-warnings-as-errors\n");
}

void show_explain(void)
{
	usage_header();
	usage_summary();
	printf("For encoding:\n");
	printf("  The input file(s) may be a PCM WAVE file, AIFF (or uncompressed AIFF-C)\n");
	printf("  file, or raw samples.\n");
	printf("  The output file(s) will be in native FLAC or Ogg FLAC format\n");
	printf("For decoding, the reverse is true.\n");
	printf("\n");
	printf("A single INPUTFILE may be - for stdin.  No INPUTFILE implies stdin.  Use of\n");
	printf("stdin implies -c (write to stdout).  Normally you should use:\n");
	printf("   flac [options] -o outfilename  or  flac -d [options] -o outfilename\n");
	printf("instead of:\n");
	printf("   flac [options] > outfilename   or  flac -d [options] > outfilename\n");
	printf("since the former allows flac to seek backwards to write the STREAMINFO or\n");
	printf("WAVE/AIFF header contents when necessary.\n");
	printf("\n");
	printf("flac checks for the presence of a AIFF/WAVE header to decide whether or not to\n");
	printf("treat an input file as AIFF/WAVE format or raw samples.  If any input file is\n");
	printf("raw you must specify the format options {-fb|fl} -fc -fp and -fs, which will\n");
	printf("apply to all raw files.  You can force AIFF/WAVE files to be treated as raw\n");
	printf("files using -fr.\n");
	printf("\n");
	printf("general options:\n");
	printf("  -v, --version                Show the flac version number\n");
	printf("  -h, --help                   Show basic usage a list of all options\n");
	printf("  -H, --explain                Show this screen\n");
	printf("  -d, --decode                 Decode (the default behavior is to encode)\n");
	printf("  -t, --test                   Same as -d except no decoded file is written\n");
	printf("  -a, --analyze                Same as -d except an analysis file is written\n");
	printf("  -c, --stdout                 Write output to stdout\n");
	printf("  -s, --silent                 Do not write runtime encode/decode statistics\n");
	printf("      --totally-silent         Do not print anything of any kind, including\n");
	printf("                               warnings or errors.  The exit code will be the\n");
	printf("                               only way to determine successful completion.\n");
	printf("      --no-utf8-convert        Do not convert tags from local charset to UTF-8.\n");
	printf("                               This is useful for scripts, and setting tags in\n");
	printf("                               situations where the locale is wrong.  This\n");
	printf("                               option must appear before any tag options!\n");
	printf("  -w, --warnings-as-errors     Treat all warnings as errors\n");
	printf("  -f, --force                  Force overwriting of output files\n");
	printf("  -o, --output-name=FILENAME   Force the output file name; usually flac just\n");
	printf("                               changes the extension.  May only be used when\n");
	printf("                               encoding a single file.  May not be used in\n");
	printf("                               conjunction with --output-prefix.\n");
	printf("      --output-prefix=STRING   Prefix each output file name with the given\n");
	printf("                               STRING.  This can be useful for encoding or\n");
	printf("                               decoding files to a different directory.  Make\n");
	printf("                               sure if your STRING is a path name that it ends\n");
	printf("                               with a '/' slash.\n");
	printf("      --delete-input-file      Automatically delete the input file after a\n");
	printf("                               successful encode or decode.  If there was an\n");
	printf("                               error (including a verify error) the input file\n");
	printf("                               is left intact.\n");
	printf("      --keep-foreign-metadata  If encoding, save WAVE or AIFF non-audio chunks\n");
	printf("                               in FLAC metadata.  If decoding, restore any saved\n");
	printf("                               non-audio chunks from FLAC metadata when writing\n");
	printf("                               the decoded file.  Foreign metadata cannot be\n");
	printf("                               transcoded, e.g. WAVE chunks saved in a FLAC file\n");
	printf("                               cannot be restored when decoding to AIFF.  Input\n");
	printf("                               and output must be regular files, not stdin/out.\n");
	printf("      --skip={#|mm:ss.ss}      Skip the first # samples of each input file; can\n");
	printf("                               be used both for encoding and decoding.  The\n");
	printf("                               alternative form mm:ss.ss can be used to specify\n");
	printf("                               minutes, seconds, and fractions of a second.\n");
	printf("      --until={#|[+|-]mm:ss.ss}  Stop at the given sample number for each input\n");
	printf("                               file.  The given sample number is not included\n");
	printf("                               in the decoded output.  The alternative form\n");
	printf("                               mm:ss.ss can be used to specify minutes,\n");
	printf("                               seconds, and fractions of a second.  If a `+'\n");
	printf("                               sign is at the beginning, the --until point is\n");
	printf("                               relative to the --skip point.  If a `-' sign is\n");
	printf("                               at the beginning, the --until point is relative\n");
	printf("                               to end of the audio.\n");
#if FLAC__HAS_OGG
	printf("      --ogg                    When encoding, generate Ogg FLAC output instead\n");
	printf("                               of native FLAC.  Ogg FLAC streams are FLAC\n");
	printf("                               streams wrapped in an Ogg transport layer.  The\n");
	printf("                               resulting file should have an '.oga' extension\n");
	printf("                               and will still be decodable by flac.  When\n");
	printf("                               decoding, force the input to be treated as\n");
	printf("                               Ogg FLAC.  This is useful when piping input\n");
	printf("                               from stdin or when the filename does not end in\n");
	printf("                               '.oga' or '.ogg'.\n");
	printf("      --serial-number          Serial number to use for the FLAC stream.  When\n");
	printf("                               encoding and no serial number is given, flac\n");
	printf("                               uses a random one.  If encoding to multiple files\n");
	printf("                               the serial number is incremented for each file.\n");
	printf("                               When decoding and no number is given, flac uses\n");
	printf("                               the serial number of the first page.\n");
#endif
	printf("analysis options:\n");
	printf("      --residual-text          Include residual signal in text output.  This\n");
	printf("                               will make the file very big, much larger than\n");
	printf("                               even the decoded file.\n");
	printf("      --residual-gnuplot       Generate gnuplot files of residual distribution\n");
	printf("                               of each subframe\n");
	printf("decoding options:\n");
	printf("  -F, --decode-through-errors  By default flac stops decoding with an error\n");
	printf("                               and removes the partially decoded file if it\n");
	printf("                               encounters a bitstream error.  With -F, errors\n");
	printf("                               are still printed but flac will continue\n");
	printf("                               decoding to completion.  Note that errors may\n");
	printf("                               cause the decoded audio to be missing some\n");
	printf("                               samples or have silent sections.\n");
	printf("      --cue=[#.#][-[#.#]]      Set the beginning and ending cuepoints to\n");
	printf("                               decode.  The optional first #.# is the track and\n");
	printf("                               index point at which decoding will start; the\n");
	printf("                               default is the beginning of the stream.  The\n");
	printf("                               optional second #.# is the track and index point\n");
	printf("                               at which decoding will end; the default is the\n");
	printf("                               end of the stream.  If the cuepoint does not\n");
	printf("                               exist, the closest one before it (for the start\n");
	printf("                               point) or after it (for the end point) will be\n");
	printf("                               used.  The cuepoints are merely translated into\n");
	printf("                               sample numbers then used as --skip and --until.\n");
	printf("                               A CD track can always be cued by, for example,\n");
	printf("                               --cue=9.1-10.1 for track 9, even if the CD has\n");
	printf("                               no 10th track.\n");
	printf("encoding options:\n");
	printf("  -V, --verify                 Verify a correct encoding by decoding the\n");
	printf("                               output in parallel and comparing to the\n");
	printf("                               original\n");
	printf("      --lax                    Allow encoder to generate non-Subset files\n");
#if 0 /*@@@ currently undocumented */
	printf("      --ignore-chunk-sizes     Ignore data chunk sizes in WAVE/AIFF files;\n");
	printf("                               useful when piping data from programs which\n");
	printf("                               generate bogus data chunk sizes.\n");
#endif
	printf("      --sector-align           Align encoding of multiple CD format WAVE files\n");
	printf("                               on sector boundaries.\n");
	printf("      --replay-gain            Calculate ReplayGain values and store them as\n");
	printf("                               FLAC tags.  Title gains/peaks will be computed\n");
	printf("                               for each file, and an album gain/peak will be\n");
	printf("                               computed for all files.  All input files must\n");
	printf("                               have the same resolution, sample rate, and\n");
	printf("                               number of channels.  The sample rate must be\n");
	printf("                               one of 8, 11.025, 12, 16, 22.05, 24, 32, 44.1,\n");
	printf("                               or 48 kHz.  NOTE: this option may also leave a\n");
	printf("                               few extra bytes in the PADDING block.\n");
	printf("      --cuesheet=FILENAME      Import the given cuesheet file and store it in\n");
	printf("                               a CUESHEET metadata block.  This option may only\n");
	printf("                               be used when encoding a single file.  A\n");
	printf("                               seekpoint will be added for each index point in\n");
	printf("                               the cuesheet to the SEEKTABLE unless\n");
	printf("                               --no-cued-seekpoints is specified.\n");
	printf("      --picture=SPECIFICATION  Import a picture and store it in a PICTURE block.\n");
	printf("                               More than one --picture command can be specified.\n");
	printf("                               The SPECIFICATION can either be a simple filename\n");
	printf("                               for the picture file, or a complete specification\n");
	printf("                               whose parts are separated by | characters.  Some\n");
	printf("                               parts may be left empty to invoke default values.\n");
	printf("                               Using a filename is shorthand for \"||||FILE\".\n");
	printf("                               The SPECIFICATION format is:\n");
	printf("         [TYPE]|[MIME-TYPE]|[DESCRIPTION]|[WIDTHxHEIGHTxDEPTH[/COLORS]]|FILE\n");
	printf("           TYPE is optional; it is a number from one of:\n");
	printf("              0: Other\n");
	printf("              1: 32x32 pixels 'file icon' (PNG only)\n");
	printf("              2: Other file icon\n");
	printf("              3: Cover (front)\n");
	printf("              4: Cover (back)\n");
	printf("              5: Leaflet page\n");
	printf("              6: Media (e.g. label side of CD)\n");
	printf("              7: Lead artist/lead performer/soloist\n");
	printf("              8: Artist/performer\n");
	printf("              9: Conductor\n");
	printf("             10: Band/Orchestra\n");
	printf("             11: Composer\n");
	printf("             12: Lyricist/text writer\n");
	printf("             13: Recording Location\n");
	printf("             14: During recording\n");
	printf("             15: During performance\n");
	printf("             16: Movie/video screen capture\n");
	printf("             17: A bright coloured fish\n");
	printf("             18: Illustration\n");
	printf("             19: Band/artist logotype\n");
	printf("             20: Publisher/Studio logotype\n");
	printf("             The default is 3 (front cover).  There may only be one picture each\n");
	printf("             of type 1 and 2 in a file.\n");
	printf("           MIME-TYPE is optional; if left blank, it will be detected from the\n");
	printf("             file.  For best compatibility with players, use pictures with MIME\n");
	printf("             type image/jpeg or image/png.  The MIME type can also be --> to\n");
	printf("             mean that FILE is actually a URL to an image, though this use is\n");
	printf("             discouraged.\n");
	printf("           DESCRIPTION is optional; the default is an empty string\n");
	printf("           The next part specfies the resolution and color information.  If\n");
	printf("             the MIME-TYPE is image/jpeg, image/png, or image/gif, you can\n");
	printf("             usually leave this empty and they can be detected from the file.\n");
	printf("             Otherwise, you must specify the width in pixels, height in pixels,\n");
	printf("             and color depth in bits-per-pixel.  If the image has indexed colors\n");
	printf("             you should also specify the number of colors used.\n");
	printf("           FILE is the path to the picture file to be imported, or the URL if\n");
	printf("             MIME type is -->\n");
	printf("  -T, --tag=FIELD=VALUE        Add a FLAC tag.  Make sure to quote the\n");
	printf("                               comment if necessary.  This option may appear\n");
	printf("                               more than once to add several comments.  NOTE:\n");
	printf("                               all tags will be added to all encoded files.\n");
	printf("      --tag-from-file=FIELD=FILENAME   Like --tag, except FILENAME is a file\n");
	printf("                               whose contents will be read verbatim to set the\n");
	printf("                               tag value.  The contents will be converted to\n");
	printf("                               UTF-8 from the local charset.  This can be used\n");
	printf("                               to store a cuesheet in a tag (e.g.\n");
	printf("                               --tag-from-file=\"CUESHEET=image.cue\").  Do not\n");
	printf("                               try to store binary data in tag fields!  Use\n");
	printf("                               APPLICATION blocks for that.\n");
	printf("  -S, --seekpoint={#|X|#x|#s}  Include a point or points in a SEEKTABLE\n");
	printf("       #  : a specific sample number for a seek point\n");
	printf("       X  : a placeholder point (always goes at the end of the SEEKTABLE)\n");
	printf("       #x : # evenly spaced seekpoints, the first being at sample 0\n");
	printf("       #s : a seekpoint every # seconds; # does not have to be a whole number\n");
	printf("     You may use many -S options; the resulting SEEKTABLE will be the unique-\n");
	printf("           ified union of all such values.\n");
	printf("     With no -S options, flac defaults to '-S 10s'.  Use -S- for no SEEKTABLE.\n");
	printf("     Note: -S #x and -S #s will not work if the encoder can't determine the\n");
	printf("           input size before starting.\n");
	printf("     Note: if you use -S # and # is >= samples in the input, there will be\n");
	printf("           either no seek point entered (if the input size is determinable\n");
	printf("           before encoding starts) or a placeholder point (if input size is not\n");
	printf("           determinable)\n");
	printf("  -P, --padding=#              Tell the encoder to write a PADDING metadata\n");
	printf("                               block of the given length (in bytes) after the\n");
	printf("                               STREAMINFO block.  This is useful if you plan\n");
	printf("                               to tag the file later with an APPLICATION\n");
	printf("                               block; instead of having to rewrite the entire\n");
	printf("                               file later just to insert your block, you can\n");
	printf("                               write directly over the PADDING block.  Note\n");
	printf("                               that the total length of the PADDING block will\n");
	printf("                               be 4 bytes longer than the length given because\n");
	printf("                               of the 4 metadata block header bytes.  You can\n");
	printf("                               force no PADDING block at all to be written with\n");
	printf("                               --no-padding.  The encoder writes a PADDING\n");
	printf("                               block of 8192 bytes by default, or 65536 bytes\n");
	printf("                               if the input audio is more than 20 minutes long.\n");
	printf("  -b, --blocksize=#            Specify the blocksize in samples; the default is\n");
	printf("                               1152 for -l 0, else 4096; must be one of 192,\n");
	printf("                               576, 1152, 2304, 4608, 256, 512, 1024, 2048,\n");
	printf("                               4096 (and 8192 or 16384 if the sample rate is\n");
	printf("                               >48kHz) for Subset streams.\n");
	printf("  -0, --compression-level-0, --fast  Synonymous with -l 0 -b 1152 -r 3\n");
	printf("  -1, --compression-level-1          Synonymous with -l 0 -b 1152 -M -r 3\n");
	printf("  -2, --compression-level-2          Synonymous with -l 0 -b 1152 -m -r 3\n");
	printf("  -3, --compression-level-3          Synonymous with -l 6 -b 4096 -r 4\n");
	printf("  -4, --compression-level-4          Synonymous with -l 8 -b 4096 -M -r 4\n");
	printf("  -5, --compression-level-5          Synonymous with -l 8 -b 4096 -m -r 5\n");
	printf("                                     -5 is the default setting\n");
	printf("  -6, --compression-level-6          Synonymous with -l 8 -b 4096 -m -r 6\n");
	printf("  -7, --compression-level-7          Synonymous with -l 8 -b 4096 -m -e -r 6\n");
	printf("  -8, --compression-level-8, --best  Synonymous with -l 12 -b 4096 -m -e -r 6\n");
	printf("  -m, --mid-side                     Try mid-side coding for each frame\n");
	printf("                                     (stereo only)\n");
	printf("  -M, --adaptive-mid-side            Adaptive mid-side coding for all frames\n");
	printf("                                     (stereo only)\n");
	printf("  -e, --exhaustive-model-search      Do exhaustive model search (expensive!)\n");
	printf("  -A, --apodization=\"function\"       Window audio data with given the function.\n");
	printf("                                     The functions are: bartlett, bartlett_hann,\n");
	printf("                                     blackman, blackman_harris_4term_92db,\n");
	printf("                                     connes, flattop, gauss(STDDEV), hamming,\n");
	printf("                                     hann, kaiser_bessel, nuttall, rectangle,\n");
	printf("                                     triangle, tukey(P), welch.  More than one\n");
	printf("                                     may be specified but encoding time is a\n");
	printf("                                     multiple of the number of functions since\n");
	printf("                                     they are each tried in turn.  The encoder\n");
	printf("                                     chooses suitable defaults in the absence\n");
	printf("                                     of any -A options.\n");
	printf("  -l, --max-lpc-order=#              Max LPC order; 0 => only fixed predictors.\n");
	printf("                                     Must be <= 12 for Subset streams if sample\n");
	printf("                                     rate is <=48kHz.\n");
	printf("  -p, --qlp-coeff-precision-search   Do exhaustive search of LP coefficient\n");
	printf("                                     quantization (expensive!); overrides -q;\n");
	printf("                                     does nothing if using -l 0\n");
	printf("  -q, --qlp-coeff-precision=#        Specify precision in bits of quantized\n");
	printf("                                     linear-predictor coefficients; 0 => let\n");
	printf("                                     encoder decide (the minimun is %u, the\n", FLAC__MIN_QLP_COEFF_PRECISION);
	printf("                                     default is -q 0)\n");
	printf("  -r, --rice-partition-order=[#,]#   Set [min,]max residual partition order\n");
	printf("                                     (# is 0..16; min defaults to 0; the\n");
	printf("                                     default is -r 0; above 4 doesn't usually\n");
	printf("                                     help much)\n");
	printf("format options:\n");
	printf("      --endian={big|little}    Set byte order for samples\n");
	printf("      --channels=#             Number of channels\n");
	printf("      --bps=#                  Number of bits per sample\n");
	printf("      --sample-rate=#          Sample rate in Hz\n");
	printf("      --sign={signed|unsigned} Sign of samples (the default is signed)\n");
	printf("      --input-size=#           Size of the raw input in bytes.  If you are\n");
	printf("                               encoding raw samples from stdin, you must set\n");
	printf("                               this option in order to be able to use --skip,\n");
	printf("                               --until, --cue-sheet, or other options that need\n");
	printf("                               to know the size of the input beforehand.  If\n");
	printf("                               the size given is greater than what is found in\n");
	printf("                               the input stream, the encoder will complain\n");
	printf("                               about an unexpected end-of-file.  If the size\n");
	printf("                               given is less, samples will be truncated.\n");
	printf("      --force-aiff-format      Force the decoder to output AIFF format.  This\n");
	printf("                               option is not needed if the output filename (as\n");
	printf("                               set by -o) ends with .aif or .aiff; this option\n");
	printf("                               has no effect when encoding since input AIFF is\n");
	printf("                               auto-detected.\n");
	printf("      --force-raw-format       Force input (when encoding) or output (when\n");
	printf("                               decoding) to be treated as raw samples\n");
	printf("negative options:\n");
	printf("      --no-adaptive-mid-side\n");
	printf("      --no-decode-through-errors\n");
	printf("      --no-delete-input-file\n");
	printf("      --no-keep-foreign-metadata\n");
	printf("      --no-exhaustive-model-search\n");
	printf("      --no-lax\n");
	printf("      --no-mid-side\n");
#if FLAC__HAS_OGG
	printf("      --no-ogg\n");
#endif
	printf("      --no-padding\n");
	printf("      --no-qlp-coeff-prec-search\n");
	printf("      --no-residual-gnuplot\n");
	printf("      --no-residual-text\n");
#if 0 /*@@@ currently undocumented */
	printf("      --no-ignore-chunk-sizes\n");
#endif
	printf("      --no-sector-align\n");
	printf("      --no-seektable\n");
	printf("      --no-silent\n");
	printf("      --no-force\n");
	printf("      --no-verify\n");
	printf("      --no-warnings-as-errors\n");
}

void format_mistake(const char *infilename, FileFormat wrong, FileFormat right)
{
	/* WATCHOUT: indexed by FileFormat */
	static const char * const ff[] = { "raw", "WAVE", "AIFF", "FLAC", "Ogg FLAC" };
	flac__utils_printf(stderr, 1, "WARNING: %s is not a %s file; treating as a %s file\n", infilename, ff[wrong], ff[right]);
}

int encode_file(const char *infilename, FLAC__bool is_first_file, FLAC__bool is_last_file)
{
	FILE *encode_infile;
	FLAC__byte lookahead[12];
	unsigned lookahead_length = 0;
	FileFormat input_format = RAW;
	FLAC__bool is_aifc = false;
	int retval;
	off_t infilesize;
	encode_options_t common_options;
	const char *outfilename = get_encoded_outfilename(infilename); /* the final name of the encoded file */
	/* internal_outfilename is the file we will actually write to; it will be a temporary name if infilename==outfilename */
	char *internal_outfilename = 0; /* NULL implies 'use outfilename' */

	if(0 == outfilename) {
		flac__utils_printf(stderr, 1, "ERROR: filename too long: %s", infilename);
		return 1;
	}

	if(0 == strcmp(infilename, "-")) {
		infilesize = (off_t)(-1);
		encode_infile = grabbag__file_get_binary_stdin();
	}
	else {
		infilesize = grabbag__file_get_filesize(infilename);
		if(0 == (encode_infile = fopen(infilename, "rb"))) {
			flac__utils_printf(stderr, 1, "ERROR: can't open input file %s: %s\n", infilename, strerror(errno));
			return 1;
		}
	}

	if(!option_values.force_raw_format) {
		/* first set format based on name */
		if(strlen(infilename) >= 4 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-4), ".wav"))
			input_format = WAV;
		else if(strlen(infilename) >= 4 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-4), ".aif"))
			input_format = AIF;
		else if(strlen(infilename) >= 5 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-5), ".aiff"))
			input_format = AIF;
		else if(strlen(infilename) >= 5 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-5), ".flac"))
			input_format = FLAC;
		else if(strlen(infilename) >= 4 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-4), ".oga"))
			input_format = OGGFLAC;
		else if(strlen(infilename) >= 4 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-4), ".ogg"))
			input_format = OGGFLAC;

		/* attempt to guess the file type based on the first 12 bytes */
		if((lookahead_length = fread(lookahead, 1, 12, encode_infile)) < 12) {
			if(input_format != RAW) {
				format_mistake(infilename, input_format, RAW);
				if(option_values.treat_warnings_as_errors) {
					conditional_fclose(encode_infile);
					return 1;
				}
			}
			input_format = RAW;
		}
		else {
			if(!strncmp((const char *)lookahead, "ID3", 3)) {
				flac__utils_printf(stderr, 1, "ERROR: input file %s has an ID3v2 tag\n", infilename);
				return 1;
			}
			else if(!strncmp((const char *)lookahead, "RIFF", 4) && !strncmp((const char *)lookahead+8, "WAVE", 4))
				input_format = WAV;
			else if(!strncmp((const char *)lookahead, "FORM", 4) && !strncmp((const char *)lookahead+8, "AIFF", 4))
				input_format = AIF;
			else if(!strncmp((const char *)lookahead, "FORM", 4) && !strncmp((const char *)lookahead+8, "AIFC", 4)) {
				input_format = AIF;
				is_aifc = true;
			}
			else if(!memcmp(lookahead, FLAC__STREAM_SYNC_STRING, sizeof(FLAC__STREAM_SYNC_STRING)))
				input_format = FLAC;
			/* this could be made more accurate by looking at the first packet */
			else if(!memcmp(lookahead, "OggS", 4))
				input_format = OGGFLAC;
			else {
				if(input_format != RAW) {
					format_mistake(infilename, input_format, RAW);
					if(option_values.treat_warnings_as_errors) {
						conditional_fclose(encode_infile);
						return 1;
					}
				}
				input_format = RAW;
			}
		}
	}

	if(option_values.keep_foreign_metadata) {
		if(encode_infile == stdin || option_values.force_to_stdout) {
			conditional_fclose(encode_infile);
			return usage_error("ERROR: --keep-foreign-metadata cannot be used when encoding from stdin or to stdout\n");
		}
		if(input_format != WAV && input_format != AIF) {
			conditional_fclose(encode_infile);
			return usage_error("ERROR: --keep-foreign-metadata can only be used with WAVE or AIFF input\n");
		}
	}

	/*
	 * Error if output file already exists (and -f not used).
	 * Use grabbag__file_get_filesize() as a cheap way to check.
	 */
	if(!option_values.test_only && !option_values.force_file_overwrite && strcmp(outfilename, "-") && grabbag__file_get_filesize(outfilename) != (off_t)(-1)) {
		if(input_format == FLAC) {
			/* need more detailed error message when re-flac'ing to avoid confusing the user */
			flac__utils_printf(stderr, 1,
				"ERROR: output file %s already exists.\n\n"
				"By default flac encodes files to FLAC format; if you meant to decode this file\n"
				"from FLAC to something else, use -d.  If you meant to re-encode this file from\n"
				"FLAC to FLAC again, use -f to force writing to the same file, or -o to specify\n"
				"a different output filename.\n",
				outfilename
			);
		}
		else if(input_format == OGGFLAC) {
			/* need more detailed error message when re-flac'ing to avoid confusing the user */
			flac__utils_printf(stderr, 1,
				"ERROR: output file %s already exists.\n\n"
				"By default 'flac -ogg' encodes files to Ogg FLAC format; if you meant to decode\n"
				"this file from Ogg FLAC to something else, use -d.  If you meant to re-encode\n"
				"this file from Ogg FLAC to Ogg FLAC again, use -f to force writing to the same\n"
				"file, or -o to specify a different output filename.\n",
				outfilename
			);
		}
		else
			flac__utils_printf(stderr, 1, "ERROR: output file %s already exists, use -f to override\n", outfilename);
		conditional_fclose(encode_infile);
		return 1;
	}

	if(option_values.format_input_size >= 0) {
	   	if (input_format != RAW || infilesize >= 0) {
			flac__utils_printf(stderr, 1, "ERROR: can only use --input-size when encoding raw samples from stdin\n");
			conditional_fclose(encode_infile);
			return 1;
		}
		else {
			infilesize = option_values.format_input_size;
		}
	}

	if(option_values.sector_align && (input_format == FLAC || input_format == OGGFLAC)) {
		flac__utils_printf(stderr, 1, "ERROR: can't use --sector-align when the input file is FLAC or Ogg FLAC\n");
		conditional_fclose(encode_infile);
		return 1;
	}
	if(option_values.sector_align && input_format == RAW && infilesize < 0) {
		flac__utils_printf(stderr, 1, "ERROR: can't use --sector-align when the input size is unknown\n");
		conditional_fclose(encode_infile);
		return 1;
	}

	if(input_format == RAW) {
		if(option_values.format_is_big_endian < 0 || option_values.format_is_unsigned_samples < 0 || option_values.format_channels < 0 || option_values.format_bps < 0 || option_values.format_sample_rate < 0) {
			conditional_fclose(encode_infile);
			return usage_error("ERROR: for encoding a raw file you must specify a value for --endian, --sign, --channels, --bps, and --sample-rate\n");
		}
	}

	if(/*@@@@@@why no stdin?*/encode_infile == stdin || option_values.force_to_stdout) {
		if(option_values.replay_gain) {
			conditional_fclose(encode_infile);
			return usage_error("ERROR: --replay-gain cannot be used when encoding to stdout\n");
		}
	}
	if(option_values.replay_gain && option_values.use_ogg) {
		conditional_fclose(encode_infile);
		return usage_error("ERROR: --replay-gain cannot be used when encoding to Ogg FLAC yet\n");
	}

	if(!flac__utils_parse_skip_until_specification(option_values.skip_specification, &common_options.skip_specification) || common_options.skip_specification.is_relative) {
		conditional_fclose(encode_infile);
		return usage_error("ERROR: invalid value for --skip\n");
	}

	if(!flac__utils_parse_skip_until_specification(option_values.until_specification, &common_options.until_specification)) { /*@@@@ more checks: no + without --skip, no - unless known total_samples_to_{en,de}code */
		conditional_fclose(encode_infile);
		return usage_error("ERROR: invalid value for --until\n");
	}
	/* if there is no "--until" we want to default to "--until=-0" */
	if(0 == option_values.until_specification)
		common_options.until_specification.is_relative = true;

	common_options.verify = option_values.verify;
	common_options.treat_warnings_as_errors = option_values.treat_warnings_as_errors;
#if FLAC__HAS_OGG
	common_options.use_ogg = option_values.use_ogg;
	/* set a random serial number if one has not yet been specified */
	if(!option_values.has_serial_number) {
		option_values.serial_number = rand();
		option_values.has_serial_number = true;
	}
	common_options.serial_number = option_values.serial_number++;
#endif
	common_options.lax = option_values.lax;
	common_options.padding = option_values.padding;
	common_options.num_compression_settings = option_values.num_compression_settings;
	FLAC__ASSERT(sizeof(common_options.compression_settings) >= sizeof(option_values.compression_settings));
	memcpy(common_options.compression_settings, option_values.compression_settings, sizeof(option_values.compression_settings));
	common_options.requested_seek_points = option_values.requested_seek_points;
	common_options.num_requested_seek_points = option_values.num_requested_seek_points;
	common_options.cuesheet_filename = option_values.cuesheet_filename;
	common_options.continue_through_decode_errors = option_values.continue_through_decode_errors;
	common_options.cued_seekpoints = option_values.cued_seekpoints;
	common_options.channel_map_none = option_values.channel_map_none;
	common_options.is_first_file = is_first_file;
	common_options.is_last_file = is_last_file;
	common_options.align_reservoir = align_reservoir;
	common_options.align_reservoir_samples = &align_reservoir_samples;
	common_options.replay_gain = option_values.replay_gain;
	common_options.ignore_chunk_sizes = option_values.ignore_chunk_sizes;
	common_options.sector_align = option_values.sector_align;
	common_options.vorbis_comment = option_values.vorbis_comment;
	FLAC__ASSERT(sizeof(common_options.pictures) >= sizeof(option_values.pictures));
	memcpy(common_options.pictures, option_values.pictures, sizeof(option_values.pictures));
	common_options.num_pictures = option_values.num_pictures;
	common_options.debug.disable_constant_subframes = option_values.debug.disable_constant_subframes;
	common_options.debug.disable_fixed_subframes = option_values.debug.disable_fixed_subframes;
	common_options.debug.disable_verbatim_subframes = option_values.debug.disable_verbatim_subframes;
	common_options.debug.do_md5 = option_values.debug.do_md5;

	/* if infilename and outfilename point to the same file, we need to write to a temporary file */
	if(encode_infile != stdin && grabbag__file_are_same(infilename, outfilename)) {
		static const char *tmp_suffix = ".tmp,fl-ac+en'c";
		/*@@@@ still a remote possibility that a file with this filename exists */
		if(0 == (internal_outfilename = (char *)safe_malloc_add_3op_(strlen(outfilename), /*+*/strlen(tmp_suffix), /*+*/1))) {
			flac__utils_printf(stderr, 1, "ERROR allocating memory for tempfile name\n");
			conditional_fclose(encode_infile);
			return 1;
		}
		strcpy(internal_outfilename, outfilename);
		strcat(internal_outfilename, tmp_suffix);
	}

	if(input_format == RAW) {
		raw_encode_options_t options;

		options.common = common_options;
		options.is_big_endian = option_values.format_is_big_endian;
		options.is_unsigned_samples = option_values.format_is_unsigned_samples;
		options.channels = option_values.format_channels;
		options.bps = option_values.format_bps;
		options.sample_rate = option_values.format_sample_rate;

		retval = flac__encode_raw(encode_infile, infilesize, infilename, internal_outfilename? internal_outfilename : outfilename, lookahead, lookahead_length, options);
	}
	else if(input_format == FLAC || input_format == OGGFLAC) {
		flac_encode_options_t options;

		options.common = common_options;

		retval = flac__encode_flac(encode_infile, infilesize, infilename, internal_outfilename? internal_outfilename : outfilename, lookahead, lookahead_length, options, input_format==OGGFLAC);
	}
	else {
		wav_encode_options_t options;

		options.common = common_options;
		options.foreign_metadata = 0;

		/* read foreign metadata if requested */
		if(option_values.keep_foreign_metadata) {
			if(0 == (options.foreign_metadata = flac__foreign_metadata_new(input_format==AIF? FOREIGN_BLOCK_TYPE__AIFF : FOREIGN_BLOCK_TYPE__RIFF))) {
				flac__utils_printf(stderr, 1, "ERROR: creating foreign metadata object\n");
				conditional_fclose(encode_infile);
				return 1;
			}
		}

		if(input_format == AIF)
			retval = flac__encode_aif(encode_infile, infilesize, infilename, internal_outfilename? internal_outfilename : outfilename, lookahead, lookahead_length, options, is_aifc);
		else
			retval = flac__encode_wav(encode_infile, infilesize, infilename, internal_outfilename? internal_outfilename : outfilename, lookahead, lookahead_length, options);

		if(options.foreign_metadata)
			flac__foreign_metadata_delete(options.foreign_metadata);
	}

	if(retval == 0) {
		if(strcmp(outfilename, "-")) {
			if(option_values.replay_gain) {
				float title_gain, title_peak;
				const char *error;
				grabbag__replaygain_get_title(&title_gain, &title_peak);
				if(
					0 != (error = grabbag__replaygain_store_to_file_reference(internal_outfilename? internal_outfilename : outfilename, /*preserve_modtime=*/true)) ||
					0 != (error = grabbag__replaygain_store_to_file_title(internal_outfilename? internal_outfilename : outfilename, title_gain, title_peak, /*preserve_modtime=*/true))
				) {
					flac__utils_printf(stderr, 1, "%s: ERROR writing ReplayGain reference/title tags (%s)\n", outfilename, error);
					retval = 1;
				}
			}
			if(strcmp(infilename, "-"))
				grabbag__file_copy_metadata(infilename, internal_outfilename? internal_outfilename : outfilename);
		}
	}

	/* rename temporary file if necessary */
	if(retval == 0 && internal_outfilename != 0) {
		if(rename(internal_outfilename, outfilename) < 0) {
#if defined _MSC_VER || defined __MINGW32__ || defined __EMX__
			/* on some flavors of windows, rename() will fail if the destination already exists, so we unlink and try again */
			if(unlink(outfilename) < 0) {
				flac__utils_printf(stderr, 1, "ERROR: moving new FLAC file %s back on top of original FLAC file %s, keeping both\n", internal_outfilename, outfilename);
				retval = 1;
			}
			else if(rename(internal_outfilename, outfilename) < 0) {
				flac__utils_printf(stderr, 1, "ERROR: moving new FLAC file %s back on top of original FLAC file %s, you must do it\n", internal_outfilename, outfilename);
				retval = 1;
			}
#else
			flac__utils_printf(stderr, 1, "ERROR: moving new FLAC file %s back on top of original FLAC file %s, keeping both\n", internal_outfilename, outfilename);
			retval = 1;
#endif
		}
	}

	/* handle --delete-input-file, but don't want to delete if piping from stdin, or if input filename and output filename are the same */
	if(retval == 0 && option_values.delete_input && strcmp(infilename, "-") && internal_outfilename == 0)
		unlink(infilename);

	if(internal_outfilename != 0)
		free(internal_outfilename);

	return retval;
}

int decode_file(const char *infilename)
{
	int retval;
	FLAC__bool treat_as_ogg = false;
	FileFormat output_format = WAV;
	decode_options_t common_options;
	const char *outfilename = get_decoded_outfilename(infilename);

	if(0 == outfilename) {
		flac__utils_printf(stderr, 1, "ERROR: filename too long: %s", infilename);
		return 1;
	}

	/*
	 * Error if output file already exists (and -f not used).
	 * Use grabbag__file_get_filesize() as a cheap way to check.
	 */
	if(!option_values.test_only && !option_values.force_file_overwrite && strcmp(outfilename, "-") && grabbag__file_get_filesize(outfilename) != (off_t)(-1)) {
		flac__utils_printf(stderr, 1, "ERROR: output file %s already exists, use -f to override\n", outfilename);
		return 1;
	}

	if(option_values.force_raw_format)
		output_format = RAW;
	else if(
		option_values.force_aiff_format ||
		(strlen(outfilename) >= 4 && 0 == FLAC__STRCASECMP(outfilename+(strlen(outfilename)-4), ".aif")) ||
		(strlen(outfilename) >= 5 && 0 == FLAC__STRCASECMP(outfilename+(strlen(outfilename)-5), ".aiff"))
	)
		output_format = AIF;
	else
		output_format = WAV;

	if(!option_values.test_only && !option_values.analyze) {
		if(output_format == RAW && (option_values.format_is_big_endian < 0 || option_values.format_is_unsigned_samples < 0))
			return usage_error("ERROR: for decoding to a raw file you must specify a value for --endian and --sign\n");
	}

	if(option_values.keep_foreign_metadata) {
		if(0 == strcmp(infilename, "-") || 0 == strcmp(outfilename, "-"))
			return usage_error("ERROR: --keep-foreign-metadata cannot be used when decoding from stdin or to stdout\n");
		if(output_format != WAV && output_format != AIF)
			return usage_error("ERROR: --keep-foreign-metadata can only be used with WAVE or AIFF output\n");
	}

	if(option_values.use_ogg)
		treat_as_ogg = true;
	else if(strlen(infilename) >= 4 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-4), ".oga"))
		treat_as_ogg = true;
	else if(strlen(infilename) >= 4 && 0 == FLAC__STRCASECMP(infilename+(strlen(infilename)-4), ".ogg"))
		treat_as_ogg = true;
	else
		treat_as_ogg = false;

#if !FLAC__HAS_OGG
	if(treat_as_ogg) {
		flac__utils_printf(stderr, 1, "%s: Ogg support has not been built into this copy of flac\n", infilename);
		return 1;
	}
#endif

	if(!flac__utils_parse_skip_until_specification(option_values.skip_specification, &common_options.skip_specification) || common_options.skip_specification.is_relative)
		return usage_error("ERROR: invalid value for --skip\n");

	if(!flac__utils_parse_skip_until_specification(option_values.until_specification, &common_options.until_specification)) /*@@@ more checks: no + without --skip, no - unless known total_samples_to_{en,de}code */
		return usage_error("ERROR: invalid value for --until\n");
	/* if there is no "--until" we want to default to "--until=-0" */
	if(0 == option_values.until_specification)
		common_options.until_specification.is_relative = true;

	if(option_values.cue_specification) {
		if(!flac__utils_parse_cue_specification(option_values.cue_specification, &common_options.cue_specification))
			return usage_error("ERROR: invalid value for --cue\n");
		common_options.has_cue_specification = true;
	}
	else
		common_options.has_cue_specification = false;

	common_options.treat_warnings_as_errors = option_values.treat_warnings_as_errors;
	common_options.continue_through_decode_errors = option_values.continue_through_decode_errors;
	common_options.replaygain_synthesis_spec = option_values.replaygain_synthesis_spec;
#if FLAC__HAS_OGG
	common_options.is_ogg = treat_as_ogg;
	common_options.use_first_serial_number = !option_values.has_serial_number;
	common_options.serial_number = option_values.serial_number;
#endif
	common_options.channel_map_none = option_values.channel_map_none;

	if(output_format == RAW) {
		raw_decode_options_t options;

		options.common = common_options;
		options.is_big_endian = option_values.format_is_big_endian;
		options.is_unsigned_samples = option_values.format_is_unsigned_samples;

		retval = flac__decode_raw(infilename, option_values.test_only? 0 : outfilename, option_values.analyze, option_values.aopts, options);
	}
	else {
		wav_decode_options_t options;

		options.common = common_options;
		options.foreign_metadata = 0;

		/* read foreign metadata if requested */
		if(option_values.keep_foreign_metadata) {
			if(0 == (options.foreign_metadata = flac__foreign_metadata_new(output_format==AIF? FOREIGN_BLOCK_TYPE__AIFF : FOREIGN_BLOCK_TYPE__RIFF))) {
				flac__utils_printf(stderr, 1, "ERROR: creating foreign metadata object\n");
				return 1;
			}
		}

		if(output_format == AIF)
			retval = flac__decode_aiff(infilename, option_values.test_only? 0 : outfilename, option_values.analyze, option_values.aopts, options);
		else
			retval = flac__decode_wav(infilename, option_values.test_only? 0 : outfilename, option_values.analyze, option_values.aopts, options);

		if(options.foreign_metadata)
			flac__foreign_metadata_delete(options.foreign_metadata);
	}

	if(retval == 0 && strcmp(infilename, "-")) {
		if(strcmp(outfilename, "-"))
			grabbag__file_copy_metadata(infilename, outfilename);
		if(option_values.delete_input && !option_values.test_only && !option_values.analyze)
			unlink(infilename);
	}

	return retval;
}

const char *get_encoded_outfilename(const char *infilename)
{
	const char *suffix = (option_values.use_ogg? ".oga" : ".flac");
	return get_outfilename(infilename, suffix);
}

const char *get_decoded_outfilename(const char *infilename)
{
	const char *suffix;
	if(option_values.analyze) {
		suffix = ".ana";
	}
	else if(option_values.force_raw_format) {
		suffix = ".raw";
	}
	else if(option_values.force_aiff_format) {
		suffix = ".aiff";
	}
	else {
		suffix = ".wav";
	}
	return get_outfilename(infilename, suffix);
}

const char *get_outfilename(const char *infilename, const char *suffix)
{
	if(0 == option_values.cmdline_forced_outfilename) {
		static char buffer[4096]; /* @@@ bad MAGIC NUMBER */

		if(0 == strcmp(infilename, "-") || option_values.force_to_stdout) {
			strcpy(buffer, "-");
		}
		else {
			char *p;
			if (flac__strlcpy(buffer, option_values.output_prefix? option_values.output_prefix : "", sizeof buffer) >= sizeof buffer)
				return 0;
			if (flac__strlcat(buffer, infilename, sizeof buffer) >= sizeof buffer)
				return 0;
			/* the . must come after any / to avoid problems with, e.g. "some.directory/extensionless-filename" */
			if(0 == (p = strrchr(buffer, '.')) || strchr(p, '/')) {
				if (flac__strlcat(buffer, suffix, sizeof buffer) >= sizeof buffer)
					return 0;
			}
			else {
				*p = '\0';
				if (flac__strlcat(buffer, suffix, sizeof buffer) >= sizeof buffer)
					return 0;
			}
		}
		return buffer;
	}
	else
		return option_values.cmdline_forced_outfilename;
}

void die(const char *message)
{
	FLAC__ASSERT(0 != message);
	flac__utils_printf(stderr, 1, "ERROR: %s\n", message);
	exit(1);
}

int conditional_fclose(FILE *f)
{
	if(f == 0 || f == stdin || f == stdout)
		return 0;
	else
		return fclose(f);
}

char *local_strdup(const char *source)
{
	char *ret;
	FLAC__ASSERT(0 != source);
	if(0 == (ret = strdup(source)))
		die("out of memory during strdup()");
	return ret;
}

#ifdef _MSC_VER
/* There's no strtoll() in MSVC6 so we just write a specialized one */
FLAC__int64 local__strtoll(const char *src, char **endptr)
{
	FLAC__bool neg = false;
	FLAC__int64 ret = 0;
	int c;
	FLAC__ASSERT(0 != src);
	if(*src == '-') {
		neg = true;
		src++;
	}
	while(0 != (c = *src)) {
		c -= '0';
		if(c >= 0 && c <= 9)
			ret = (ret * 10) + c;
		else
			break;
		src++;
	}
	if(endptr)
		*endptr = (char*)src;
	return neg? -ret : ret;
}
#endif
