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

    speex_a.c
	Written by Iwata <b6330015@kit.jp>
    Functions to output Ogg Speex (*.ogg).
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <speex.h>
#include <speex_header.h>
#include <ogg/ogg.h>

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

#define dpm speex_play_mode

PlayMode dpm = {
  DEFAULT_RATE, PE_SIGNED|PE_16BIT, PF_PCM_STREAM,
  -1,
  {0},
  "Ogg Speex", 'S',
  NULL,
  open_output,
  close_output,
  output_data,
  acntl
};


#define MAX_FRAME_SIZE 2000
#define MAX_FRAME_BYTES 2000

typedef struct {
  /* Ogg */
  ogg_stream_state os;
  ogg_page         og;
  ogg_packet       op;
  int              ogg_packetid;

  /* Speex */
  SpeexHeader      header;
  SpeexBits        bits;
  SpeexMode*       mode;
  void*            state;
  int              frame_size;
  int              nframes;
  int              channels;

  /* state */
  unsigned long    in_bytes;
  unsigned long    out_bytes;

  /* simple buffer */
  float            *input;
  int              input_idx;
} Speex_ctx;

static int ogg_seqnum = 0;

static Speex_ctx *speex_ctx = NULL;

typedef struct {
  int quality;
  int vbr;
  int abr;
  int vad;
  int dtx;
  int complexity;
  int nframes;
} Speex_options;

Speex_options speex_options = {
  8, /* quality */
  0, /* vbr */
  0, /* abr */
  0, /* vad */
  0, /* dtx */
  3, /* complexity */
  1  /* nframes */
};


/*
  code from speexenc.c
 */
static int speex_mode_preset()
{
  int rate = dpm.rate;
  Speex_ctx *ctx = speex_ctx;

  if (speex_ctx == NULL)
    return 0;

  if (ctx->mode == NULL) {
    if (rate > 48000) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"sampling rate too high: %d Hz,\n"
		"try down-sampling\n", rate);
      return 0;
    }
    else if (rate > 25000) {
      ctx->mode = &speex_uwb_mode;
    }
    else if (rate > 12500) {
      ctx->mode = &speex_wb_mode;
    }
    else if (rate >= 6000) {
      ctx->mode = &speex_nb_mode;
    }
    else {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"Sampling rate too low: %d Hz\n", rate);
      return 0;
    }
  }
 
  if (rate != 8000 && rate != 16000 && rate != 32000)
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
	      "Speex is only optimized for 8, 16 and 32 kHz.\n"
	      "It will still work at %d Hz but your mileage may vary", rate);

  return 1;
}

void speex_set_option_quality(int quality)
{
  speex_options.quality = quality;
  if (quality < 0)
    speex_options.quality = 0;
  if (10 < quality)
    speex_options.quality = 10;
}
void speex_set_option_vbr(int vbr)
{
  speex_options.vbr = vbr;
}
void speex_set_option_abr(int rate)
{
  speex_options.abr = rate;
}
void speex_set_option_vad(int vad)
{
  speex_options.vad = vad;
}
void speex_set_option_dtx(int dtx)
{
  speex_options.dtx = dtx;
}
void speex_set_option_complexity(int complexity)
{
  speex_options.complexity = complexity;
  if (complexity < 0)
    speex_options.complexity = 0;
  if (10 < complexity)
      speex_options.complexity = 10;
}
void speex_set_option_nframes(int nframes)
{
  speex_options.nframes = nframes;
  if (nframes < 1)
    speex_options.nframes = 0;
  if (10 < nframes)
    speex_options.nframes = 10;
}

/*
  Write an Ogg page to a file pointer
  code from speexenc.c
*/
int oe_write_page(ogg_page *page, int fd)
{
  int written = 0;
  written += write(fd, page->header, page->header_len);
  written += write(fd, page->body,   page->body_len);

  return written;
}

#define writeint(buf, base, val) do { buf[base+3]=((val)>>24)&0xff; \
                                      buf[base+2]=((val)>>16)&0xff; \
                                      buf[base+1]=((val)>>8)&0xff; \
                                      buf[base]=(val)&0xff; \
                                 } while(0)
void comment_init(char **comments, int* length, char *vendor_string)
{
  int vendor_length = strlen(vendor_string);
  int user_comment_list_length = 0;
  int len= 4 + vendor_length + 4;
  char *p = (char*)safe_malloc(len);

  writeint(p, 0, vendor_length);
  memcpy(p + 4, vendor_string, vendor_length);
  writeint(p, 4 + vendor_length, user_comment_list_length);
  *length = len;
  *comments = p;
}
/*
  Write header (format will change)
  code from speexenc.c
*/
int write_ogg_header(Speex_ctx *ctx, int fd, char *comments)
{
  int ret, result;
  char *vendor_string = "Encoded with Timidity++-" VERSION "(compiled " __DATE__ ")";
  int comments_length = strlen(comments);

  comment_init(&comments, &comments_length, vendor_string);

  ctx->op.packet = (unsigned char *)speex_header_to_packet(&ctx->header, (int*)&(ctx->op.bytes));
  ctx->op.b_o_s = 1;
  ctx->op.e_o_s = 0;
  ctx->op.granulepos = 0;
  ctx->op.packetno = 0;
  ogg_stream_packetin(&ctx->os, &ctx->op);
  free(ctx->op.packet);

  ctx->op.packet = (unsigned char *)comments;
  ctx->op.bytes = comments_length;

  ctx->op.b_o_s = 0;
  ctx->op.e_o_s = 0;
  ctx->op.granulepos = 0;
  ctx->op.packetno = 1;
  ogg_stream_packetin(&ctx->os, &ctx->op);

  while((result = ogg_stream_flush(&ctx->os, &ctx->og))) {
    if(!result) break;
    ret = oe_write_page(&ctx->og, fd);
    if(ret != ctx->og.header_len + ctx->og.body_len) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "failed writing header to output Ogg stream\n");
      return 1;
    }
    else
      ctx->out_bytes += ret;
  }
  return 0;
}

static int speex_output_open(const char *fname, const char *comment)
{
  Speex_ctx *ctx;
  int fd;

  if (!(speex_ctx = (Speex_ctx *)calloc(sizeof(Speex_ctx), 1))) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s", strerror(errno));
    return -1;
  }
  ctx = speex_ctx;

  /* init id */
  ctx->in_bytes = ctx->out_bytes = 0;
  ctx->ogg_packetid = -1;
  ctx->channels = (dpm.encoding & PE_MONO) ? 1 : 2;

  if(strcmp(fname, "-") == 0) {
    fd = 1; /* data to stdout */
    if(comment == NULL)
      comment = "(stdout)";
  } else {
    /* Open the audio file */
    fd = open(fname, FILE_OUTPUT_MODE);
    if (fd < 0) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
                fname, strerror(errno));
      return -1;
    }
  }

  /*Initialize Ogg stream struct*/
  if (ogg_seqnum == 0) {
    srand(time(NULL));
    ogg_seqnum = rand();
  }
  if (ogg_stream_init(&ctx->os, ogg_seqnum++) == -1) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Ogg stream init failed\n");
    return -1;
  }

  if (!speex_mode_preset())
    return -1;

  speex_init_header(&ctx->header, dpm.rate, 1, ctx->mode);
  ctx->header.nb_channels = ctx->channels;

  ctx->header.frames_per_packet = ctx->nframes = speex_options.nframes;

  ctx->state = speex_encoder_init(ctx->mode);

  /*Set the quality to 8 (15 kbps)*/
  speex_encoder_ctl(ctx->state, SPEEX_SET_QUALITY, &speex_options.quality);

  if(strcmp(fname, "-") == 0) {
    fd = 1; /* data to stdout */
    if(comment == NULL)
      comment = "(stdout)";
  } else {
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

  write_ogg_header(ctx, fd, (char *)comment);

  speex_encoder_ctl(ctx->state, SPEEX_GET_FRAME_SIZE, &ctx->frame_size);
  speex_encoder_ctl(ctx->state, SPEEX_SET_COMPLEXITY, &speex_options.complexity);
  speex_encoder_ctl(ctx->state, SPEEX_SET_SAMPLING_RATE, &dpm.rate);

  if (speex_options.vbr) {
    speex_encoder_ctl(ctx->state, SPEEX_SET_VBR, &speex_options.vbr);
  }
  else if (speex_options.vad) {
    speex_encoder_ctl(ctx->state, SPEEX_SET_VAD, &speex_options.vad);
  }
  if (speex_options.dtx)
    speex_encoder_ctl(ctx->state, SPEEX_SET_DTX, &speex_options.dtx);
  if (speex_options.dtx && !(speex_options.vbr ||
			     speex_options.abr ||
			     speex_options.vad)) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "--speex-dtx is useless without --speex-vad, --speex-vbr or --speex-abr");
  }
  else if ((speex_options.vbr || speex_options.abr) && (speex_options.vad)) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "--speex-vad is already implied by --speex-vbr or --speex-abr");
  }
  if (speex_options.abr) {
    speex_encoder_ctl(ctx->state, SPEEX_SET_ABR, &speex_options.abr);
  }

  speex_bits_init(&ctx->bits);

  ctx->input = (float *)safe_malloc(sizeof(float) * MAX_FRAME_SIZE * ctx->channels);
  ctx->input_idx = 0;

  return fd;
}

/*
  code from vorbis_a.c
 */
extern char *create_auto_output_name(const char *input_filename, char *ext_str, char *output_dir, int mode);

static int auto_speex_output_open(const char *input_filename, const char *title)
{
  char *output_filename;

#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
  output_filename = create_auto_output_name(input_filename, "ogg", NULL, 0);
#else
  output_filename = create_auto_output_name(input_filename, "ogg", w32g_output_dir, w32g_auto_output_mode);
#endif
  if (output_filename == NULL){
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "unknown output file name");
    return -1;
  }
  if ((dpm.fd = speex_output_open(output_filename, input_filename)) == -1) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "files open failed %s->%s",
	      output_filename, input_filename);
    free(output_filename);
    return -1;
  }
  if (dpm.name != NULL)
    free(dpm.name);
  dpm.name = output_filename;

  ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Output: %s", dpm.name);
  {
    char *st_string="mono";
    if (speex_ctx->channels == 2)
      st_string = "stereo";
    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Encoding %d Hz audio using %s mode (%s)",
	      speex_ctx->header.rate, speex_ctx->mode->modeName, st_string);
   }

  return 0;
}

static int open_output(void)
{
  int include_enc, exclude_enc;

  include_enc = exclude_enc = 0;

  /* only 16 bit is supported */
  include_enc |= PE_16BIT|PE_SIGNED;
  exclude_enc |= PE_BYTESWAP|PE_24BIT;
  dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

#if !defined (IA_W32GUI) && !defined (IA_W32G_SYN)
  if (dpm.name == NULL) {
    dpm.flag |= PF_AUTO_SPLIT_FILE;
    dpm.name = NULL;
  }
  else {
    dpm.flag &= ~PF_AUTO_SPLIT_FILE;
    if ((dpm.fd = speex_output_open(dpm.name, NULL)) == -1)
      return -1;
  }
#else
  if (w32g_auto_output_mode > 0){
    dpm.flag |= PF_AUTO_SPLIT_FILE;
    dpm.name = NULL;
  }
  else {
    dpm.flag &= ~PF_AUTO_SPLIT_FILE;
    if ((dpm.fd = speex_output_open(dpm.name, NULL)) == -1)
      return -1;
  }
#endif
  return 0;
}

static int output_data(char *buf, int32 nbytes)
{
  char cbits[MAX_FRAME_BYTES];
  Speex_ctx *ctx = speex_ctx;
  int nbBytes;
  int16 *s;
  int i, j;
  int ret;
  int nbytes_left;

  if (dpm.fd < 0)
    return 0;

  ctx->in_bytes += nbytes;

  /*
    Main encoding loop (one frame per iteration)
  */
  nbytes_left = nbytes;
  s = (int16 *)buf;
  while (1) {
    ctx->ogg_packetid++;
    /*
      packing 16 bit -> float sample
    */
    for (i = ctx->input_idx; i < ctx->frame_size * ctx->channels; i++) {
      /* stream is ended, and buffer is not full. wait next spooling */
      if (nbytes_left < 0) {
	/* canceling ogg packet */
	ctx->ogg_packetid--;
	return 0;
      }
      ctx->input[i] = *s++;
      nbytes_left -= 2; /* -16 bit*/
      ctx->input_idx++;
    }
    /*
      buffer is full. encode now.
    */
    ctx->input_idx = 0;

    if (ctx->channels == 2)
      speex_encode_stereo(ctx->input, ctx->frame_size, &ctx->bits);
    /* Encode the frame */
    speex_encode(ctx->state, ctx->input, &ctx->bits);

    if ((ctx->ogg_packetid + 1) % ctx->nframes != 0)
      continue;

    speex_bits_insert_terminator(&ctx->bits);
    /* Copy the bits to an array of char that can be written */
    nbBytes = speex_bits_write(&ctx->bits, cbits, MAX_FRAME_BYTES);
    
    /* Flush all the bits in the struct so we can encode a new frame */
    speex_bits_reset(&ctx->bits);

    /* ogg packet setup */
    ctx->op.packet = (unsigned char *)cbits;
    ctx->op.bytes = nbBytes;
    ctx->op.b_o_s = 0;
    ctx->op.e_o_s = 0;

    ctx->op.granulepos = (ctx->ogg_packetid + ctx->nframes) * ctx->frame_size;
    ctx->op.packetno = 2 + ctx->ogg_packetid / ctx->nframes;
    ogg_stream_packetin(&ctx->os, &ctx->op);

    /* Write all new pages (most likely 0 or 1) */
    while (ogg_stream_pageout(&ctx->os, &ctx->og)) {
      ret = oe_write_page(&ctx->og, dpm.fd);
      if (ret != ctx->og.header_len + ctx->og.body_len) {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "failed writing header to output stream");
	return -1;
      }
      else
	ctx->out_bytes += ret;
    }
  }
  
  return 0;
}

static void close_output(void)
{
  int i;
  char cbits[MAX_FRAME_BYTES];
  Speex_ctx *ctx = speex_ctx;
  int nbBytes;
  int ret;

  if (ctx == NULL)
    return;

  if (dpm.fd < 0)
    return;

  /*
    Write last frame
  */
  if (speex_ctx != NULL) {
    if ((ctx->ogg_packetid + 1) % ctx->nframes != 0) {
      while ((ctx->ogg_packetid + 1) % ctx->nframes != 0) {
	ctx->ogg_packetid++;
	speex_bits_pack(&ctx->bits, 15, 5);
      }
      nbBytes = speex_bits_write(&ctx->bits, cbits, MAX_FRAME_BYTES);
      ctx->op.packet = (unsigned char *)cbits;
      ctx->op.bytes = nbBytes;
      ctx->op.b_o_s = 0;
      ctx->op.e_o_s = 1;
      ctx->op.granulepos = (ctx->ogg_packetid + ctx->nframes) * ctx->frame_size;
      ctx->op.packetno = 2 + ctx->ogg_packetid / ctx->nframes;
      ogg_stream_packetin(&ctx->os, &ctx->op);
    }
    for (i = ctx->input_idx; i < ctx->frame_size * ctx->channels; i++) {
      /* left is zero-cleaned */
      ctx->input[i] = 0;
    }
    if (ctx->channels == 2)
      speex_encode_stereo(ctx->input, ctx->frame_size, &ctx->bits);
    /* Encode the frame */
    speex_encode(ctx->state, ctx->input, &ctx->bits);
    speex_bits_insert_terminator(&ctx->bits);
    /* Copy the bits to an array of char that can be written */
    nbBytes = speex_bits_write(&ctx->bits, cbits, MAX_FRAME_BYTES);
    
    /* Flush all the bits in the struct so we can encode a new frame */
    speex_bits_reset(&ctx->bits);

    /* ogg packet setup */
    ctx->op.packet = (unsigned char *)cbits;
    ctx->op.bytes = nbBytes;
    ctx->op.b_o_s = 0;
    ctx->op.e_o_s = 1;

    ctx->op.granulepos = (ctx->ogg_packetid + ctx->nframes) * ctx->frame_size;
    ctx->op.packetno = 2 + ctx->ogg_packetid / ctx->nframes;
    ogg_stream_packetin(&ctx->os, &ctx->op);

    /* Write all new pages (most likely 0 or 1) */
    while (ogg_stream_pageout(&ctx->os, &ctx->og)) {
      ret = oe_write_page(&ctx->og, dpm.fd);
      if (ret != ctx->og.header_len + ctx->og.body_len) {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "failed writing header to output stream");
	return;
      }
      else
	ctx->out_bytes += ret;
    }

    ogg_stream_clear(&speex_ctx->os);
    speex_bits_destroy(&speex_ctx->bits);
    speex_encoder_destroy(speex_ctx->state);
    close(dpm.fd);
    dpm.fd = -1;
    free(speex_ctx->input);

    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Wrote %lu/%lu bytes(%g%% compressed)",
	      ctx->out_bytes, ctx->in_bytes, ((double)ctx->out_bytes / (double)ctx->in_bytes)) * 100.;


    speex_ctx->input = NULL;
    free(speex_ctx);
    speex_ctx = NULL;
  }
  return;
}

static int acntl(int request, void *arg)
{
  switch(request) {
  case PM_REQ_PLAY_START:
    if(dpm.flag & PF_AUTO_SPLIT_FILE)
      return auto_speex_output_open(current_file_info->filename,current_file_info->seq_name);
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
