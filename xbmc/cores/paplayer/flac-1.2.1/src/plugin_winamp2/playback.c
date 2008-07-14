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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <limits.h> /* for INT_MAX */
#include <stdlib.h>
#include <string.h> /* for memmove() */
#include "playback.h"
#include "share/grabbag.h"


static FLAC__int32 reservoir_[FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS][FLAC__MAX_BLOCK_SIZE * 2/*for overflow*/];
static FLAC__int32 *reservoir__[FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS] = { reservoir_[0], reservoir_[1] }; /*@@@ kind of a hard-coded hack */
static unsigned wide_samples_in_reservoir_;
static output_config_t cfg;     /* local copy */

static unsigned bh_index_last_w, bh_index_last_o, written_time_last;
static FLAC__int64 decode_position, decode_position_last;

/*
 *  callbacks
 */

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	stream_data_struct *stream_data = (stream_data_struct*)client_data;
	const unsigned channels = stream_data->channels, wide_samples = frame->header.blocksize;
	unsigned channel;

	(void)decoder;

	if (stream_data->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	for (channel = 0; channel < channels; channel++)
		memcpy(&reservoir_[channel][wide_samples_in_reservoir_], buffer[channel], sizeof(buffer[0][0]) * wide_samples);

	wide_samples_in_reservoir_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	stream_data_struct *stream_data = (stream_data_struct*)client_data;
	(void)decoder;

	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		stream_data->total_samples = metadata->data.stream_info.total_samples;
		stream_data->bits_per_sample = metadata->data.stream_info.bits_per_sample;
		stream_data->channels = metadata->data.stream_info.channels;
		stream_data->sample_rate = metadata->data.stream_info.sample_rate;

		if (stream_data->bits_per_sample!=8 && stream_data->bits_per_sample!=16 && stream_data->bits_per_sample!=24)
		{
			FLAC_plugin__show_error("This plugin can only handle 8/16/24-bit samples.");
			stream_data->abort_flag = true;
			return;
		}

		{
			/* with VC++ you have to spoon feed it the casting from uint64->int64->double */
			FLAC__uint64 l = (FLAC__uint64)((double)(FLAC__int64)stream_data->total_samples / (double)stream_data->sample_rate * 1000.0 + 0.5);
			if (l > INT_MAX)
				l = INT_MAX;
			stream_data->length_in_msec = (int)l;
		}
	}
	else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
	{
		double reference, gain, peak;
		if (grabbag__replaygain_load_from_vorbiscomment(metadata, cfg.replaygain.album_mode, /*strict=*/false, &reference, &gain, &peak))
		{
			stream_data->has_replaygain = true;
			stream_data->replay_scale = grabbag__replaygain_compute_scale_factor(peak, gain, (double)cfg.replaygain.preamp, !cfg.replaygain.hard_limit);
		}
	}
}

static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	stream_data_struct *stream_data = (stream_data_struct*)client_data;
	(void)decoder;

	if (cfg.misc.stop_err || status!=FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		stream_data->abort_flag = true;
}

/*
 *  init/delete
 */

FLAC__bool FLAC_plugin__decoder_init(FLAC__StreamDecoder *decoder, const char *filename, FLAC__int64 filesize, stream_data_struct *stream_data, output_config_t *config)
{
	FLAC__StreamDecoderInitStatus init_status;

	FLAC__ASSERT(decoder);
	FLAC_plugin__decoder_finish(decoder);
	/* init decoder */
	FLAC__stream_decoder_set_md5_checking(decoder, false);
	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);

	if ((init_status = FLAC__stream_decoder_init_file(decoder, filename, write_callback, metadata_callback, error_callback, /*client_data=*/stream_data)) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		FLAC_plugin__show_error("Error while initializing decoder (%s [%s]).", FLAC__StreamDecoderInitStatusString[init_status], FLAC__stream_decoder_get_resolved_state_string(decoder));
		return false;
	}
	/* process */
	cfg = *config;
	wide_samples_in_reservoir_ = 0;
	stream_data->is_playing = false;
	stream_data->abort_flag = false;
	stream_data->has_replaygain = false;

	if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
	{
		FLAC_plugin__show_error("Error while processing metadata (%s).", FLAC__stream_decoder_get_resolved_state_string(decoder));
		return false;
	}
	/* check results */
	if (stream_data->abort_flag) return false;                /* metadata callback already popped up the error dialog */
	/* init replaygain */
	stream_data->output_bits_per_sample = stream_data->has_replaygain && cfg.replaygain.enable ?
		cfg.resolution.replaygain.bps_out :
		cfg.resolution.normal.dither_24_to_16 ? min(stream_data->bits_per_sample, 16) : stream_data->bits_per_sample;

	if (stream_data->has_replaygain && cfg.replaygain.enable && cfg.resolution.replaygain.dither)
		FLAC__replaygain_synthesis__init_dither_context(&stream_data->dither_context, stream_data->bits_per_sample, cfg.resolution.replaygain.noise_shaping);
	/* more inits */
	stream_data->eof = false;
	stream_data->seek_to = -1;
	stream_data->is_playing = true;
	stream_data->average_bps = (unsigned)(filesize / (125.*(double)(FLAC__int64)stream_data->total_samples/(double)stream_data->sample_rate));
	
	bh_index_last_w = 0;
	bh_index_last_o = BITRATE_HIST_SIZE;
	decode_position = 0;
	decode_position_last = 0;
	written_time_last = 0;

	return true;
}

void FLAC_plugin__decoder_finish(FLAC__StreamDecoder *decoder)
{
	if (decoder && FLAC__stream_decoder_get_state(decoder) != FLAC__STREAM_DECODER_UNINITIALIZED)
		(void)FLAC__stream_decoder_finish(decoder);
}

void FLAC_plugin__decoder_delete(FLAC__StreamDecoder *decoder)
{
	if (decoder)
	{
		FLAC_plugin__decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
	}
}

/*
 *  decode
 */

int FLAC_plugin__seek(FLAC__StreamDecoder *decoder, stream_data_struct *stream_data)
{
	int pos;
	FLAC__uint64 target_sample = stream_data->total_samples * stream_data->seek_to / stream_data->length_in_msec;

	if (stream_data->total_samples > 0 && target_sample >= stream_data->total_samples && target_sample > 0)
		target_sample = stream_data->total_samples - 1;

	/* even if the seek fails we have to reset these so that we don't repeat the seek */
	stream_data->seek_to = -1;
	stream_data->eof = false;
	wide_samples_in_reservoir_ = 0;
	pos = (int)(target_sample*1000 / stream_data->sample_rate);

	if (!FLAC__stream_decoder_seek_absolute(decoder, target_sample)) {
		if(FLAC__stream_decoder_get_state(decoder) == FLAC__STREAM_DECODER_SEEK_ERROR)
			FLAC__stream_decoder_flush(decoder);
		pos = -1;
	}

	bh_index_last_o = bh_index_last_w = (pos/BITRATE_HIST_SEGMENT_MSEC) % BITRATE_HIST_SIZE;
	if (!FLAC__stream_decoder_get_decode_position(decoder, &decode_position))
		decode_position = 0;

	return pos;
}

unsigned FLAC_plugin__decode(FLAC__StreamDecoder *decoder, stream_data_struct *stream_data, char *sample_buffer)
{
	/* fill reservoir */
	while (wide_samples_in_reservoir_ < SAMPLES_PER_WRITE)
	{
		if (FLAC__stream_decoder_get_state(decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		{
			stream_data->eof = true;
			break;
		}
		else if (!FLAC__stream_decoder_process_single(decoder))
		{
			FLAC_plugin__show_error("Error while processing frame (%s).", FLAC__stream_decoder_get_resolved_state_string(decoder));
			stream_data->eof = true;
			break;
		}
		if (!FLAC__stream_decoder_get_decode_position(decoder, &decode_position))
			decode_position = 0;
	}
	/* output samples */
	if (wide_samples_in_reservoir_ > 0)
	{
		const unsigned n = min(wide_samples_in_reservoir_, SAMPLES_PER_WRITE);
		const unsigned channels = stream_data->channels;
		unsigned i;
		int bytes;

		if (cfg.replaygain.enable && stream_data->has_replaygain)
		{
			bytes = FLAC__replaygain_synthesis__apply_gain(
				sample_buffer,
				true, /* little_endian_data_out */
				stream_data->output_bits_per_sample == 8, /* unsigned_data_out */
				reservoir__,
				n,
				channels,
				stream_data->bits_per_sample,
				stream_data->output_bits_per_sample,
				stream_data->replay_scale,
				cfg.replaygain.hard_limit,
				cfg.resolution.replaygain.dither,
				&stream_data->dither_context
			);
		}
		else
		{
			bytes = FLAC__plugin_common__pack_pcm_signed_little_endian(
				sample_buffer,
				reservoir__,
				n,
				channels,
				stream_data->bits_per_sample,
				stream_data->output_bits_per_sample
			);
		}

		wide_samples_in_reservoir_ -= n;
		for (i = 0; i < channels; i++)
			memmove(&reservoir_[i][0], &reservoir_[i][n], sizeof(reservoir_[0][0]) * wide_samples_in_reservoir_);

		return bytes;
	}
	else
	{
		stream_data->eof = true;
		return 0;
	}
}

int FLAC_plugin__get_rate(unsigned written_time, unsigned output_time, stream_data_struct *stream_data)
{
	static int bitrate_history_[BITRATE_HIST_SIZE];
	unsigned bh_index_w = (written_time/BITRATE_HIST_SEGMENT_MSEC) % BITRATE_HIST_SIZE;
	unsigned bh_index_o = (output_time/BITRATE_HIST_SEGMENT_MSEC) % BITRATE_HIST_SIZE;

	/* written bitrate */
	if (bh_index_w != bh_index_last_w)
	{
		bitrate_history_[(bh_index_w + BITRATE_HIST_SIZE-1)%BITRATE_HIST_SIZE] =
			decode_position>decode_position_last && written_time > written_time_last ?
			(unsigned)(8000*(decode_position - decode_position_last)/(written_time - written_time_last)) :
			stream_data->average_bps;

		bh_index_last_w = bh_index_w;
		written_time_last = written_time;
		decode_position_last = decode_position;
	}

	/* output bitrate */
	if (bh_index_o!=bh_index_last_o && bh_index_o!=bh_index_last_w)
	{
		bh_index_last_o = bh_index_o;
		return bitrate_history_[bh_index_o];
	}

	return 0;
}
