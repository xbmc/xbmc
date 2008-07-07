/*
** MPEG4IP plugin for FAAD2
** Copyright (C) 2003 Bill May wmay@cisco.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: faad2.h,v 1.2 2004/01/05 14:05:12 menno Exp $
**/
/*
 * aa.h - class definition for AAC codec.
 */

#ifndef __AA_H__
#define __AA_H__ 1
#include "faad.h"
#include "codec_plugin.h"

#ifndef M_LLU
#define M_LLU M_64
#define LLU U64
#endif

typedef struct aac_codec_t {
  codec_data_t c;
  audio_vft_t *m_vft;
  void *m_ifptr;
  faacDecHandle m_info;
  int m_object_type;
  int m_resync_with_header;
  int m_record_sync_time;
  uint64_t m_current_time;
  uint64_t m_last_rtp_ts;
  uint64_t m_msec_per_frame;
  uint32_t m_current_frame;
  int m_audio_inited;
  int m_faad_inited;
  int m_freq;  // frequency
  int m_chans; // channels
  int m_output_frame_size;
#if DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  FILE *m_ifile;
  uint8_t *m_buffer;
  uint32_t m_buffer_size_max;
  uint32_t m_buffer_size;
  uint32_t m_buffer_on;
  uint64_t m_framecount;
  int m_ignore_first_sample;
  uint64_t m_last_ts;
} aac_codec_t;

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr
#define MAX_READ_BUFFER (768 * 8)

#define aa_message aac->m_vft->log_msg
void aac_close(codec_data_t *ptr);
extern const char *aaclib;

codec_data_t *aac_file_check(lib_message_func_t message,
                 const char *name,
                 double *max,
                 char *desc[4]
#ifdef HAVE_PLUGIN_VERSION_0_8
               , CConfigSet *pConfig
#endif
);

int aac_file_next_frame(codec_data_t *ifptr,
            uint8_t **buffer,
            uint64_t *ts);
int aac_file_eof(codec_data_t *ifptr);

void aac_file_used_for_frame(codec_data_t *ifptr,
                 uint32_t bytes);

int aac_raw_file_seek_to(codec_data_t *ifptr,
             uint64_t ts);
#endif
