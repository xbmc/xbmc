/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    flac_a.c
	Written by Iwata <b6330015@kit.jp>
    Functions to output FLAC / OggFLAC  (*.flac, *.ogg).
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <fcntl.h>

#ifdef __W32__
#include <io.h>
#include <time.h>
#endif

#if defined(AU_FLAC_DLL) || defined(AU_OGGFLAC_DLL)
#include <windows.h>
#define FLAC__EXPORT_H  /* don't include "OggFLAC/export.h" */
#define FLAC_API
#define OggFLAC__EXPORT_H  /* don't include "FLAC/export.h" */
#define OggFLAC_API
#endif

#include <FLAC/all.h>
#ifdef AU_OGGFLAC
#include <OggFLAC/stream_encoder.h>
#endif

#ifdef AU_FLAC_DLL
#include "w32_libFLAC_dll_g.h"
#endif
#ifdef AU_OGGFLAC_DLL
#include "w32_libOGGFLAC_dll_g.h"
#endif

#include "timidity.h"
#include "output.h"
#include "controls.h"
#include "timer.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "miditrace.h"

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);

/* export the playback mode */

#define dpm flac_play_mode

PlayMode dpm = {
  DEFAULT_RATE, PE_SIGNED|PE_16BIT, PF_PCM_STREAM,
  -1,
  {0}, /* default: get all the buffer fragments you can */
#ifndef AU_OGGFLAC
  "FLAC", 'F',
#else
  "FLAC / OggFLAC", 'F',
#endif /* AU_OGGFLAC */
  NULL,
  open_output,
  close_output,
  output_data,
  acntl
};

#ifdef __W32G__
extern char *w32g_output_dir;
extern int w32g_auto_output_mode;
#endif

typedef struct {
  unsigned long in_bytes;
  unsigned long out_bytes;
  union {
    FLAC__StreamEncoderState flac;
    FLAC__SeekableStreamEncoderState s_flac;
#ifdef AU_OGGFLAC
    OggFLAC__StreamEncoderState ogg;
#endif
  } state;
  union {
    union {
      FLAC__StreamEncoder *stream;
      FLAC__SeekableStreamEncoder *s_stream;
    } flac;
#ifdef AU_OGGFLAC
    union {
      OggFLAC__StreamEncoder *stream;
    } ogg;
#endif
  } encoder;
} FLAC_ctx;

typedef struct {
#ifdef AU_OGGFLAC
  int isogg;
#endif
  int verify;
  int padding;
  int blocksize;
  int mid_side;
  int adaptive_mid_side;
  int exhaustive_model_search;
  int max_lpc_order;
  int qlp_coeff_precision_search;
  int qlp_coeff_precision;
  int min_residual_partition_order;
  int max_residual_partition_order;
	int seekable;
} FLAC_options;

/* default compress level is 5 */
FLAC_options flac_options = {
#ifdef AU_OGGFLAC
  0,    /* isogg */
#endif
  0,    /* verify */
  4096, /* padding */
  4608, /* blocksize */
  1,    /* mid_side */
  0,    /* adaptive_mid_side */
  0,    /* exhaustive-model-search */
  8,    /* max_lpc_order */
  0,    /* qlp_coeff_precision_search */
  0,    /* qlp_coeff_precision */
  3,    /* min_residual_partition_order */
  3,     /* max_residual_partition_order */
	0,		/* seekable */
};

static long serial_number = 0;
FLAC_ctx *flac_ctx = NULL;

#ifdef AU_OGGFLAC
static FLAC__StreamEncoderWriteStatus
ogg_stream_encoder_write_callback(const OggFLAC__StreamEncoder *encoder,
				  const FLAC__byte buffer[],
				  unsigned bytes, unsigned samples,
				  unsigned current_frame, void *client_data);
#endif
static FLAC__StreamEncoderWriteStatus
flac_stream_encoder_write_callback(const FLAC__StreamEncoder *encoder,
				   const FLAC__byte buffer[],
				   unsigned bytes, unsigned samples,
				   unsigned current_frame, void *client_data);
static void flac_stream_encoder_metadata_callback(const FLAC__StreamEncoder *encoder,
						  const FLAC__StreamMetadata *metadata,
						  void *client_data);
static FLAC__StreamEncoderWriteStatus
flac_seekable_stream_encoder_write_callback(const FLAC__SeekableStreamEncoder *encoder,
				   const FLAC__byte buffer[],
				   unsigned bytes, unsigned samples,
				   unsigned current_frame, void *client_data);
static void flac_seekable_stream_encoder_metadata_callback(const FLAC__SeekableStreamEncoder *encoder,
						  const FLAC__StreamMetadata *metadata,
						  void *client_data);

/* preset */
void flac_set_compression_level(int compression_level)
{
  switch (compression_level) {
  case 0:
    flac_options.max_lpc_order = 0;
    flac_options.blocksize = 1152;
    flac_options.mid_side = 0;
    flac_options.adaptive_mid_side = 0;
    flac_options.min_residual_partition_order = 2;
    flac_options.max_residual_partition_order = 2;
    flac_options.exhaustive_model_search = 0;
    break;
  case 1:
    flac_options.max_lpc_order = 0;
    flac_options.blocksize = 1152;
    flac_options.mid_side = 0;
    flac_options.adaptive_mid_side = 1;
    flac_options.min_residual_partition_order = 2;
    flac_options.max_residual_partition_order = 2;
    flac_options.exhaustive_model_search = 0;
    break;
  case 2:
    flac_options.max_lpc_order = 0;
    flac_options.blocksize = 1152;
    flac_options.mid_side = 1;
    flac_options.adaptive_mid_side = 0;
    flac_options.min_residual_partition_order = 0;
    flac_options.max_residual_partition_order = 3;
    flac_options.exhaustive_model_search = 0;
    break;
  case 3:
    flac_options.max_lpc_order = 6;
    flac_options.blocksize = 4608;
    flac_options.mid_side = 0;
    flac_options.adaptive_mid_side = 0;
    flac_options.min_residual_partition_order = 3;
    flac_options.max_residual_partition_order = 3;
    flac_options.exhaustive_model_search = 0;
    break;
  case 4:
    flac_options.max_lpc_order = 8;
    flac_options.blocksize = 4608;
    flac_options.mid_side = 0;
    flac_options.adaptive_mid_side = 1;
    flac_options.min_residual_partition_order = 3;
    flac_options.max_residual_partition_order = 3;
    flac_options.exhaustive_model_search = 0;
    break;
  case 6:
    flac_options.max_lpc_order = 8;
    flac_options.blocksize = 4608;
    flac_options.mid_side = 1;
    flac_options.adaptive_mid_side = 0;
    flac_options.min_residual_partition_order = 0;
    flac_options.max_residual_partition_order = 4;
    flac_options.exhaustive_model_search = 0;
    break;
  case 7:
    flac_options.max_lpc_order = 8;
    flac_options.blocksize = 4608;
    flac_options.mid_side = 1;
    flac_options.adaptive_mid_side = 0;
    flac_options.min_residual_partition_order = 0;
    flac_options.max_residual_partition_order = 6;
    flac_options.exhaustive_model_search = 1;
    break;
  case 8:
    flac_options.max_lpc_order = 12;
    flac_options.blocksize = 4608;
    flac_options.mid_side = 1;
    flac_options.adaptive_mid_side = 0;
    flac_options.min_residual_partition_order = 0;
    flac_options.max_residual_partition_order = 6;
    flac_options.exhaustive_model_search = 1;
    break;
  case 5:
  default:
    flac_options.max_lpc_order = 8;
    flac_options.blocksize = 4608;
    flac_options.mid_side = 1;
    flac_options.adaptive_mid_side = 0;
    flac_options.min_residual_partition_order = 3;
    flac_options.max_residual_partition_order = 3;
    flac_options.exhaustive_model_search = 0;
  }
}

void flac_set_option_padding(int padding)
{
  flac_options.padding = padding;
}
void flac_set_option_verify(int verify)
{
  flac_options.verify = verify;
}
#ifdef AU_OGGFLAC
void flac_set_option_oggflac(int isogg)
{
  flac_options.isogg = isogg;
}
#endif

static int flac_session_close()
{
  FLAC_ctx *ctx = flac_ctx;

  if (dpm.fd > 0) {
    close(dpm.fd);
  }
  dpm.fd = -1;

  if (ctx != NULL) {
#ifdef AU_OGGFLAC
    if (flac_options.isogg) {
      if (ctx->encoder.ogg.stream) {
	OggFLAC__stream_encoder_finish(ctx->encoder.ogg.stream);
	OggFLAC__stream_encoder_delete(ctx->encoder.ogg.stream);
      }
    }
    else
#endif /* AU_OGGFLAC */
    if (flac_options.seekable) {
      if (ctx->encoder.flac.s_stream) {
	FLAC__seekable_stream_encoder_finish(ctx->encoder.flac.s_stream);
	FLAC__seekable_stream_encoder_delete(ctx->encoder.flac.s_stream);
      }
    }
    else
    {
      if (ctx->encoder.flac.stream) {
	FLAC__stream_encoder_finish(ctx->encoder.flac.stream);
	FLAC__stream_encoder_delete(ctx->encoder.flac.stream);
      }
    }
    free(ctx);
    flac_ctx = NULL;
  }
}

static int flac_output_open(const char *fname, const char *comment)
{
  int fd;
  int nch;
  FLAC__StreamMetadata padding;
  FLAC__StreamMetadata *metadata[4];
  int num_metadata = 0;

  FLAC_ctx *ctx;

  if (flac_ctx == NULL)
    flac_session_close();

  if (!(flac_ctx = (FLAC_ctx *)calloc(sizeof(FLAC_ctx), 1))) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s", strerror(errno));
    return -1;
  }

  ctx = flac_ctx;

  ctx->in_bytes = ctx->out_bytes = 0;

  if(strcmp(fname, "-") == 0) {
    fd = 1; /* data to stdout */
    if (comment == NULL)
      comment = "(stdout)";
  }
  else {
    /* Open the audio file */
    fd = open(fname, FILE_OUTPUT_MODE);
    if(fd < 0) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		fname, strerror(errno));
      return -1;
    }
    if(comment == NULL)
      comment = fname;
  }

  dpm.fd = fd;
  nch = (dpm.encoding & PE_MONO) ? 1 : 2;

  if (0 < flac_options.padding) {
    padding.is_last = 0;
    padding.type = FLAC__METADATA_TYPE_PADDING;
    padding.length = flac_options.padding;
    metadata[num_metadata++] = &padding;
  }

#ifdef AU_OGGFLAC
  if (flac_options.isogg) {
    if ((ctx->encoder.ogg.stream = OggFLAC__stream_encoder_new()) == NULL) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot create OggFLAC stream");
      flac_session_close();
      return -1;
    }

    OggFLAC__stream_encoder_set_channels(ctx->encoder.ogg.stream, nch);
    /* 16bps only */
    OggFLAC__stream_encoder_set_bits_per_sample(ctx->encoder.ogg.stream, 16);

    /* set sequential number for serial */
    serial_number++;
    if (serial_number == 1) {
      srand(time(NULL));
      serial_number = rand();
    }
    OggFLAC__stream_encoder_set_serial_number(ctx->encoder.ogg.stream, serial_number);

    OggFLAC__stream_encoder_set_verify(ctx->encoder.ogg.stream, flac_options.verify);

    if (!FLAC__format_sample_rate_is_valid(dpm.rate)) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "invalid sampling rate %d",
		dpm.rate);
      flac_session_close();
      return -1;
    }
    OggFLAC__stream_encoder_set_sample_rate(ctx->encoder.ogg.stream, dpm.rate);

    OggFLAC__stream_encoder_set_qlp_coeff_precision(ctx->encoder.ogg.stream, flac_options.qlp_coeff_precision);
    /* expensive! */
    OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(ctx->encoder.ogg.stream, flac_options.qlp_coeff_precision_search);

    if (nch == 2) {
      OggFLAC__stream_encoder_set_do_mid_side_stereo(ctx->encoder.ogg.stream, flac_options.mid_side);
      OggFLAC__stream_encoder_set_loose_mid_side_stereo(ctx->encoder.ogg.stream, flac_options.adaptive_mid_side);
    }

    OggFLAC__stream_encoder_set_max_lpc_order(ctx->encoder.ogg.stream, flac_options.max_lpc_order);
    OggFLAC__stream_encoder_set_min_residual_partition_order(ctx->encoder.ogg.stream, flac_options.min_residual_partition_order);
    OggFLAC__stream_encoder_set_max_residual_partition_order(ctx->encoder.ogg.stream, flac_options.max_residual_partition_order);

    OggFLAC__stream_encoder_set_blocksize(ctx->encoder.ogg.stream, flac_options.blocksize);

    OggFLAC__stream_encoder_set_client_data(ctx->encoder.ogg.stream, ctx);

    if (0 < num_metadata)
      OggFLAC__stream_encoder_set_metadata(ctx->encoder.ogg.stream, metadata, num_metadata);

    /* set callback */
    OggFLAC__stream_encoder_set_write_callback(ctx->encoder.ogg.stream, ogg_stream_encoder_write_callback);

    ctx->state.ogg = OggFLAC__stream_encoder_init(ctx->encoder.ogg.stream);
    if (ctx->state.ogg != OggFLAC__STREAM_ENCODER_OK) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot create OggFLAC state (%s)",
		OggFLAC__StreamEncoderStateString[ctx->state.ogg]);
      flac_session_close();
      return -1;
    }
  }
  else
#endif /* AU_OGGFLAC */
  if (flac_options.seekable) {
    if ((ctx->encoder.flac.s_stream = FLAC__seekable_stream_encoder_new()) == NULL) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot create FLAC stream");
      flac_session_close();
      return -1;
    }

    FLAC__seekable_stream_encoder_set_channels(ctx->encoder.flac.s_stream, nch);
    /* 16bps only */
    FLAC__seekable_stream_encoder_set_bits_per_sample(ctx->encoder.flac.s_stream, 16);

    FLAC__seekable_stream_encoder_set_verify(ctx->encoder.flac.s_stream, flac_options.verify);

    if (!FLAC__format_sample_rate_is_valid(dpm.rate)) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "invalid sampling rate %d",
		dpm.rate);
      flac_session_close();
      return -1;
    }
    FLAC__seekable_stream_encoder_set_sample_rate(ctx->encoder.flac.s_stream, dpm.rate);

    FLAC__seekable_stream_encoder_set_qlp_coeff_precision(ctx->encoder.flac.s_stream, flac_options.qlp_coeff_precision);
    /* expensive! */
    FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(ctx->encoder.flac.s_stream, flac_options.qlp_coeff_precision_search);

    if (nch == 2) {
      FLAC__seekable_stream_encoder_set_do_mid_side_stereo(ctx->encoder.flac.s_stream, flac_options.mid_side);
      FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(ctx->encoder.flac.s_stream, flac_options.adaptive_mid_side);
    }

    FLAC__seekable_stream_encoder_set_max_lpc_order(ctx->encoder.flac.s_stream, flac_options.max_lpc_order);
    FLAC__seekable_stream_encoder_set_min_residual_partition_order(ctx->encoder.flac.s_stream, flac_options.min_residual_partition_order);
    FLAC__seekable_stream_encoder_set_max_residual_partition_order(ctx->encoder.flac.s_stream, flac_options.max_residual_partition_order);

    FLAC__seekable_stream_encoder_set_blocksize(ctx->encoder.flac.s_stream, flac_options.blocksize);
    FLAC__seekable_stream_encoder_set_client_data(ctx->encoder.flac.s_stream, ctx);

    if (0 < num_metadata)
      FLAC__seekable_stream_encoder_set_metadata(ctx->encoder.flac.s_stream, metadata, num_metadata);

    /* set callback */
/*    FLAC__seekable_stream_encoder_set_metadata_callback(ctx->encoder.flac.s_stream, flac_seekable_stream_encoder_metadata_callback); /* */
#ifndef __BORLANDC__
    FLAC__stream_encoder_set_metadata_callback(ctx->encoder.flac.s_stream, flac_seekable_stream_encoder_metadata_callback); /* */
#endif
    FLAC__seekable_stream_encoder_set_write_callback(ctx->encoder.flac.s_stream, flac_seekable_stream_encoder_write_callback);

    ctx->state.s_flac = FLAC__seekable_stream_encoder_init(ctx->encoder.flac.s_stream);
    if (ctx->state.s_flac != FLAC__SEEKABLE_STREAM_ENCODER_OK) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot create FLAC state (%s)",
		FLAC__SeekableStreamEncoderStateString[ctx->state.s_flac]);
      flac_session_close();
      return -1;
    }
	}
	else
  {
    if ((ctx->encoder.flac.stream = FLAC__stream_encoder_new()) == NULL) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot create FLAC stream");
      flac_session_close();
      return -1;
    }

    FLAC__stream_encoder_set_channels(ctx->encoder.flac.stream, nch);
    /* 16bps only */
    FLAC__stream_encoder_set_bits_per_sample(ctx->encoder.flac.stream, 16);

    FLAC__stream_encoder_set_verify(ctx->encoder.flac.stream, flac_options.verify);

    if (!FLAC__format_sample_rate_is_valid(dpm.rate)) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "invalid sampling rate %d",
		dpm.rate);
      flac_session_close();
      return -1;
    }
    FLAC__stream_encoder_set_sample_rate(ctx->encoder.flac.stream, dpm.rate);

    FLAC__stream_encoder_set_qlp_coeff_precision(ctx->encoder.flac.stream, flac_options.qlp_coeff_precision);
    /* expensive! */
    FLAC__stream_encoder_set_do_qlp_coeff_prec_search(ctx->encoder.flac.stream, flac_options.qlp_coeff_precision_search);

    if (nch == 2) {
      FLAC__stream_encoder_set_do_mid_side_stereo(ctx->encoder.flac.stream, flac_options.mid_side);
      FLAC__stream_encoder_set_loose_mid_side_stereo(ctx->encoder.flac.stream, flac_options.adaptive_mid_side);
    }

    FLAC__stream_encoder_set_max_lpc_order(ctx->encoder.flac.stream, flac_options.max_lpc_order);
    FLAC__stream_encoder_set_min_residual_partition_order(ctx->encoder.flac.stream, flac_options.min_residual_partition_order);
    FLAC__stream_encoder_set_max_residual_partition_order(ctx->encoder.flac.stream, flac_options.max_residual_partition_order);

    FLAC__stream_encoder_set_blocksize(ctx->encoder.flac.stream, flac_options.blocksize);
    FLAC__stream_encoder_set_client_data(ctx->encoder.flac.stream, ctx);

    if (0 < num_metadata)
      FLAC__stream_encoder_set_metadata(ctx->encoder.flac.stream, metadata, num_metadata);

    /* set callback */
    FLAC__stream_encoder_set_metadata_callback(ctx->encoder.flac.stream, flac_stream_encoder_metadata_callback);
    FLAC__stream_encoder_set_write_callback(ctx->encoder.flac.stream, flac_stream_encoder_write_callback);

    ctx->state.flac = FLAC__stream_encoder_init(ctx->encoder.flac.stream);
    if (ctx->state.flac != FLAC__STREAM_ENCODER_OK) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot create FLAC state (%s)",
		FLAC__StreamEncoderStateString[ctx->state.flac]);
      flac_session_close();
      return -1;
    }
  }

  return 0;
}

static int auto_flac_output_open(const char *input_filename, const char *title)
{
  char *output_filename;

#ifdef AU_OGGFLAC
  if (flac_options.isogg) {
#ifndef __W32G__
  output_filename = create_auto_output_name(input_filename, "ogg", NULL, 0);
#else
  output_filename = create_auto_output_name(input_filename, "ogg", w32g_output_dir, w32g_auto_output_mode);
#endif
  }
  else
#endif /* AU_OGGFLAC */
  {
#ifndef __W32G__
    output_filename = create_auto_output_name(input_filename, "flac", NULL, 0);
#else
    output_filename = create_auto_output_name(input_filename, "flac", w32g_output_dir, w32g_auto_output_mode);
#endif
  }
  if (output_filename == NULL) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "unknown output file name");
	  return -1;
  }
  if ((flac_output_open(output_filename, input_filename)) == -1) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "files open failed %s->%s", output_filename, input_filename);
    free(output_filename);
    return -1;
  }
  if (dpm.name != NULL)
    free(dpm.name);
  dpm.name = output_filename;
  ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Output %s", dpm.name);
  return 0;
}

static int open_output(void)
{
  int include_enc, exclude_enc;  

#ifdef AU_FLAC_DLL
	if (g_load_libFLAC_dll("libFLAC.dll")) {
		return -1;
	}
#endif
#ifdef AU_OGGFLAC_DLL
	if (g_load_libOggFLAC_dll("libOggFLAC.dll")) {
#ifdef AU_FLAC_DLL
		g_free_libFLAC_dll ();
#endif
		return -1;
	}
#endif

  include_enc = exclude_enc = 0;

  /* only 16 bit is supported */
  include_enc |= PE_16BIT | PE_SIGNED;
  exclude_enc |= PE_BYTESWAP | PE_24BIT;
  dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

#ifdef AU_OGGFLAC
  if (flac_options.isogg) {
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "*** cannot write back seekpoints when encoding to Ogg yet ***");
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "*** and stream end will not be written.                   ***");
  }
#endif

#ifndef __W32G__
  if(dpm.name == NULL) {
    dpm.flag |= PF_AUTO_SPLIT_FILE;
    dpm.name = NULL;
  } else {
    dpm.flag &= ~PF_AUTO_SPLIT_FILE;
    if(flac_output_open(dpm.name, NULL) == -1)
      return -1;
  }
#else
  if(w32g_auto_output_mode > 0) {
    dpm.flag |= PF_AUTO_SPLIT_FILE;
    dpm.name = NULL;
    } else {
    dpm.flag &= ~PF_AUTO_SPLIT_FILE;
    if (flac_output_open(dpm.name, NULL) == -1)
      return -1;
  }
#endif

  return 0;
}

#ifdef AU_OGGFLAC
static FLAC__StreamEncoderWriteStatus
ogg_stream_encoder_write_callback(const OggFLAC__StreamEncoder *encoder,
				  const FLAC__byte buffer[],
				  unsigned bytes, unsigned samples,
				  unsigned current_frame, void *client_data)
{
  FLAC_ctx *ctx = (FLAC_ctx *)client_data;

  ctx->out_bytes += bytes;

  if (write(dpm.fd, buffer, bytes) != -1)
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  else
    return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}
#endif
static FLAC__StreamEncoderWriteStatus
flac_stream_encoder_write_callback(const FLAC__StreamEncoder *encoder,
				   const FLAC__byte buffer[],
				   unsigned bytes, unsigned samples,
				   unsigned current_frame, void *client_data)
{
  FLAC_ctx *ctx = (FLAC_ctx *)client_data;

  ctx->out_bytes += bytes;

  if (write(dpm.fd, buffer, bytes) == bytes)
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  else
    return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}
static void flac_stream_encoder_metadata_callback(const FLAC__StreamEncoder *encoder,
						  const FLAC__StreamMetadata *metadata,
						  void *client_data)
{
}
static FLAC__StreamEncoderWriteStatus
flac_seekable_stream_encoder_write_callback(const FLAC__SeekableStreamEncoder *encoder,
				   const FLAC__byte buffer[],
				   unsigned bytes, unsigned samples,
				   unsigned current_frame, void *client_data)
{
  FLAC_ctx *ctx = (FLAC_ctx *)client_data;

  ctx->out_bytes += bytes;

  if (write(dpm.fd, buffer, bytes) == bytes)
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  else
    return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}
static void flac_seekable_stream_encoder_metadata_callback(const FLAC__SeekableStreamEncoder *encoder,
						  const FLAC__StreamMetadata *metadata,
						  void *client_data)
{
}

static int output_data(char *buf, int32 nbytes)
{
  FLAC__int32 *oggbuf;
  FLAC__int16 *s;
  int i;
  int nch = (dpm.encoding & PE_MONO) ? 1 : 2;

  FLAC_ctx *ctx = flac_ctx;

  if (dpm.fd < 0)
    return 0;

  if (ctx == NULL) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "FLAC stream is not initialized");
    return -1;
  }

  oggbuf = (FLAC__int32 *)safe_malloc(nbytes * sizeof(FLAC__int32) / nch);

  /*
    packing 16 -> 32 bit sample
  */
  s = (FLAC__int16 *)buf;
  for (i = 0; i < nbytes / nch; i++) {
    oggbuf[i] = *s++;
  }

#ifdef AU_OGGFLAC
  if (flac_options.isogg) {
    ctx->state.ogg = OggFLAC__stream_encoder_get_state(ctx->encoder.ogg.stream);
    if (ctx->state.ogg != OggFLAC__STREAM_ENCODER_OK) {
      if (ctx->state.ogg == OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR) {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "FLAC stream verify error (%s)",
		  FLAC__StreamDecoderStateString[OggFLAC__stream_encoder_get_verify_decoder_state(ctx->encoder.ogg.stream)]);
      }
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot encode OggFLAC stream (%s)",
		OggFLAC__StreamEncoderStateString[ctx->state.ogg]);
      flac_session_close();
      return -1;
    }

    if (!OggFLAC__stream_encoder_process_interleaved(ctx->encoder.ogg.stream, oggbuf,
						     nbytes / nch / 2)) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot encode OggFLAC stream");
      flac_session_close();
      return -1;
    }
  }
  else
#endif /* AU_OGGFLAC */
	if (flac_options.seekable) {
    ctx->state.s_flac = FLAC__seekable_stream_encoder_get_state(ctx->encoder.flac.s_stream);
    if (ctx->state.s_flac != FLAC__STREAM_ENCODER_OK) {
      if (ctx->state.s_flac == FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR |
	  FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA) {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "FLAC stream verify error (%s)",
		  FLAC__SeekableStreamDecoderStateString[FLAC__seekable_stream_encoder_get_verify_decoder_state(ctx->encoder.flac.s_stream)]);
      }
      else {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot encode FLAC stream (%s)",
		  FLAC__SeekableStreamEncoderStateString[ctx->state.s_flac]);
      }
      flac_session_close();
      return -1;
    }

    if (!FLAC__seekable_stream_encoder_process_interleaved(ctx->encoder.flac.s_stream, oggbuf,
						  nbytes / nch / 2 )) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot encode FLAC stream");
      flac_session_close();
      return -1;
    }
	}
  else
	{
    ctx->state.flac = FLAC__stream_encoder_get_state(ctx->encoder.flac.stream);
    if (ctx->state.flac != FLAC__STREAM_ENCODER_OK) {
      if (ctx->state.flac == FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR |
	  FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA) {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "FLAC stream verify error (%s)",
		  FLAC__StreamDecoderStateString[FLAC__stream_encoder_get_verify_decoder_state(ctx->encoder.flac.stream)]);
      }
      else {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot encode FLAC stream (%s)",
		  FLAC__StreamEncoderStateString[ctx->state.flac]);
      }
      flac_session_close();
      return -1;
    }

    if (!FLAC__stream_encoder_process_interleaved(ctx->encoder.flac.stream, oggbuf,
						  nbytes / nch / 2 )) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "cannot encode FLAC stream");
      flac_session_close();
      return -1;
    }
  }
  ctx->in_bytes += nbytes;

  free(oggbuf);
  return 0;
}

static void close_output(void)
{
  FLAC_ctx *ctx;

  ctx = flac_ctx;

  if (ctx == NULL)
    return;

  if (dpm.fd < 0) {
    flac_session_close();
    return;
  }

  if (flac_options.isogg) {
#ifdef AU_OGGFLAC
    if ((ctx->state.ogg = OggFLAC__stream_encoder_get_state(ctx->encoder.ogg.stream)) != OggFLAC__STREAM_ENCODER_OK) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "OggFLAC stream encoder is invalid (%s)",
		OggFLAC__StreamEncoderStateString[ctx->state.ogg]);
      /* fall through */
    }
  }
  else
#endif /* AU_OGGFLAC */
  if (flac_options.seekable) {
    if ((ctx->state.s_flac = FLAC__seekable_stream_encoder_get_state(ctx->encoder.flac.s_stream)) != FLAC__SEEKABLE_STREAM_ENCODER_OK) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "FLAC stream encoder is invalid (%s)",
		FLAC__SeekableStreamEncoderStateString[ctx->state.s_flac]);
      /* fall through */
    }
	}
	else
  {
    if ((ctx->state.flac = FLAC__stream_encoder_get_state(ctx->encoder.flac.stream)) != FLAC__STREAM_ENCODER_OK) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "FLAC stream encoder is invalid (%s)",
		FLAC__StreamEncoderStateString[ctx->state.flac]);
      /* fall through */
    }
  }

  ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Wrote %lu/%lu bytes(%g%% compressed)",
            ctx->out_bytes, ctx->in_bytes, ((double)ctx->out_bytes / (double)ctx->in_bytes) * 100.);

  flac_session_close();

#ifdef AU_FLAC_DLL
	g_free_libFLAC_dll ();
#endif
#ifdef AU_OGGFLAC_DLL
	g_free_libOggFLAC_dll ();
#endif

}

static int acntl(int request, void *arg)
{
  switch(request) {
  case PM_REQ_PLAY_START:
    if(dpm.flag & PF_AUTO_SPLIT_FILE)
      return auto_flac_output_open(current_file_info->filename, current_file_info->seq_name);
    break;
  case PM_REQ_PLAY_END:
    if(dpm.flag & PF_AUTO_SPLIT_FILE) {
      close_output();
      return 0;
    }
    break;
  case PM_REQ_DISCARD:
    return 0;
  }
  return -1;
}
