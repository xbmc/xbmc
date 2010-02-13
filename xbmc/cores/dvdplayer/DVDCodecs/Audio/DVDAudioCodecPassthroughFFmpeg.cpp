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

#include "Encoders/DVDAudioEncoderFFmpeg.h"

//These values are forced to allow spdif out
#define OUT_SAMPLESIZE 16
#define OUT_CHANNELS   2
#define OUT_SAMPLERATE 48000

/* Lookup tables */
static const uint16_t AC3Bitrates[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod   [] = {48000, 44100, 32000, 0};

CDVDAudioCodecPassthroughFFmpeg::CDVDAudioCodecPassthroughFFmpeg(void)
{
  m_pFormat      = NULL;
  m_pStream      = NULL;
  m_pSyncFrame   = NULL;
  m_Buffer       = NULL;
  m_BufferSize   = 0;
  m_OutputSize   = 0;
  m_Consumed     = 0;

  m_Codec        = NULL;
  m_Encoder      = NULL;
  m_InitEncoder  = true;
  
  /* make enough room for at-least two audio frames */
  m_DecodeSize   = 0;
  m_DecodeBuffer = NULL;
}

CDVDAudioCodecPassthroughFFmpeg::~CDVDAudioCodecPassthroughFFmpeg(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthroughFFmpeg::SupportsFormat(CDVDStreamInfo &hints)
{
  m_pSyncFrame = NULL;

       if (m_bSupportsAC3Out && hints.codec == CODEC_ID_AC3) m_pSyncFrame = &CDVDAudioCodecPassthroughFFmpeg::SyncAC3;
  else if (m_bSupportsDTSOut && hints.codec == CODEC_ID_DTS) m_pSyncFrame = &CDVDAudioCodecPassthroughFFmpeg::SyncDTS;
  else if (m_bSupportsAACOut && hints.codec == CODEC_ID_AAC);
  else if (m_bSupportsMP1Out && hints.codec == CODEC_ID_MP1);
  else if (m_bSupportsMP2Out && hints.codec == CODEC_ID_MP2);
  else if (m_bSupportsMP3Out && hints.codec == CODEC_ID_MP3);
  else return false;

  return true;
}

bool CDVDAudioCodecPassthroughFFmpeg::SetupEncoder(CDVDStreamInfo &hints)
{
  /* there is no point encoding <= 2 channel sources, and we dont support anything but AC3 at the moment */
  if (hints.channels <= 2 || !m_bSupportsAC3Out)
    return false;

  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughFFmpeg::SetupEncoder - Setting up encoder for on the fly transcode");

  /* we need to decode the incoming audio data, so we can re-encode it as we need it */
  CDVDStreamInfo h(hints);
  m_Codec = CDVDFactoryCodec::CreateAudioCodec(h, false);
  if (!m_Codec)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::SetupEncoder - Unable to create a decoder for transcode");
    return false;
  }

  /* create and setup the encoder */
  m_Encoder     = new CDVDAudioEncoderFFmpeg();
  m_InitEncoder = true;

  /* adjust the hints according to the encorders output */
  hints.codec    = m_Encoder->GetCodecID();
  hints.bitrate  = m_Encoder->GetBitRate();
  hints.channels = OUT_CHANNELS;

  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughFFmpeg::SetupEncoder - Ready to transcode");
  return true;
}

bool CDVDAudioCodecPassthroughFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  // TODO - move this stuff somewhere else
  if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL)
  {
    m_bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    m_bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
    m_bSupportsAACOut = g_guiSettings.GetBool("audiooutput.aacpassthrough");
    m_bSupportsMP1Out = g_guiSettings.GetBool("audiooutput.mp1passthrough");
    m_bSupportsMP2Out = g_guiSettings.GetBool("audiooutput.mp2passthrough");
    m_bSupportsMP3Out = g_guiSettings.GetBool("audiooutput.mp3passthrough");
  }
  else
    return false;

  // TODO - this is only valid for video files, and should be moved somewhere else
  if( hints.channels == 2 && g_settings.m_currentVideoSettings.m_OutputToAllSpeakers )
  {
    CLog::Log(LOGINFO, "CDVDAudioCodecPassthroughFFmpeg::Open - disabled passthrough due to video OTAS");
    return false;
  }

  // TODO - some soundcards do support other sample rates, but they are quite uncommon
  if( hints.samplerate > 0 && hints.samplerate != 48000 )
  {
    CLog::Log(LOGINFO, "CDVDAudioCodecPassthrough::Open - disabled passthrough due to sample rate not being 48000");
    return false;
  }

  /* see if the muxer supports our codec (see spdif.c for supported formats) */
  if (!SupportsFormat(hints))
  {
    if (!SetupEncoder(hints) || !SupportsFormat(hints))
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Open - FFmpeg SPDIF muxer does not support this codec");
      Dispose();
      return false;
    }
  }
  else
  {
    m_Codec   = NULL;
    m_Encoder = NULL;
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
  m_pFormat->pb = m_dllAvFormat.av_alloc_put_byte(m_BCBuffer, AVCODEC_MAX_AUDIO_FRAME_SIZE, URL_RDONLY, this,  NULL, _BCReadPacket, NULL);
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
  params.sample_rate = hints.samplerate;
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
  m_pStream->codec->sample_fmt  = SAMPLE_FMT_S16;
  m_pStream->codec->channels    = hints.channels;
  m_pStream->codec->bit_rate    = hints.bitrate;

  /* we will check the first packet's crc */
  m_LostSync = true;
  return true;
}

void CDVDAudioCodecPassthroughFFmpeg::Dispose()
{
  Reset();

  if (m_DecodeBuffer)
  {
    _aligned_free(m_DecodeBuffer);
    m_DecodeBuffer = NULL;
  }

  delete m_Encoder;
  m_Encoder = NULL;

  delete m_Codec;
  m_Codec   = NULL;

  if (m_pFormat)
  {
    m_dllAvUtil.av_freep(&m_pFormat->pb);
    m_dllAvUtil.av_freep(&m_pFormat);
    m_dllAvUtil.av_freep(&m_pStream);
  }

  delete[] m_Buffer;
  m_Buffer = NULL;
  m_BufferSize = 0;
}

int CDVDAudioCodecPassthroughFFmpeg::BCReadPacket(uint8_t *buf, int buf_size)
{
  if (buf_size == 0)
    return 0;

  int s = buf_size;

#if (LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(52, 47, 0))
  /* to check see if the line "put_le16(s->pb, 0);" repeated 4 times in the end of the function "spdif_write_header" in spdif.c */
  #pragma warning "Make sure upstream version still needs this workaround (issue #1735)"
#endif
  /* the +8 a hack to work around a bug in the SPDIF muxer */
  s   -= 8;
  buf += 8;

  /* create a new packet and push it into our output buffer */
  DataPacket *packet = new DataPacket();
  packet->size       = s;
  packet->data       = new uint8_t[s];
  memcpy(packet->data, buf, s);

  m_OutputBuffer.push_back(packet);
  m_OutputSize += s;

  /* return how much we wrote to our buffer */
  return buf_size;
}

unsigned int CDVDAudioCodecPassthroughFFmpeg::SyncAC3(BYTE* pData, unsigned int iSize, unsigned int *fSize)
{
  unsigned int skip = 0;
  for(skip = 0; iSize - skip > 6; ++skip, ++pData)
  {
    /* search for an ac3 sync word */
    if(pData[0] != 0x0b || pData[1] != 0x77)
      continue;
 
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

    /* get the details we need to check crc1 and framesize */
    uint16_t     bitrate   = AC3Bitrates[frmsizecod >> 1];
    unsigned int framesize = 0;
    switch(fscod)
    {
      case 0: framesize = bitrate * 2; break;
      case 1: framesize = (320 * bitrate / 147 + (frmsizecod & 1 ? 1 : 0)); break;
      case 2: framesize = bitrate * 4; break;
    }

    *fSize = framesize * 2;

    /* dont do extensive testing if we have not lost sync */
    if (!m_LostSync && skip == 0)
      return 0;

    unsigned int crc_size;
    /* if we have enough data, validate the entire packet, else try to validate crc2 (5/8 of the packet) */
    if (framesize <= iSize - skip)
         crc_size = framesize - 1;
    else crc_size = (framesize >> 1) + (framesize >> 3) - 1;

    if (crc_size <= iSize - skip)
      if(m_dllAvUtil.av_crc(m_dllAvUtil.av_crc_get_table(AV_CRC_16_ANSI), 0, &pData[2], crc_size * 2))
        continue;

    /* if we get here, we can sync */
    m_LostSync = false;
    return skip;
  }

  /* if we get here, the entire packet is invalid and we have lost sync */
  m_LostSync = true;
  return iSize;
}

unsigned int CDVDAudioCodecPassthroughFFmpeg::SyncDTS(BYTE* pData, unsigned int iSize, unsigned int *fSize)
{
  unsigned int skip;
  for(skip = 0; iSize - skip > 8; ++skip, ++pData)
  {
    if (
      pData[0] != 0x7F ||
      pData[1] != 0xFE ||
      pData[2] != 0x80 ||
      pData[3] != 0x01
    ) continue;

    /* if it is not a termination frame, check the next 6 bits */
    if ((pData[4] & 0x80) != 0 && (pData[4] & 0x7C) != 0x7C)
      continue;

    /* get and validate the framesize */
    *fSize = ((((pData[5] & 0x3) << 8 | pData[6]) << 4) | ((pData[7] & 0xF0) >> 4)) + 1;
    if (*fSize < 95 || *fSize > 16383)
      continue;

    m_LostSync = false;
    return skip;
  }

  m_LostSync = true;
  return iSize;
}

int CDVDAudioCodecPassthroughFFmpeg::Decode(BYTE* pData, int iSize)
{
  unsigned int used, fSize;
  used = fSize = iSize;

  /* if we are transcoding */
  if (m_Encoder)
  {
    uint8_t *decData;
    uint8_t *encData;

    used  = m_Codec->Decode (pData, iSize);
    fSize = m_Codec->GetData(&decData);

    /* we may not get any data for a few frames, this is expected */
    if (fSize == 0)
      return used;

    /* now we have data, it is safe to initialize the encoder, as we should now have a channel map */
    if (m_InitEncoder)
    {
      if (m_Encoder->Initialize(m_Codec->GetChannels(), m_Codec->GetChannelMap(), m_Codec->GetBitsPerSample(), m_Codec->GetSampleRate()))
      {
        m_InitEncoder   = false;
        m_EncPacketSize = m_Encoder->GetPacketSize();
        /* allocate enough room for two packets of data */
        m_DecodeBuffer  = (uint8_t*)_aligned_malloc(m_EncPacketSize * 2, 16);
        m_DecodeSize    = 0;
      }
      else
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecPassthroughFFmpeg::Encode - Unable to initialize the encoder for transcode");
        return -1;
      }
    }

    unsigned int avail, eUsed, eCoded = 0;
    avail = fSize + m_DecodeSize;

    while(avail >= m_EncPacketSize)
    {
      /* append up to one packet of data to the buffer */
      if (m_DecodeSize < m_EncPacketSize)
      {
        unsigned int copy = (fSize > m_EncPacketSize) ? m_EncPacketSize : fSize;
        if (copy)
        {
          memcpy(m_DecodeBuffer + m_DecodeSize, decData, copy);
          m_DecodeSize += copy;
          decData      += copy;
          fSize        -= copy;
        }
      }

      /* encode the data and advance our data pointer */
      eUsed  = m_Encoder->Encode(m_DecodeBuffer, m_EncPacketSize);
      avail -= eUsed;

      /* shift buffered data along with memmove as the data can overlap */
      m_DecodeSize -= eUsed;
      memmove(m_DecodeBuffer, m_DecodeBuffer + eUsed, m_DecodeSize);

      /* output the frame of data */
      while((eCoded = m_Encoder->GetData(&encData)))
        WriteFrame(encData, eCoded);
    }

    /* append any leftover data to the buffer */
    memcpy(m_DecodeBuffer + m_DecodeSize, decData, fSize);
    m_DecodeSize += fSize;

    return used;
  }

  if (m_pSyncFrame)
  {
    int skip = (this->*m_pSyncFrame)(pData, iSize, &fSize);
    if (skip > 0)
      return skip;
  }

  WriteFrame(pData, fSize);
  return fSize;
}

void CDVDAudioCodecPassthroughFFmpeg::WriteFrame(uint8_t *pData, int iSize)
{
  AVPacket pkt;
  m_dllAvCodec.av_init_packet(&pkt);
  pkt.data = pData;
  pkt.size = iSize;

  m_Consumed += iSize;
  if (m_dllAvFormat.av_write_header(m_pFormat) != 0)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::WriteFrame - Failed to write the frame header");
    return;
  }

  if (m_dllAvFormat.av_write_frame(m_pFormat, &pkt) < 0)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::WriteFrame - Failed to write the frame data");
    return;
  }

  if (m_dllAvFormat.av_write_trailer(m_pFormat) != 0)
  {
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::WriteFrame - Failed to write the frame trailer");
    return;
  }
}

int CDVDAudioCodecPassthroughFFmpeg::GetData(BYTE** dst)
{
  int size;
  if(m_OutputSize)
  {
    /* check if the buffer is allocated */
    if (m_Buffer)
    {
      /* only re-allocate the buffer it is too small */
      if (m_BufferSize < m_OutputSize)
      {
        delete[] m_Buffer;
        m_Buffer = new uint8_t[m_OutputSize];
        m_BufferSize = m_OutputSize;
      }
    }
    else
    {
      /* allocate the buffer */
      m_Buffer     = new uint8_t[m_OutputSize];
      m_BufferSize = m_OutputSize;
    }

    /* fill the buffer with the output data */
    uint8_t *offset;
    offset = m_Buffer;
    while(!m_OutputBuffer.empty())
    {
      DataPacket* packet = m_OutputBuffer.front();
      m_OutputBuffer.pop_front();

      memcpy(offset, packet->data, packet->size);
      offset += packet->size;

      delete[] packet->data;
      delete   packet;
    }
    
    *dst = m_Buffer;
    size = m_OutputSize;
    m_OutputSize = 0;
    m_Consumed   = 0;
    return size;
  }
  else
    return 0;
}

void CDVDAudioCodecPassthroughFFmpeg::Reset()
{
  m_DecodeSize = 0;
  m_LostSync   = true;

  m_OutputSize = 0;
  m_Consumed   = 0;
  while(!m_OutputBuffer.empty())
  {
    DataPacket* packet = m_OutputBuffer.front();
    m_OutputBuffer.pop_front();
    delete[] packet->data;
    delete   packet;
  }

  if (m_Encoder)
    m_Encoder->Reset();
}

int CDVDAudioCodecPassthroughFFmpeg::GetChannels()
{
  //Can't return correct channels here as this is used to keep sync.
  //should probably have some other way to find out this
  return OUT_CHANNELS;
}

int CDVDAudioCodecPassthroughFFmpeg::GetSampleRate()
{
  //return OUT_SAMPLERATE;
  //not all cards support 44100hz output, we force 48000hz output
  return m_pStream->codec->sample_rate;
}

int CDVDAudioCodecPassthroughFFmpeg::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

int CDVDAudioCodecPassthroughFFmpeg::GetBufferSize()
{
  if (m_Codec)
    return m_Codec->GetBufferSize();
  else
    return m_Consumed;
}

