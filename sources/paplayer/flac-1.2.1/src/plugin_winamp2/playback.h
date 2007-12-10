/* in_flac - Winamp2 FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "FLAC/all.h"
#include "share/replaygain_synthesis.h"
#include "plugin_common/all.h"

/*
 *  constants
 */

#define SAMPLES_PER_WRITE           576

#define BITRATE_HIST_SEGMENT_MSEC   500
#define BITRATE_HIST_SIZE           64

/*
 *  common structures
 */

typedef struct {
	volatile FLAC__bool is_playing;
	volatile FLAC__bool abort_flag;
	volatile FLAC__bool eof;
	volatile int seek_to;
	FLAC__uint64 total_samples;
	unsigned bits_per_sample;
	unsigned output_bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	int length_in_msec; /* int (instead of FLAC__uint64) only because that's what Winamp uses; seeking won't work right if this maxes out */
	unsigned average_bps;
	FLAC__bool has_replaygain;
	double replay_scale;
	DitherContext dither_context;
} stream_data_struct;


typedef struct {
	struct {
		FLAC__bool enable;
		FLAC__bool album_mode;
		int  preamp;
		FLAC__bool hard_limit;
	} replaygain;
	struct {
		struct {
			FLAC__bool dither_24_to_16;
		} normal;
		struct {
			FLAC__bool dither;
			int  noise_shaping; /* value must be one of NoiseShaping enum, see plugin_common/replaygain_synthesis.h */
			int  bps_out;
		} replaygain;
	} resolution;
	struct {
		FLAC__bool stop_err;
	} misc;
} output_config_t;

/*
 *  protopytes
 */

FLAC__bool FLAC_plugin__decoder_init(FLAC__StreamDecoder *decoder, const char *filename, FLAC__int64 filesize, stream_data_struct *stream_data, output_config_t *config);
void FLAC_plugin__decoder_finish(FLAC__StreamDecoder *decoder);
void FLAC_plugin__decoder_delete(FLAC__StreamDecoder *decoder);

int FLAC_plugin__seek(FLAC__StreamDecoder *decoder, stream_data_struct *stream_data);
unsigned FLAC_plugin__decode(FLAC__StreamDecoder *decoder, stream_data_struct *stream_data, char *sample_buffer);
int FLAC_plugin__get_rate(unsigned written_time, unsigned output_time, stream_data_struct *stream_data);

/*
 *  these should be defined in plug-in
 */

extern void FLAC_plugin__show_error(const char *message,...);
