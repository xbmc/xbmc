/* libxmms-flac - XMMS FLAC input plugin
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

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <xmms/plugin.h>
#include <xmms/util.h>
#include <xmms/configfile.h>
#include <xmms/titlestring.h>

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "FLAC/all.h"
#include "plugin_common/all.h"
#include "share/grabbag.h"
#include "share/replaygain_synthesis.h"
#include "configure.h"
#include "charset.h"
#include "http.h"
#include "tag.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

extern void FLAC_XMMS__file_info_box(char *filename);

typedef struct {
	FLAC__bool abort_flag;
	FLAC__bool is_playing;
	FLAC__bool is_http_source;
	FLAC__bool eof;
	FLAC__bool play_thread_open; /* if true, is_playing must also be true */
	FLAC__uint64 total_samples;
	unsigned bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	int length_in_msec; /* int (instead of FLAC__uint64) only because that's what XMMS uses; seeking won't work right if this maxes out */
	gchar *title;
	AFormat sample_format;
	unsigned sample_format_bytes_per_sample;
	int seek_to_in_sec;
	FLAC__bool has_replaygain;
	double replay_scale;
	DitherContext dither_context;
} stream_data_struct;

static void FLAC_XMMS__init(void);
static int  FLAC_XMMS__is_our_file(char *filename);
static void FLAC_XMMS__play_file(char *filename);
static void FLAC_XMMS__stop(void);
static void FLAC_XMMS__pause(short p);
static void FLAC_XMMS__seek(int time);
static int  FLAC_XMMS__get_time(void);
static void FLAC_XMMS__cleanup(void);
static void FLAC_XMMS__get_song_info(char *filename, char **title, int *length);

static void *play_loop_(void *arg);

static FLAC__bool safe_decoder_init_(const char *filename, FLAC__StreamDecoder *decoder);
static void safe_decoder_finish_(FLAC__StreamDecoder *decoder);
static void safe_decoder_delete_(FLAC__StreamDecoder *decoder);

static FLAC__StreamDecoderReadStatus http_read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

InputPlugin flac_ip =
{
	NULL,
	NULL,
	"FLAC Player v" VERSION,
	FLAC_XMMS__init,
	FLAC_XMMS__aboutbox,
	FLAC_XMMS__configure,
	FLAC_XMMS__is_our_file,
	NULL,
	FLAC_XMMS__play_file,
	FLAC_XMMS__stop,
	FLAC_XMMS__pause,
	FLAC_XMMS__seek,
	NULL,
	FLAC_XMMS__get_time,
	NULL,
	NULL,
	FLAC_XMMS__cleanup,
	NULL,
	NULL,
	NULL,
	NULL,
	FLAC_XMMS__get_song_info,
	FLAC_XMMS__file_info_box,
	NULL
};

#define SAMPLES_PER_WRITE 512
#define SAMPLE_BUFFER_SIZE ((FLAC__MAX_BLOCK_SIZE + SAMPLES_PER_WRITE) * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS * (24/8))
static FLAC__byte sample_buffer_[SAMPLE_BUFFER_SIZE];
static unsigned sample_buffer_first_, sample_buffer_last_;

static FLAC__StreamDecoder *decoder_ = 0;
static stream_data_struct stream_data_;
static pthread_t decode_thread_;
static FLAC__bool audio_error_ = false;
static FLAC__bool is_big_endian_host_;

#define BITRATE_HIST_SEGMENT_MSEC 500
/* 500ms * 50 = 25s should be enough */
#define BITRATE_HIST_SIZE 50
static unsigned bitrate_history_[BITRATE_HIST_SIZE];


InputPlugin *get_iplugin_info(void)
{
	flac_ip.description = g_strdup_printf("Reference FLAC Player v%s", FLAC__VERSION_STRING);
	return &flac_ip;
}

void set_track_info(const char* title, int length_in_msec)
{
	if (stream_data_.is_playing) {
		flac_ip.set_info((char*) title, length_in_msec, stream_data_.sample_rate * stream_data_.channels * stream_data_.bits_per_sample, stream_data_.sample_rate, stream_data_.channels);
	}
}

static gchar* homedir(void)
{
	gchar *result;
	char *env_home = getenv("HOME");
	if (env_home) {
		result = g_strdup (env_home);
	} else {
		uid_t uid = getuid();
		struct passwd *pwent;
		do {
			pwent = getpwent();
		} while (pwent && pwent->pw_uid != uid);
		result = pwent ? g_strdup (pwent->pw_dir) : NULL;
		endpwent();
	}
	return result;
}

static FLAC__bool is_http_source(const char *source)
{
	return 0 == strncasecmp(source, "http://", 7);
}

void FLAC_XMMS__init(void)
{
	ConfigFile *cfg;
	FLAC__uint32 test = 1;

	is_big_endian_host_ = (*((FLAC__byte*)(&test)))? false : true;

	flac_cfg.title.tag_override = FALSE;
	if (flac_cfg.title.tag_format)
		g_free(flac_cfg.title.tag_format);
	flac_cfg.title.convert_char_set = FALSE;

	cfg = xmms_cfg_open_default_file();

	/* title */

	xmms_cfg_read_boolean(cfg, "flac", "title.tag_override", &flac_cfg.title.tag_override);

	if(!xmms_cfg_read_string(cfg, "flac", "title.tag_format", &flac_cfg.title.tag_format))
		flac_cfg.title.tag_format = g_strdup("%p - %t");

	xmms_cfg_read_boolean(cfg, "flac", "title.convert_char_set", &flac_cfg.title.convert_char_set);

	if(!xmms_cfg_read_string(cfg, "flac", "title.user_char_set", &flac_cfg.title.user_char_set))
		flac_cfg.title.user_char_set = FLAC_plugin__charset_get_current();

	/* replaygain */

	xmms_cfg_read_boolean(cfg, "flac", "output.replaygain.enable", &flac_cfg.output.replaygain.enable);

	xmms_cfg_read_boolean(cfg, "flac", "output.replaygain.album_mode", &flac_cfg.output.replaygain.album_mode);

	if(!xmms_cfg_read_int(cfg, "flac", "output.replaygain.preamp", &flac_cfg.output.replaygain.preamp))
		flac_cfg.output.replaygain.preamp = 0;

	xmms_cfg_read_boolean(cfg, "flac", "output.replaygain.hard_limit", &flac_cfg.output.replaygain.hard_limit);

	xmms_cfg_read_boolean(cfg, "flac", "output.resolution.normal.dither_24_to_16", &flac_cfg.output.resolution.normal.dither_24_to_16);
	xmms_cfg_read_boolean(cfg, "flac", "output.resolution.replaygain.dither", &flac_cfg.output.resolution.replaygain.dither);

	if(!xmms_cfg_read_int(cfg, "flac", "output.resolution.replaygain.noise_shaping", &flac_cfg.output.resolution.replaygain.noise_shaping))
		flac_cfg.output.resolution.replaygain.noise_shaping = 1;

	if(!xmms_cfg_read_int(cfg, "flac", "output.resolution.replaygain.bps_out", &flac_cfg.output.resolution.replaygain.bps_out))
		flac_cfg.output.resolution.replaygain.bps_out = 16;

	/* stream */

	xmms_cfg_read_int(cfg, "flac", "stream.http_buffer_size", &flac_cfg.stream.http_buffer_size);
	xmms_cfg_read_int(cfg, "flac", "stream.http_prebuffer", &flac_cfg.stream.http_prebuffer);
	xmms_cfg_read_boolean(cfg, "flac", "stream.use_proxy", &flac_cfg.stream.use_proxy);
	if(flac_cfg.stream.proxy_host)
		g_free(flac_cfg.stream.proxy_host);
	if(!xmms_cfg_read_string(cfg, "flac", "stream.proxy_host", &flac_cfg.stream.proxy_host))
		flac_cfg.stream.proxy_host = g_strdup("");
	xmms_cfg_read_int(cfg, "flac", "stream.proxy_port", &flac_cfg.stream.proxy_port);
	xmms_cfg_read_boolean(cfg, "flac", "stream.proxy_use_auth", &flac_cfg.stream.proxy_use_auth);
	if(flac_cfg.stream.proxy_user)
		g_free(flac_cfg.stream.proxy_user);
	flac_cfg.stream.proxy_user = NULL;
	xmms_cfg_read_string(cfg, "flac", "stream.proxy_user", &flac_cfg.stream.proxy_user);
	if(flac_cfg.stream.proxy_pass)
		g_free(flac_cfg.stream.proxy_pass);
	flac_cfg.stream.proxy_pass = NULL;
	xmms_cfg_read_string(cfg, "flac", "stream.proxy_pass", &flac_cfg.stream.proxy_pass);
	xmms_cfg_read_boolean(cfg, "flac", "stream.save_http_stream", &flac_cfg.stream.save_http_stream);
	if (flac_cfg.stream.save_http_path)
		g_free (flac_cfg.stream.save_http_path);
	if (!xmms_cfg_read_string(cfg, "flac", "stream.save_http_path", &flac_cfg.stream.save_http_path) || ! *flac_cfg.stream.save_http_path) {
		if (flac_cfg.stream.save_http_path)
			g_free (flac_cfg.stream.save_http_path);
		flac_cfg.stream.save_http_path = homedir();
	}
	xmms_cfg_read_boolean(cfg, "flac", "stream.cast_title_streaming", &flac_cfg.stream.cast_title_streaming);
	xmms_cfg_read_boolean(cfg, "flac", "stream.use_udp_channel", &flac_cfg.stream.use_udp_channel);

	decoder_ = FLAC__stream_decoder_new();

	xmms_cfg_free(cfg);
}

int FLAC_XMMS__is_our_file(char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if(ext)
		if(!strcasecmp(ext, ".flac") || !strcasecmp(ext, ".fla"))
			return 1;
	return 0;
}

void FLAC_XMMS__play_file(char *filename)
{
	FILE *f;

	sample_buffer_first_ = sample_buffer_last_ = 0;
	audio_error_ = false;
	stream_data_.abort_flag = false;
	stream_data_.is_playing = false;
	stream_data_.is_http_source = is_http_source(filename);
	stream_data_.eof = false;
	stream_data_.play_thread_open = false;
	stream_data_.has_replaygain = false;

	if(!is_http_source(filename)) {
		if(0 == (f = fopen(filename, "r")))
			return;
		fclose(f);
	}

	if(decoder_ == 0)
		return;

	if(!safe_decoder_init_(filename, decoder_))
		return;

	if(stream_data_.has_replaygain && flac_cfg.output.replaygain.enable) {
		if(flac_cfg.output.resolution.replaygain.bps_out == 8) {
			stream_data_.sample_format = FMT_U8;
			stream_data_.sample_format_bytes_per_sample = 1;
		}
		else if(flac_cfg.output.resolution.replaygain.bps_out == 16) {
			stream_data_.sample_format = (is_big_endian_host_) ? FMT_S16_BE : FMT_S16_LE;
			stream_data_.sample_format_bytes_per_sample = 2;
		}
		else {
			/*@@@ need some error here like wa2: MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16-bit samples\n", "ERROR: plugin can only handle 8/16-bit samples", 0); */
			fprintf(stderr, "libxmms-flac: can't handle %d bit output\n", flac_cfg.output.resolution.replaygain.bps_out);
			safe_decoder_finish_(decoder_);
			return;
		}
	}
	else {
		if(stream_data_.bits_per_sample == 8) {
			stream_data_.sample_format = FMT_U8;
			stream_data_.sample_format_bytes_per_sample = 1;
		}
		else if(stream_data_.bits_per_sample == 16 || (stream_data_.bits_per_sample == 24 && flac_cfg.output.resolution.normal.dither_24_to_16)) {
			stream_data_.sample_format = (is_big_endian_host_) ? FMT_S16_BE : FMT_S16_LE;
			stream_data_.sample_format_bytes_per_sample = 2;
		}
		else {
			/*@@@ need some error here like wa2: MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16-bit samples\n", "ERROR: plugin can only handle 8/16-bit samples", 0); */
			fprintf(stderr, "libxmms-flac: can't handle %d bit output\n", stream_data_.bits_per_sample);
			safe_decoder_finish_(decoder_);
			return;
		}
	}
	FLAC__replaygain_synthesis__init_dither_context(&stream_data_.dither_context, stream_data_.sample_format_bytes_per_sample * 8, flac_cfg.output.resolution.replaygain.noise_shaping);
	stream_data_.is_playing = true;

	if(flac_ip.output->open_audio(stream_data_.sample_format, stream_data_.sample_rate, stream_data_.channels) == 0) {
		audio_error_ = true;
		safe_decoder_finish_(decoder_);
		return;
	}

	stream_data_.title = flac_format_song_title(filename);
	flac_ip.set_info(stream_data_.title, stream_data_.length_in_msec, stream_data_.sample_rate * stream_data_.channels * stream_data_.bits_per_sample, stream_data_.sample_rate, stream_data_.channels);

	stream_data_.seek_to_in_sec = -1;
	stream_data_.play_thread_open = true;
	pthread_create(&decode_thread_, NULL, play_loop_, NULL);
}

void FLAC_XMMS__stop(void)
{
	if(stream_data_.is_playing) {
		stream_data_.is_playing = false;
		if(stream_data_.play_thread_open) {
			stream_data_.play_thread_open = false;
			pthread_join(decode_thread_, NULL);
		}
		flac_ip.output->close_audio();
		safe_decoder_finish_(decoder_);
	}
}

void FLAC_XMMS__pause(short p)
{
	flac_ip.output->pause(p);
}

void FLAC_XMMS__seek(int time)
{
	if(!stream_data_.is_http_source) {
		stream_data_.seek_to_in_sec = time;
		stream_data_.eof = false;

		while(stream_data_.seek_to_in_sec != -1)
			xmms_usleep(10000);
	}
}

int FLAC_XMMS__get_time(void)
{
	if(audio_error_)
		return -2;
	if(!stream_data_.is_playing || (stream_data_.eof && !flac_ip.output->buffer_playing()))
		return -1;
	else
		return flac_ip.output->output_time();
}

void FLAC_XMMS__cleanup(void)
{
	safe_decoder_delete_(decoder_);
	decoder_ = 0;
}

void FLAC_XMMS__get_song_info(char *filename, char **title, int *length_in_msec)
{
	FLAC__StreamMetadata streaminfo;

	if(0 == filename)
		filename = "";

	if(!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		/* @@@ how to report the error? */
		if(title) {
			if (!is_http_source(filename)) {
				static const char *errtitle = "Invalid FLAC File: ";
				if(strlen(errtitle) + 1 + strlen(filename) + 1 + 1 < strlen(filename)) { /* overflow check */
					*title = NULL;
				}
				else {
					*title = g_malloc(strlen(errtitle) + 1 + strlen(filename) + 1 + 1);
					sprintf(*title, "%s\"%s\"", errtitle, filename);
				}
			} else {
				*title = NULL;
			}
		}
		if(length_in_msec)
			*length_in_msec = -1;
		return;
	}

	if(title) {
		*title = flac_format_song_title(filename);
	}
	if(length_in_msec) {
		FLAC__uint64 l = (FLAC__uint64)((double)streaminfo.data.stream_info.total_samples / (double)streaminfo.data.stream_info.sample_rate * 1000.0 + 0.5);
		if (l > INT_MAX)
			l = INT_MAX;
		*length_in_msec = (int)l;
	}
}

/***********************************************************************
 * local routines
 **********************************************************************/

void *play_loop_(void *arg)
{
	unsigned written_time_last = 0, bh_index_last_w = 0, bh_index_last_o = BITRATE_HIST_SIZE, blocksize = 1;
	FLAC__uint64 decode_position_last = 0, decode_position_frame_last = 0, decode_position_frame = 0;

	(void)arg;

	while(stream_data_.is_playing) {
		if(!stream_data_.eof) {
			while(sample_buffer_last_ - sample_buffer_first_ < SAMPLES_PER_WRITE) {
				unsigned s;

				s = sample_buffer_last_ - sample_buffer_first_;
				if(FLAC__stream_decoder_get_state(decoder_) == FLAC__STREAM_DECODER_END_OF_STREAM) {
					stream_data_.eof = true;
					break;
				}
				else if(!FLAC__stream_decoder_process_single(decoder_)) {
					/*@@@ this should probably be a dialog */
					fprintf(stderr, "libxmms-flac: READ ERROR processing frame\n");
					stream_data_.eof = true;
					break;
				}
				blocksize = sample_buffer_last_ - sample_buffer_first_ - s;
				decode_position_frame_last = decode_position_frame;
				if(stream_data_.is_http_source || !FLAC__stream_decoder_get_decode_position(decoder_, &decode_position_frame))
					decode_position_frame = 0;
			}
			if(sample_buffer_last_ - sample_buffer_first_ > 0) {
				const unsigned n = min(sample_buffer_last_ - sample_buffer_first_, SAMPLES_PER_WRITE);
				int bytes = n * stream_data_.channels * stream_data_.sample_format_bytes_per_sample;
				FLAC__byte *sample_buffer_start = sample_buffer_ + sample_buffer_first_ * stream_data_.channels * stream_data_.sample_format_bytes_per_sample;
				unsigned written_time, bh_index_w;
				FLAC__uint64 decode_position;

				sample_buffer_first_ += n;
				flac_ip.add_vis_pcm(flac_ip.output->written_time(), stream_data_.sample_format, stream_data_.channels, bytes, sample_buffer_start);
				while(flac_ip.output->buffer_free() < (int)bytes && stream_data_.is_playing && stream_data_.seek_to_in_sec == -1)
					xmms_usleep(10000);
				if(stream_data_.is_playing && stream_data_.seek_to_in_sec == -1)
					flac_ip.output->write_audio(sample_buffer_start, bytes);

				/* compute current bitrate */

				written_time = flac_ip.output->written_time();
				bh_index_w = written_time / BITRATE_HIST_SEGMENT_MSEC % BITRATE_HIST_SIZE;
				if(bh_index_w != bh_index_last_w) {
					bh_index_last_w = bh_index_w;
					decode_position = decode_position_frame - (double)(sample_buffer_last_ - sample_buffer_first_) * (double)(decode_position_frame - decode_position_frame_last) / (double)blocksize;
					bitrate_history_[(bh_index_w + BITRATE_HIST_SIZE - 1) % BITRATE_HIST_SIZE] =
						decode_position > decode_position_last && written_time > written_time_last ?
							8000 * (decode_position - decode_position_last) / (written_time - written_time_last) :
							stream_data_.sample_rate * stream_data_.channels * stream_data_.bits_per_sample;
					decode_position_last = decode_position;
					written_time_last = written_time;
				}
			}
			else {
				stream_data_.eof = true;
				xmms_usleep(10000);
			}
		}
		else
			xmms_usleep(10000);
		if(!stream_data_.is_http_source && stream_data_.seek_to_in_sec != -1) {
			const double distance = (double)stream_data_.seek_to_in_sec * 1000.0 / (double)stream_data_.length_in_msec;
			FLAC__uint64 target_sample = (FLAC__uint64)(distance * (double)stream_data_.total_samples);
			if(stream_data_.total_samples > 0 && target_sample >= stream_data_.total_samples)
				target_sample = stream_data_.total_samples - 1;
			if(FLAC__stream_decoder_seek_absolute(decoder_, target_sample)) {
				flac_ip.output->flush(stream_data_.seek_to_in_sec * 1000);
				bh_index_last_w = bh_index_last_o = flac_ip.output->output_time() / BITRATE_HIST_SEGMENT_MSEC % BITRATE_HIST_SIZE;
				if(!FLAC__stream_decoder_get_decode_position(decoder_, &decode_position_frame))
					decode_position_frame = 0;
				stream_data_.eof = false;
				sample_buffer_first_ = sample_buffer_last_ = 0;
			}
			else if(FLAC__stream_decoder_get_state(decoder_) == FLAC__STREAM_DECODER_SEEK_ERROR) {
				/*@@@ this should probably be a dialog */
				fprintf(stderr, "libxmms-flac: SEEK ERROR\n");
				FLAC__stream_decoder_flush(decoder_);
				stream_data_.eof = false;
				sample_buffer_first_ = sample_buffer_last_ = 0;
			}
			stream_data_.seek_to_in_sec = -1;
		}
		else {
			/* display the right bitrate from history */
			unsigned bh_index_o = flac_ip.output->output_time() / BITRATE_HIST_SEGMENT_MSEC % BITRATE_HIST_SIZE;
			if(bh_index_o != bh_index_last_o && bh_index_o != bh_index_last_w && bh_index_o != (bh_index_last_w + 1) % BITRATE_HIST_SIZE) {
				bh_index_last_o = bh_index_o;
				flac_ip.set_info(stream_data_.title, stream_data_.length_in_msec, bitrate_history_[bh_index_o], stream_data_.sample_rate, stream_data_.channels);
			}
		}
	}

	safe_decoder_finish_(decoder_);

	/* are these two calls necessary? */
	flac_ip.output->buffer_free();
	flac_ip.output->buffer_free();

	g_free(stream_data_.title);

	pthread_exit(NULL);
	return 0; /* to silence the compiler warning about not returning a value */
}

FLAC__bool safe_decoder_init_(const char *filename, FLAC__StreamDecoder *decoder)
{
	if(decoder == 0)
		return false;

	safe_decoder_finish_(decoder);

	FLAC__stream_decoder_set_md5_checking(decoder, false);
	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(stream_data_.is_http_source) {
		flac_http_open(filename, 0);
		if(FLAC__stream_decoder_init_stream(decoder, http_read_callback_, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, /*eof_callback=*/0, write_callback_, metadata_callback_, error_callback_, /*client_data=*/&stream_data_) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
			return false;
	}
	else {
		if(FLAC__stream_decoder_init_file(decoder, filename, write_callback_, metadata_callback_, error_callback_, /*client_data=*/&stream_data_) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
			return false;
	}

	if(!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
		return false;

	return true;
}

void safe_decoder_finish_(FLAC__StreamDecoder *decoder)
{
	if(decoder && FLAC__stream_decoder_get_state(decoder) != FLAC__STREAM_DECODER_UNINITIALIZED)
		(void)FLAC__stream_decoder_finish(decoder);
	if(stream_data_.is_http_source)
		flac_http_close();
}

void safe_decoder_delete_(FLAC__StreamDecoder *decoder)
{
	if(decoder) {
		safe_decoder_finish_(decoder);
		FLAC__stream_decoder_delete(decoder);
	}
}

FLAC__StreamDecoderReadStatus http_read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	(void)decoder;
	(void)client_data;
	*bytes = flac_http_read(buffer, *bytes);
	return *bytes ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	stream_data_struct *stream_data = (stream_data_struct *)client_data;
	const unsigned channels = stream_data->channels, wide_samples = frame->header.blocksize;
	const unsigned bits_per_sample = stream_data->bits_per_sample;
	FLAC__byte *sample_buffer_start;

	(void)decoder;

	if(stream_data->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if((sample_buffer_last_ + wide_samples) > (SAMPLE_BUFFER_SIZE / (channels * stream_data->sample_format_bytes_per_sample))) {
		memmove(sample_buffer_, sample_buffer_ + sample_buffer_first_ * channels * stream_data->sample_format_bytes_per_sample, (sample_buffer_last_ - sample_buffer_first_) * channels * stream_data->sample_format_bytes_per_sample);
		sample_buffer_last_ -= sample_buffer_first_;
		sample_buffer_first_ = 0;
	}
	sample_buffer_start = sample_buffer_ + sample_buffer_last_ * channels * stream_data->sample_format_bytes_per_sample;
	if(stream_data->has_replaygain && flac_cfg.output.replaygain.enable) {
		FLAC__replaygain_synthesis__apply_gain(
				sample_buffer_start,
				!is_big_endian_host_,
				stream_data->sample_format_bytes_per_sample == 1, /* unsigned_data_out */
				buffer,
				wide_samples,
				channels,
				bits_per_sample,
				stream_data->sample_format_bytes_per_sample * 8,
				stream_data->replay_scale,
				flac_cfg.output.replaygain.hard_limit,
				flac_cfg.output.resolution.replaygain.dither,
				&stream_data->dither_context
		);
	}
	else if(is_big_endian_host_) {
		FLAC__plugin_common__pack_pcm_signed_big_endian(
			sample_buffer_start,
			buffer,
			wide_samples,
			channels,
			bits_per_sample,
			stream_data->sample_format_bytes_per_sample * 8
		);
	}
	else {
		FLAC__plugin_common__pack_pcm_signed_little_endian(
			sample_buffer_start,
			buffer,
			wide_samples,
			channels,
			bits_per_sample,
			stream_data->sample_format_bytes_per_sample * 8
		);
	}

	sample_buffer_last_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	stream_data_struct *stream_data = (stream_data_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		stream_data->total_samples = metadata->data.stream_info.total_samples;
		stream_data->bits_per_sample = metadata->data.stream_info.bits_per_sample;
		stream_data->channels = metadata->data.stream_info.channels;
		stream_data->sample_rate = metadata->data.stream_info.sample_rate;
		{
			FLAC__uint64 l = (FLAC__uint64)((double)stream_data->total_samples / (double)stream_data->sample_rate * 1000.0 + 0.5);
			if (l > INT_MAX)
				l = INT_MAX;
			stream_data->length_in_msec = (int)l;
		}
	}
	else if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		double reference, gain, peak;
		if(grabbag__replaygain_load_from_vorbiscomment(metadata, flac_cfg.output.replaygain.album_mode, /*strict=*/false, &reference, &gain, &peak)) {
			stream_data->has_replaygain = true;
			stream_data->replay_scale = grabbag__replaygain_compute_scale_factor(peak, gain, (double)flac_cfg.output.replaygain.preamp, /*prevent_clipping=*/!flac_cfg.output.replaygain.hard_limit);
		}
	}
}

void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	stream_data_struct *stream_data = (stream_data_struct *)client_data;
	(void)decoder;
	if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		stream_data->abort_flag = true;
}
