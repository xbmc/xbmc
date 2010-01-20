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
** $Id: faad2.cpp,v 1.3 2004/08/20 08:30:53 menno Exp $
**/
#include "faad2.h"
#include <mpeg4_audio_config.h>
#include <mpeg4_sdp.h>
#include <mp4.h>
#include <SDL/SDL.h>

#define DEBUG_SYNC 2

#ifndef M_LLU
#define M_LLU M_64
#define LLU U64
#endif
const char *aaclib="faad2";

/*
 * Create CAACodec class
 */
static codec_data_t *aac_codec_create (const char *compressor,
                       int type,
                       int profile,
                       format_list_t *media_fmt,
                       audio_info_t *audio,
                       const uint8_t *userdata,
                       uint32_t userdata_size,
                       audio_vft_t *vft,
                       void *ifptr)

{
  aac_codec_t *aac;

  aac = (aac_codec_t *)malloc(sizeof(aac_codec_t));
  memset(aac, 0, sizeof(aac_codec_t));

  aac->m_vft = vft;
  aac->m_ifptr = ifptr;
  fmtp_parse_t *fmtp = NULL;
  // Start setting up FAAC stuff...

  aac->m_resync_with_header = 1;
  aac->m_record_sync_time = 1;

  aac->m_audio_inited = 0;

  // Use media_fmt to indicate that we're streaming.
  if (media_fmt != NULL) {
    // haven't checked for null buffer
    // This is not necessarilly right - it is, for the most part, but
    // we should be reading the fmtp statement, and looking at the config.
    // (like we do below in the userdata section...
    aac->m_freq = media_fmt->rtpmap->clock_rate;
    fmtp = parse_fmtp_for_mpeg4(media_fmt->fmt_param, vft->log_msg);
    if (fmtp != NULL) {
      userdata = fmtp->config_binary;
      userdata_size = fmtp->config_binary_len;
    }
  }

  aac->m_info = faacDecOpen();
  unsigned long srate;
  unsigned char chan;
  if ((userdata == NULL && fmtp == NULL) ||
      (faacDecInit2(aac->m_info,
             (uint8_t *)userdata,
             userdata_size,
            &srate,
             &chan) < 0)) {
      if (fmtp != NULL) free_fmtp_parse(fmtp);
      return NULL;
  }

  mp4AudioSpecificConfig mp4ASC;
  aac->m_output_frame_size = 1024;
  if (AudioSpecificConfig((unsigned char *)userdata,
              userdata_size,
              &mp4ASC)) {
    if (mp4ASC.frameLengthFlag) {
      aac->m_output_frame_size = 960;
    }
  }
  aac->m_freq = srate;
  aac->m_chans = chan;
  aac->m_faad_inited = 1;
  aac->m_msec_per_frame = aac->m_output_frame_size;
  aac->m_msec_per_frame *= M_LLU;
  aac->m_msec_per_frame /= aac->m_freq;

  //  faad_init_bytestream(&m_info->ld, c_read_byte, c_bookmark, m_bytestream);

  aa_message(LOG_INFO, aaclib, "Setting freq to %d", aac->m_freq);
#if DUMP_OUTPUT_TO_FILE
  aac->m_outfile = fopen("temp.raw", "w");
#endif
  if (fmtp != NULL) {
    free_fmtp_parse(fmtp);
  }
  return (codec_data_t *)aac;
}

void aac_close (codec_data_t *ptr)
{
  if (ptr == NULL) {
    return;
  }
  aac_codec_t *aac = (aac_codec_t *)ptr;
  faacDecClose(aac->m_info);
  aac->m_info = NULL;

#if DUMP_OUTPUT_TO_FILE
  fclose(aac->m_outfile);
#endif
  free(aac);
}

/*
 * Handle pause - basically re-init the codec
 */
static void aac_do_pause (codec_data_t *ifptr)
{
  aac_codec_t *aac = (aac_codec_t *)ifptr;
  aac->m_resync_with_header = 1;
  aac->m_record_sync_time = 1;
  aac->m_audio_inited = 0;
  aac->m_ignore_first_sample = 0;
  faacDecPostSeekReset(aac->m_info, 0);
}

/*
 * Decode task call for FAAC
 */
static int aac_decode (codec_data_t *ptr,
               uint64_t ts,
               int from_rtp,
               int *sync_frame,
               uint8_t *buffer,
               uint32_t buflen,
               void *userdata)
{
  aac_codec_t *aac = (aac_codec_t *)ptr;
  unsigned long bytes_consummed;
  int bits = -1;
  //  struct timezone tz;

  if (aac->m_record_sync_time) {
    aac->m_current_frame = 0;
    aac->m_record_sync_time = 0;
    aac->m_current_time = ts;
    aac->m_last_rtp_ts = ts;
  } else {
    if (aac->m_last_rtp_ts == ts) {
      aac->m_current_time += aac->m_msec_per_frame;
      aac->m_current_frame++;
    } else {
      aac->m_last_rtp_ts = ts;
      aac->m_current_time = ts;
      aac->m_current_frame = 0;
    }

    // Note - here m_current_time should pretty much always be >= rtpts.
    // If we're not, we most likely want to stop and resync.  We don't
    // need to keep decoding - just decode this frame and indicate we
    // need a resync... That should handle fast forwards...  We need
    // someway to handle reverses - perhaps if we're more than .5 seconds
    // later...
  }

  if (aac->m_faad_inited == 0) {
    /*
     * If not initialized, do so.
     */
    abort();
    unsigned long freq;
    unsigned char chans;

    faacDecInit(aac->m_info,
        (unsigned char *)buffer,
        buflen,
        &freq,
        &chans);
    aac->m_freq = freq;
    aac->m_chans = chans;
    aac->m_faad_inited = 1;
  }

  uint8_t *buff;
  unsigned long samples;
  bytes_consummed = buflen;
  //aa_message(LOG_DEBUG, aaclib, "decoding %d bits", buflen * 8);
  faacDecFrameInfo frame_info;
  buff = (uint8_t *)faacDecDecode(aac->m_info,
                  &frame_info,
                  buffer,
                  buflen);
  if (buff != NULL) {
    bytes_consummed = frame_info.bytesconsumed;
#if 0
    aa_message(LOG_DEBUG, aaclib, LLU" bytes %d samples %d",
           ts, bytes_consummed, frame_info.samples);
#endif
    if (aac->m_audio_inited != 0) {
      int tempchans = frame_info.channels;
      if (tempchans != aac->m_chans) {
    aa_message(LOG_NOTICE, aaclib, "chupdate - chans from data is %d",
           tempchans);
      }
    } else {
      int tempchans = frame_info.channels;

      if (tempchans == 0) {
    aa_message(LOG_ERR, aaclib, "initializing aac, returned channels are 0");
    aac->m_resync_with_header = 1;
    aac->m_record_sync_time = 1;
    return bytes_consummed;
      }
      aac->m_chans = tempchans;
      aac->m_freq = frame_info.samplerate;

      aac->m_vft->audio_configure(aac->m_ifptr,
                  aac->m_freq,
                  aac->m_chans,
                  (audio_format_t)AUDIO_S16SYS,
                  aac->m_output_frame_size);
      uint8_t *now = aac->m_vft->audio_get_buffer(aac->m_ifptr);
      aac->m_audio_inited = 1;
    }
    /*
     * good result - give it to audio sync class
     */
#if DUMP_OUTPUT_TO_FILE
    fwrite(buff, aac->m_output_frame_size * 4, 1, aac->m_outfile);
#endif
    if (frame_info.samples != 0) {
      aac->m_vft->audio_load_buffer(aac->m_ifptr,
                    buff,
                    frame_info.samples * 2,
                    aac->m_last_ts,
                    aac->m_resync_with_header);
      if (aac->m_resync_with_header == 1) {
    aac->m_resync_with_header = 0;
#ifdef DEBUG_SYNC
    aa_message(LOG_DEBUG, aaclib, "Back to good at "LLU, aac->m_current_time);
#endif
      }
    }
  } else {
    aa_message(LOG_ERR, aaclib, "error return is %d", frame_info.error);
    aac->m_resync_with_header = 1;
#ifdef DEBUG_SYNC
    aa_message(LOG_ERR, aaclib, "Audio decode problem - at "LLU,
           aac->m_current_time);
#endif
  }
  aac->m_last_ts = aac->m_current_time;
  return (bytes_consummed);
}

static const char *aac_compressors[] = {
  "aac ",
  "mp4a",
  "enca",
  NULL
};

static int aac_codec_check (lib_message_func_t message,
                const char *compressor,
                int type,
                int profile,
                format_list_t *fptr,
                const uint8_t *userdata,
                uint32_t userdata_size
#ifdef HAVE_PLUGIN_VERSION_0_8
              ,CConfigSet *pConfig
#endif
              )
{
  fmtp_parse_t *fmtp = NULL;
  if (compressor != NULL &&
      strcasecmp(compressor, "MP4 FILE") == 0 &&
      type != -1) {
    switch (type) {
    case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
    case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
    case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
    case MP4_MPEG4_AUDIO_TYPE:
      break;
    default:
      return -1;
    }
  }
  if (fptr != NULL &&
      fptr->rtpmap != NULL &&
      fptr->rtpmap->encode_name != NULL) {
    if (strcasecmp(fptr->rtpmap->encode_name, "mpeg4-generic") != 0) {
      return -1;
    }
    if (userdata == NULL) {
      fmtp = parse_fmtp_for_mpeg4(fptr->fmt_param, message);
      if (fmtp != NULL) {
    userdata = fmtp->config_binary;
    userdata_size = fmtp->config_binary_len;
      }
    }
  }
  if (userdata != NULL) {
    mpeg4_audio_config_t audio_config;
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config);
    message(LOG_DEBUG, "aac", "audio type is %d", audio_config.audio_object_type);
    if (fmtp != NULL) free_fmtp_parse(fmtp);

    if (audio_object_type_is_aac(&audio_config) == 0) {
      return -1;
    }
#if 0
    if (audio_config.audio_object_type == 17) {
      message(LOG_INFO, "aac", "audio type is legal ISMA, but not supported");
      return -1;
    }
#endif
    return 2;
  }
#if 0
  // I'm not sure I want to be here if we don't have an audio config
  if (compressor != NULL) {
    const char **lptr = aac_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
    return 2;
      }
      lptr++;
    }
  }
#endif
  return -1;
}

#ifndef HAVE_PLUGIN_VERSION_0_8
AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("faad2",
                 aac_codec_create,
                 aac_do_pause,
                 aac_decode,
                 NULL,
                 aac_close,
                 aac_codec_check,
                 aac_file_check,
                 aac_file_next_frame,
                 aac_file_used_for_frame,
                 aac_raw_file_seek_to,
               aac_file_eof
               );
#else
AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("faad2",
               aac_codec_create,
               aac_do_pause,
               aac_decode,
               NULL,
               aac_close,
               aac_codec_check,
               aac_file_check,
               aac_file_next_frame,
               aac_file_used_for_frame,
               aac_raw_file_seek_to,
               aac_file_eof,
               NULL,
               0
               );
#endif
/* end file aa.cpp */


