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

    vorbis_a.c

    Functions to output Ogg Vorbis (*.ogg).
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"
#include <stdio.h>
#include <string.h>

#ifdef AU_VORBIS

#ifdef AU_VORBIS_DLL
#include <stdlib.h>
#include <io.h>
#include <ctype.h>
extern int load_ogg_dll(void);
extern void free_ogg_dll(void);
extern int load_vorbis_dll(void);
extern void free_vorbis_dll(void);
extern int load_vorbisenc_dll(void);
extern void free_vorbisenc_dll(void);
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef __W32__
#include <windows.h>
#include <winnls.h>
#endif

#include <vorbis/vorbisenc.h>

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);

/* export the playback mode */
#define dpm vorbis_play_mode

PlayMode dpm = {
    44100, PE_16BIT|PE_SIGNED, PF_PCM_STREAM,
    -1,
    {0,0,0,0,0},
    "Ogg Vorbis", 'v',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl
};
static char *tag_title = NULL;

static	ogg_stream_state os; /* take physical pages, weld into a logical
				stream of packets */
static	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
static	vorbis_block	 vb; /* local working space for packet->PCM decode */
static	vorbis_info	 vi; /* struct that stores all the static vorbis bitstream
				settings */
static	vorbis_comment	 vc; /* struct that stores all the user comments */

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
extern char *w32g_output_dir;
extern int w32g_auto_output_mode;
extern int vorbis_ConfigDialogInfoApply(void);
int ogg_vorbis_mode = 8;	/* initial mode. */
#endif

/*************************************************************************/

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
static int
choose_bitrate(int nch, int rate)
{
  int bitrate;

#if 0
  /* choose an encoding mode */
  /* (mode 0: -> mode2 */
  /* (mode 1: 44kHz stereo uncoupled, N/A\n */
  /* (mode 2: 44kHz stereo uncoupled, roughly 128kbps VBR) */
  /* (mode 3: 44kHz stereo uncoupled, roughly 160kbps VBR) */
  /* (mode 4: 44kHz stereo uncoupled, roughly 192kbps VBR) */
  /* (mode 5: 44kHz stereo uncoupled, roughly 256kbps VBR) */
  /* (mode 6: 44kHz stereo uncoupled, roughly 350kbps VBR) */

  switch (ogg_vorbis_mode) {
  case 0:
    bitrate = 128 * 1000; break;
  case 1:
    bitrate = 112 * 1000; break;
  case 2:
    bitrate = 128 * 1000; break;
  case 3:
    bitrate = 160 * 1000; break;
  case 4:
    bitrate = 192 * 1000; break;
  case 5:
    bitrate = 256 * 1000; break;
  case 6:
    bitrate = 350 * 1000; break;
  default:
    bitrate = 160 * 1000; break;
  }
  return bitrate;
#else
	if (ogg_vorbis_mode < 1 || ogg_vorbis_mode > 1000)
		bitrate = 8;
	else
		bitrate = ogg_vorbis_mode;
	return bitrate;
#endif
  return (int)(nch * rate * (128000.0 / (2.0 * 44100.0)) + 0.5); /* +0.5 for rounding */
}
#else
static int
choose_bitrate(int nch, int rate)
{
  int target;

  /* 44.1kHz 2ch --> 128kbps */
  target = (int)(nch * rate * (128000.0 / (2.0 * 44100.0)) + 0.5); /* +0.5 for rounding */

  return target;
}
#endif

#ifdef __W32__
static char *w32_mbs_to_utf8(const char* str)
{
	int str_size = strlen(str);
	int buff16_size = str_size;
	wchar_t* buff16;
	int buff8_size = 0;
	char* buff8;
	char* buff8_p;
	int i;
	if ( str_size == 0 ) {
		return strdup ( str );
	}
	buff16 = (wchar_t*) malloc (sizeof(wchar_t)*buff16_size + 1);
	if ( buff16 == NULL ) return NULL;
	buff16_size = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, str, str_size, buff16, buff16_size ) ;
	if ( buff16_size == 0 ) {
		free ( buff16 );
		return NULL;
	}
	for ( i = 0; i < buff16_size; ++i ) {
		wchar_t w = buff16[i];
		if ( w < 0x0080 ) buff8_size += 1;
		else if ( w < 0x0800 ) buff8_size += 2;
		else buff8_size += 3;
	}
	buff8 = (char*) malloc ( sizeof(char)*buff8_size + 1 );
	if ( buff8 == NULL ) {
		free ( buff16 );
		return NULL;
	}
	for ( i = 0, buff8_p = buff8; i < buff16_size; ++i ) {
		wchar_t w = buff16[i];
		if ( w < 0x0080 ) {
			*(buff8_p++) = (char)w;
		} else if ( buff16[i] < 0x0800 ) {
			*(buff8_p++) = 0xc0 | (w >> 6);
			*(buff8_p++) = 0x80 | (w & 0x3f);
		} else {
			*(buff8_p++) = 0xe0 | (w >> 12);
			*(buff8_p++) = 0x80 | ((w >>6 ) & 0x3f);
			*(buff8_p++) = 0x80 | (w & 0x3f);
		}
	}
	*buff8_p = '\0';
	free ( buff16 );
	return buff8;
}
#endif

static int ogg_output_open(const char *fname, const char *comment)
{
  int fd;
  int nch;
#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
  int bitrate;
#endif

#ifdef AU_VORBIS_DLL
  {
	  int flag = 0;
		if(!load_ogg_dll())
			if(!load_vorbis_dll())
				if(!load_vorbisenc_dll())
					flag = 1;
		if(!flag){
			free_ogg_dll();
			free_vorbis_dll();
			free_vorbisenc_dll();
			return -1;
		}
  }
#endif

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

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
  vorbis_ConfigDialogInfoApply();
#endif

  nch = (dpm.encoding & PE_MONO) ? 1 : 2;

  /* choose an encoding mode */
  vorbis_info_init(&vi);
#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
  bitrate = choose_bitrate(nch, dpm.rate);
  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Target encoding bitrate: %dbps", bitrate);
  vorbis_encode_init(&vi, nch, dpm.rate, -1, bitrate, -1);
#else
  {
	  float bitrate_f = (float)choose_bitrate(nch, dpm.rate);
	  if (bitrate_f <= 10.0 )
		  bitrate_f /= 10.0;
	  if (bitrate_f > 10 )
		  bitrate_f /= 1000.0;
	  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Target encoding VBR quality: %d", bitrate_f);
	  vorbis_encode_init_vbr(&vi, nch, dpm.rate, bitrate_f);
  }
#endif

  {
    /* add a comment */
    char *location_string;

    vorbis_comment_init(&vc);

    location_string =
      (char *)safe_malloc(strlen(comment) + sizeof("LOCATION=") + 2);
    strcpy(location_string, "LOCATION=");
    strcat(location_string, comment);
#ifndef __W32__
    vorbis_comment_add(&vc, (char *)location_string);
    free(location_string);
#else
		{
			char* location_string_utf8 = w32_mbs_to_utf8 ( location_string );
			if ( location_string_utf8 == NULL ) {
		    vorbis_comment_add(&vc, (char *)location_string);
			} else {
		    vorbis_comment_add(&vc, (char *)location_string_utf8);
				if ( location_string_utf8 != location_string )
					free ( location_string_utf8 );
			}
			free(location_string);
		}
#endif
  }
  /* add default tag */
    if (tag_title != NULL) {
#ifndef __W32__
	vorbis_comment_add_tag(&vc, "title", (char *)tag_title);
#else
		{
			char* tag_title_utf8 = w32_mbs_to_utf8 ( tag_title );
			if ( tag_title_utf8 == NULL ) {
				vorbis_comment_add_tag(&vc, "title", (char *)tag_title);
			} else {
				vorbis_comment_add_tag(&vc, "title", (char *)tag_title_utf8);
				if ( tag_title_utf8 != tag_title )
					free ( tag_title_utf8 );
			}
		}
#endif
  }

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init(&vd, &vi);
  vorbis_block_init(&vd, &vb);

  /* set up our packet->stream encoder */
  /* pick a random serial number; that way we can more likely build
     chained streams just by concatenation */
  srand(time(NULL));
  ogg_stream_init(&os, rand());

  /* Vorbis streams begin with three headers; the initial header (with
     most of the codec setup parameters) which is mandated by the Ogg
     bitstream spec.  The second header holds any comment fields.  The
     third header holds the bitstream codebook.  We merely need to
     make the headers, then pass them to libvorbis one at a time;
     libvorbis handles the additional Ogg bitstream constraints */

  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
    ogg_stream_packetin(&os, &header); /* automatically placed in its own
					  page */
    ogg_stream_packetin(&os, &header_comm);
    ogg_stream_packetin(&os, &header_code);

    /* no need to write out here.  We'll get to that in the main loop */
  }

  return fd;
}

/* mode
  0,1: Default mode.
  2: Remove the directory path of input_filename, then add output_dir.
  3: Replace directory separator characters ('/','\',':') with '_', then add output_dir.
 */
extern char *create_auto_output_name(const char *input_filename, char *ext_str, char *output_dir, int mode);

static int auto_ogg_output_open(const char *input_filename, const char *title)
{
  char *output_filename;

#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
  output_filename = create_auto_output_name(input_filename,"ogg",NULL,0);
#else
  output_filename = create_auto_output_name(input_filename,"ogg",w32g_output_dir,w32g_auto_output_mode);
#endif
  if(output_filename==NULL){
	  return -1;
  }
  if (tag_title != NULL) {
	free(tag_title);
	tag_title = NULL;
  }
  if (title != NULL) {
	tag_title = (char *)safe_malloc(sizeof(char)*(strlen(title)+1));
	strcpy(tag_title, title);
  }
  if((dpm.fd = ogg_output_open(output_filename, input_filename)) == -1) {
    free(output_filename);
    return -1;
  }
  if(dpm.name != NULL)
    free(dpm.name);
  dpm.name = output_filename;
  ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Output %s", dpm.name);
  return 0;
}

static int open_output(void)
{
  int include_enc, exclude_enc;

  /********** Encode setup ************/

  include_enc = exclude_enc = 0;

  /* only 16 bit is supported */
  include_enc |= PE_16BIT|PE_SIGNED;
  exclude_enc |= PE_BYTESWAP|PE_24BIT;
  dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
  if(dpm.name == NULL) {
    dpm.flag |= PF_AUTO_SPLIT_FILE;
    dpm.name = NULL;
  } else {
    dpm.flag &= ~PF_AUTO_SPLIT_FILE;
    if((dpm.fd = ogg_output_open(dpm.name, NULL)) == -1)
      return -1;
  }
#else
	if(w32g_auto_output_mode>0){
      dpm.flag |= PF_AUTO_SPLIT_FILE;
      dpm.name = NULL;
    } else {
      dpm.flag &= ~PF_AUTO_SPLIT_FILE;
      if((dpm.fd = ogg_output_open(dpm.name,NULL)) == -1)
		return -1;
    }
#endif

  return 0;
}

static int output_data(char *readbuffer, int32 bytes)
{
  int i, j, ch = ((dpm.encoding & PE_MONO) ? 1 : 2);
  float **buffer;
  int16 *samples = (int16 *)readbuffer;
  int nsamples = bytes / (2 * ch);
  ogg_page   og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet op; /* one raw packet of data for decode */

  if (dpm.fd<0)
    return 0;

  /* data to encode */

  /* expose the buffer to submit data */
  buffer = vorbis_analysis_buffer(&vd, nsamples);
      
  /* uninterleave samples */
  for(j = 0; j < ch; j++)
    for(i = 0; i < nsamples; i++)
      buffer[j][i] = (float)(samples[i*ch+j] * (1.0/32768.0));

  /* tell the library how much we actually submitted */
  vorbis_analysis_wrote(&vd, nsamples);

  /* vorbis does some data preanalysis, then divvies up blocks for
     more involved (potentially parallel) processing.  Get a single
     block for encoding now */
  while(vorbis_analysis_blockout(&vd, &vb) == 1) {

    /* analysis */
    vorbis_analysis(&vb, NULL);
	vorbis_bitrate_addblock(&vb);

	while (vorbis_bitrate_flushpacket(&vd, &op)) {
		/* weld the packet into the bitstream */
		ogg_stream_packetin(&os, &op);

		/* write out pages (if any) */
		while(ogg_stream_pageout(&os, &og) != 0) {
		  write(dpm.fd, og.header, og.header_len);
		  write(dpm.fd, og.body, og.body_len);
		}
	}
  }
  return 0;
}

static void close_output(void)
{
  int eos = 0;
  ogg_page   og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet op; /* one raw packet of data for decode */

  if(dpm.fd < 0)
    return;

  /* end of file.  this can be done implicitly in the mainline,
     but it's easier to see here in non-clever fashion.
     Tell the library we're at end of stream so that it can handle
     the last frame and mark end of stream in the output properly */
  vorbis_analysis_wrote(&vd, 0);

  /* vorbis does some data preanalysis, then divvies up blocks for
     more involved (potentially parallel) processing.  Get a single
     block for encoding now */
  while(vorbis_analysis_blockout(&vd, &vb) == 1) {

    /* analysis */
    vorbis_analysis(&vb, NULL);
    vorbis_bitrate_addblock(&vb);

    while(vorbis_bitrate_flushpacket(&vd,&op)) { 

    /* weld the packet into the bitstream */
    ogg_stream_packetin(&os, &op);

    /* write out pages (if any) */
    while(!eos){
      int result = ogg_stream_pageout(&os,&og);
      if(result == 0)
	break;
      write(dpm.fd, og.header, og.header_len);
      write(dpm.fd, og.body, og.body_len);

      /* this could be set above, but for illustrative purposes, I do
	 it here (to show that vorbis does know where the stream ends) */

      if(ogg_page_eos(&og))
	eos = 1;
    }
	}
  }

  /* clean up and exit.  vorbis_info_clear() must be called last */

  ogg_stream_clear(&os);
  vorbis_block_clear(&vb);
  vorbis_dsp_clear(&vd);
  vorbis_comment_clear(&vc);
  vorbis_info_clear(&vi);
  close(dpm.fd);

#ifdef AU_VORBIS_DLL
  free_vorbisenc_dll();
  free_vorbis_dll();
  free_ogg_dll();
#endif

  dpm.fd = -1;
}

static int acntl(int request, void *arg)
{
  switch(request) {
  case PM_REQ_PLAY_START:
    if(dpm.flag & PF_AUTO_SPLIT_FILE)
      return auto_ogg_output_open(current_file_info->filename,current_file_info->seq_name);
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

#endif
