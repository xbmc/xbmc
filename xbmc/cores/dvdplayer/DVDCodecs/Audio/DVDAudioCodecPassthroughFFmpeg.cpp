/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DVDAudioCodecPassthroughFFmpeg.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "GUISettings.h"
#include "Settings.h"
#include "utils/log.h"

//These values are forced to allow spdif out
#define OUT_SAMPLESIZE 16
#define OUT_CHANNELS   2
#define OUT_SAMPLERATE 48000

/* Lookup tables */
static const uint16_t AC3Bitrates[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod   [] = {48000, 41000, 32000, 0};

CDVDAudioCodecPassthroughFFmpeg::CDVDAudioCodecPassthroughFFmpeg(void)
{
  m_pFormat      = NULL;
  m_pStream      = NULL;
  m_pSyncFrame   = NULL;

  m_OutputSize   = 0;
  m_OutputBuffer = (BYTE*)_aligned_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE, 16);
  memset(m_OutputBuffer, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
}

CDVDAudioCodecPassthroughFFmpeg::~CDVDAudioCodecPassthroughFFmpeg(void)
{
  _aligned_free(m_OutputBuffer);
  Dispose();
}

bool CDVDAudioCodecPassthroughFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  bool bSupportsAC3Out = false;
  bool bSupportsDTSOut = false;
  bool bSupportsAACOut = false;
  bool bSupportsMP1Out = false;
  bool bSupportsMP2Out = false;
  bool bSupportsMP3Out = false;

  // TODO - move this stuff somewhere else
  if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL)
  {
    bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
    bSupportsAACOut = g_guiSettings.GetBool("audiooutput.aacpassthrough");
    bSupportsMP1Out = g_guiSettings.GetBool("audiooutput.mp1passthrough");
    bSupportsMP2Out = g_guiSettings.GetBool("audiooutput.mp2passthrough");
    bSupportsMP3Out = g_guiSettings.GetBool("audiooutput.mp3passthrough");
  }

  //Samplerate cannot be checked here as we don't know it at this point in time.
  //We should probably have a way to try to decode data so that we know what samplerate it is.
  if ((hints.codec == CODEC_ID_AC3 && bSupportsAC3Out)
   || (hints.codec == CODEC_ID_DTS && bSupportsDTSOut)
   || (hints.codec == CODEC_ID_AAC && bSupportsAACOut)
   || (hints.codec == CODEC_ID_MP1 && bSupportsMP1Out)
   || (hints.codec == CODEC_ID_MP2 && bSupportsMP2Out)
   || (hints.codec == CODEC_ID_MP3 && bSupportsMP3Out))
  {

    // TODO - this is only valid for video files, and should be moved somewhere else
    if( hints.channels == 2 && g_settings.m_currentVideoSettings.m_OutputToAllSpeakers )
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecPassthroughFFmpeg::Open - disabled passthrough due to video OTAS");
      return false;
    }

    if (!m_dllAvFormat.Load() || !m_dllAvUtil.Load() || !m_dllAvCodec.Load())
      return false;

    /* get the muxer */
    AVOutputFormat *outFormat;
#if LIBAVFORMAT_VERSION_MAJOR < 53
    outFormat = m_dllAvFormat.guess_format("spdif", NULL, NULL);
#else
    outFormat = m_dllAvFormat.av_guess_format("spdif", NULL, NULL);
#endif
    if (!outFormat)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - Failed to get the ffmpeg spdif muxer");
      Dispose();
      return false;
    }

    /* see if the muxer supports our codec (see spdif.c for supported formats) */
    m_pSyncFrame = NULL;
    switch(hints.codec) {
      case CODEC_ID_AC3: m_pSyncFrame = &CDVDAudioCodecPassthroughFFmpeg::SyncAC3; break;
      case CODEC_ID_DTS: m_pSyncFrame = &CDVDAudioCodecPassthroughFFmpeg::SyncDTS; break;
      case CODEC_ID_MP1:
      case CODEC_ID_MP2:
      case CODEC_ID_MP3:
      case CODEC_ID_AAC:
        break;

      default:
        CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - ffmpeg SPDIF muxer does not support this codec");
        Dispose();
        return false;
    }

    /* allocate a the format context */
    m_pFormat = m_dllAvFormat.avformat_alloc_context();
    if (!m_pFormat)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - Failed to allocate AVFormat context");
      Dispose();
      return false;
    }
    m_pFormat->oformat = outFormat;

    /* allocate a put_byte struct so we can grab the output */
    m_pFormat->pb = m_dllAvFormat.av_alloc_put_byte(m_bcBuffer, AVCODEC_MAX_AUDIO_FRAME_SIZE, URL_RDONLY, this,  NULL, _BCReadPacket, NULL);
    if (!m_pFormat->pb)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - Failed to allocate ByteIOContext");
      Dispose();
      return false;
    }

    /* this is streamed, no file, and ignore the index */
    m_pFormat->pb->is_streamed   = 1;
    m_pFormat->flags            |= AVFMT_NOFILE | AVFMT_FLAG_IGNIDX;
    m_pFormat->bit_rate          = hints.bitrate;

    /* setup the muxer */
    AVFormatParameters params;
    memset(&params, 0, sizeof(params));
    params.channels    = hints.channels;
    params.sample_rate = OUT_SAMPLERATE;
    params.channel     = 0;
    if (m_dllAvFormat.av_set_parameters(m_pFormat, &params) != 0)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - Failed to set the spdif muxer parameters");
      Dispose();
      return false;
    }

    /* add a stream to it */
    m_pStream = m_dllAvFormat.av_new_stream(m_pFormat, 1);
    if (!m_pStream)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - Failed to allocate AVStream context");
      Dispose();
      return false;
    }

    /* set the stream's parameters */
    m_pStream->stream_copy        = 1;
    m_pStream->codec->codec_type  = CODEC_TYPE_AUDIO;
    m_pStream->codec->codec_id    = hints.codec;
    m_pStream->codec->sample_rate = hints.samplerate;
    m_pStream->codec->channels    = hints.channels;

    /* we will check the first packet's crc */
    m_lostSync = true;
    return true;
  }

  return false;
}

void CDVDAudioCodecPassthroughFFmpeg::Dispose()
{
  if (m_pFormat) {
    if (m_pFormat->pb)
      m_dllAvUtil.av_freep(&m_pFormat->pb);
    m_dllAvUtil.av_freep(&m_pFormat);

    if (m_pStream)
      m_dllAvUtil.av_freep(&m_pStream);
   }
}

int CDVDAudioCodecPassthroughFFmpeg::BCReadPacket(uint8_t *buf, int buf_size)
{
#if (LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(52, 47, 0))
  /* to check see if the line "put_le16(s->pb, 0);" repeated 4 times in the end of the function "spdif_write_header" in spdif.c */
  #pragma warning "Make sure upstream version still needs this workaround"
#endif
  /* this is a hack to work around a bug in the SPDIF muxer */
  m_OutputSize = buf_size - 8;
  memcpy(m_OutputBuffer, buf + 8, m_OutputSize);

  /* return how much we wrote to our buffer */
  return buf_size;
}

int CDVDAudioCodecPassthroughFFmpeg::SyncAC3(BYTE* pData, int iSize, int *fSize)
{
  int skip = 0;
  for(skip = 0; iSize - skip > 6; ++skip, ++pData)
  {
    /* search for an ac3 sync word */
    if(pData[0] != 0x0b || pData[1] != 0x77)
      continue;
 
    /* dont do extensive testing if we have not lost sync */
    if (!m_lostSync && skip == 0)
      return 0;

    uint8_t fscod      = pData[4] >> 6;
    uint8_t frmsizecod = pData[4] & 0x3F;
    uint8_t bsid       = pData[5] >> 3;

    /* sanity checks on the header */
    if (
        fscod      ==   3 ||
        frmsizecod >   37 ||
        bsid       > 0x11 ||
        AC3FSCod[fscod] != m_pStream->codec->sample_rate
    ) continue;

    /* get the details we need to check crc1 */
    uint16_t bitrate   = AC3Bitrates[frmsizecod >> 1];
    int      framesize = 0;
    switch(fscod)
    {
      case 0: framesize = bitrate * 2; break;
      case 1: framesize = (320 * bitrate / 147 + (frmsizecod & 1 ? 1 : 0)); break;
      case 2: framesize = bitrate * 4; break;
    }

    *fSize = framesize * 2;

    int crc_size;
    /* if we have enough data, validate the entire packet, else try to validate crc2 (5/8 of the packet) */
    if (framesize <= iSize - skip)
         crc_size = framesize - 1;
    else crc_size = (framesize >> 1) + (framesize >> 3) - 1;

    if (crc_size <= iSize - skip)
      if(m_dllAvUtil.av_crc(m_dllAvUtil.av_crc_get_table(AV_CRC_16_ANSI), 0, &pData[2], crc_size * 2))
        continue;

    /* if we get here, we can sync */
    m_lostSync = false;
    return skip;
  }

  /* if we get here, the entire packet is invalid and we have lost sync */
  m_lostSync = true;
  return iSize;
}

int CDVDAudioCodecPassthroughFFmpeg::SyncDTS(BYTE* pData, int iSize, int *fSize)
{
  int skip;
  for(skip = 0; iSize - skip > 8; ++skip, ++pData)
  {
    if (
      pData[0] != 0x7F ||
      pData[1] != 0xFE ||
      pData[2] != 0x80 ||
      pData[3] != 0x01
    ) continue;

    /* dont do extensive testing if we have not lost sync */
    if (!m_lostSync && skip == 0)
      return 0;

    /* if it is not a termination frame, check the next 6 bits */
    if (pData[4] & 0x80 != 0 && pData[4] & 0x7C != 0x7C)
      continue;

    /* get and validate the framesize */
    *fSize = ((pData[5] & 0x3) << 8 | pData[6]) << 4 | ((pData[7] & 0xF0) >> 4) + 1;
    if (*fSize < 95 || *fSize > 16383)
      continue;

    m_lostSync = false;
    return skip;
  }

  m_lostSync = true;
  return iSize;
}

int CDVDAudioCodecPassthroughFFmpeg::Decode(BYTE* pData, int iSize)
{
  int fSize = iSize;
  if (m_pSyncFrame)
  {
    int skip = (this->*m_pSyncFrame)(pData, iSize, &fSize);
    if (skip > 0)
      return skip;
  }

  AVPacket pkt;
  m_dllAvCodec.av_init_packet(&pkt);
  pkt.data = pData;
  pkt.size = fSize;

  m_dllAvFormat.av_write_header (m_pFormat);
  m_dllAvFormat.av_write_frame  (m_pFormat, &pkt);
  m_dllAvFormat.av_write_trailer(m_pFormat);
  return fSize;
}

int CDVDAudioCodecPassthroughFFmpeg::GetData(BYTE** dst)
{
  int size;
  if(m_OutputSize)
  {
    *dst = m_OutputBuffer;
    size = m_OutputSize;

    m_OutputSize = 0;
    return size;
  }
  else
    return 0;
}

void CDVDAudioCodecPassthroughFFmpeg::Reset()
{
  m_OutputSize = 0;
}

int CDVDAudioCodecPassthroughFFmpeg::GetChannels()
{
  //Can't return correct channels here as this is used to keep sync.
  //should probably have some other way to find out this
  return OUT_CHANNELS;
}

int CDVDAudioCodecPassthroughFFmpeg::GetSampleRate()
{
  return OUT_SAMPLERATE;
}

int CDVDAudioCodecPassthroughFFmpeg::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

