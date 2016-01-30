/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDAudioCodecPassthroughRaw.h"
#include "DVDCodecs/DVDCodecs.h"
#include "utils/log.h"

#include <algorithm>

#include "cores/AudioEngine/AEFactory.h"

//#define DEBUG_VERBOSE 1

#define CONSTANT_BUFFER_SIZE_SD 16384
#define CONSTANT_BUFFER_SIZE_HD 61440

CDVDAudioCodecPassthroughRaw::CDVDAudioCodecPassthroughRaw(void) :
  m_buffer    (NULL),
  m_bufferSize(0),
  m_infobuffer    (NULL),
  m_infobufferSize(0),
  m_bufferUsed(0),
  m_sampleRate(0),
  m_codec(AE_FMT_INVALID),
  m_frameoffset(0),
  m_framepos(0),
  m_pktperframe(1),
  m_constant_size(0)
{
}

CDVDAudioCodecPassthroughRaw::~CDVDAudioCodecPassthroughRaw(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthroughRaw::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_hints = hints;

  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::Open codec: %d; ch:%d; birate:%d; blk;%d; bits:%d; profile:%d", m_hints.codec, m_hints.channels, m_hints.bitrate, m_hints.blockalign, m_hints.bitspersample, m_hints.profile);

  bool bSupportsAC3Out    = CAEFactory::SupportsRaw((AEDataFormat)(AE_FMT_AC3 + PT_FORMAT_RAW_CLASS), hints.samplerate);
  bool bSupportsDTSOut    = CAEFactory::SupportsRaw((AEDataFormat)(AE_FMT_DTS + PT_FORMAT_RAW_CLASS), hints.samplerate);
  bool bSupportsEAC3Out   = CAEFactory::SupportsRaw((AEDataFormat)(AE_FMT_EAC3 + PT_FORMAT_RAW_CLASS), hints.samplerate);
  bool bSupportsTrueHDOut = CAEFactory::SupportsRaw((AEDataFormat)(AE_FMT_TRUEHD + PT_FORMAT_RAW_CLASS), hints.samplerate);
  bool bSupportsDTSHDOut  = CAEFactory::SupportsRaw((AEDataFormat)(AE_FMT_DTSHD + PT_FORMAT_RAW_CLASS), hints.samplerate);

  if ((hints.codec == AV_CODEC_ID_AC3 && bSupportsAC3Out) ||
      (hints.codec == AV_CODEC_ID_EAC3 && bSupportsEAC3Out) ||
      ((hints.codec == AV_CODEC_ID_DTS && hints.profile < 50) && bSupportsDTSOut) ||
      (hints.codec == AV_CODEC_ID_TRUEHD && bSupportsTrueHDOut) ||
      ((hints.codec == AV_CODEC_ID_DTS && hints.profile >= 50) && bSupportsDTSHDOut) )
  {
    return true;
  }

  return false;
}

void CDVDAudioCodecPassthroughRaw::GetData(DVDAudioFrame &frame)
{
  frame.nb_frames = 0;
  frame.data_format           = GetDataFormat();
  frame.bits_per_sample       = 8;
  frame.framesize             = 1;
  frame.planes                = 1;
  frame.passthrough           = NeedPassthrough();
  frame.pts                   = DVD_NOPTS_VALUE;
  frame.encoded_channel_count = GetEncodedChannels();
  frame.encoded_sample_rate   = GetEncodedSampleRate();
  frame.nb_frames             = GetData(frame.data)/frame.framesize;

  switch(m_codec)
  {
    case AE_FMT_AC3 + PT_FORMAT_RAW_CLASS:
    {
      frame.duration = 0.032;
      break;
    }
    case AE_FMT_EAC3 + PT_FORMAT_RAW_CLASS:
    {
      frame.duration = 1536.0 / frame.encoded_sample_rate;
      break;
    }
    case AE_FMT_TRUEHD + PT_FORMAT_RAW_CLASS:
    {
      int rate;
      if (frame.encoded_sample_rate == 48000 ||
          frame.encoded_sample_rate == 96000 ||
          frame.encoded_sample_rate == 192000)
        rate = 192000;
      else
        rate = 176400;
      frame.duration = 3840.0 / rate;
      break;
    }

    case AE_FMT_DTS + PT_FORMAT_RAW_CLASS:
    case AE_FMT_DTSHD + PT_FORMAT_RAW_CLASS:
    {
      frame.duration = 512.0 / frame.encoded_sample_rate;
      break;
    }
  }
  m_sampleRate = frame.nb_frames / frame.duration;
  frame.duration *= DVD_TIME_BASE;

  frame.sample_rate           = GetSampleRate();
  frame.channel_layout        = GetChannelMap();
  frame.channel_count         = GetChannels();


#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::GetData codec: %d; samplerate: %d; duration: %f", m_codec, frame.sample_rate, frame.duration);
#endif
}

int CDVDAudioCodecPassthroughRaw::GetSampleRate()
{
  if (m_sampleRate)
    return m_sampleRate;
  else
    return GetEncodedSampleRate();
}

int CDVDAudioCodecPassthroughRaw::GetEncodedSampleRate()
{
  return m_hints.samplerate;
}

enum AEDataFormat CDVDAudioCodecPassthroughRaw::GetDataFormat()
{
  enum AEDataFormat codec = AE_FMT_INVALID;
  unsigned int pktperframe = 1;

  switch(m_info.GetDataType())
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      codec = (AEDataFormat)(AE_FMT_AC3 + PT_FORMAT_RAW_CLASS);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      codec = (AEDataFormat)(AE_FMT_DTS + PT_FORMAT_RAW_CLASS);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      codec = (AEDataFormat)(AE_FMT_DTSHD + PT_FORMAT_RAW_CLASS);
      break;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      codec = (AEDataFormat)(AE_FMT_EAC3 + PT_FORMAT_RAW_CLASS);
      break;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      codec = (AEDataFormat)(AE_FMT_TRUEHD + PT_FORMAT_RAW_CLASS);
      pktperframe = 24;
      break;
  }
  if (codec != m_codec)
  {
    Cleanup();
    m_codec = codec;
    m_pktperframe = pktperframe;
  }

  return m_codec;
}

int CDVDAudioCodecPassthroughRaw::GetChannels()
{
  return m_info.GetOutputChannels();
}

int CDVDAudioCodecPassthroughRaw::GetEncodedChannels()
{
  return m_hints.channels;
}

CAEChannelInfo CDVDAudioCodecPassthroughRaw::GetChannelMap()
{
  return m_info.GetChannelMap();
}

void CDVDAudioCodecPassthroughRaw::Dispose()
{
  Cleanup();
}

int CDVDAudioCodecPassthroughRaw::Decode(uint8_t* pData, int iSize)
{
  if (iSize <= 0) return 0;

  // Do pseudo-encapsulation to make bitrate constant
  unsigned int used = 0;

  unsigned int size = m_infobufferSize;
  used = m_info.AddData(pData, iSize, &m_infobuffer, &size);
#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::Decode iSize(%d), size(%d), used(%d)", iSize, size, used);
#endif
  m_infobufferSize = std::max(m_infobufferSize, size);

  m_bufferUsed = 0;
  if (size)
  {
    if (!m_buffer)
    {
      if (m_info.GetDataType() == CAEStreamInfo::STREAM_TYPE_TRUEHD || m_info.GetDataType() == CAEStreamInfo::STREAM_TYPE_DTSHD)
        m_constant_size = CONSTANT_BUFFER_SIZE_HD;
      else
        m_constant_size = size * 2;

      m_bufferSize = m_constant_size;
      m_buffer = (uint8_t*)malloc(m_bufferSize);
      m_frameoffset = m_framepos = 0;
    }

    if (m_frameoffset + size > m_constant_size)
    {
      CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::Decode Error: Frame Buffer too small %d(%d) vs. %d", m_frameoffset + iSize, m_framepos, m_constant_size);
      return 0;
    }

    memcpy(m_buffer+sizeof(int)+m_frameoffset, m_infobuffer, size);
    m_frameoffset += size;
    m_framepos++;
    if (m_framepos == m_pktperframe)
    {
      ((int*)m_buffer)[0] = m_frameoffset;
      m_bufferUsed = m_bufferSize;
      m_frameoffset = m_framepos = 0;
    }
  }

  return used;
}

int CDVDAudioCodecPassthroughRaw::GetData(uint8_t** dst)
{
#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::GetData %d", m_bufferUsed);
#endif
  *dst     = m_buffer;
  return m_bufferUsed;
}

int CDVDAudioCodecPassthroughRaw::GetBufferSize()
{
  return (int)m_info.GetBufferSize();
}

void CDVDAudioCodecPassthroughRaw::Cleanup()
{
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer = NULL;
  }
  if (m_infobuffer)
  {
    delete[] m_infobuffer;
    m_infobuffer = NULL;
  }

  m_pktperframe = 1;
  m_bufferSize = 0;
  m_bufferUsed = 0;
  m_infobufferSize = 0;
  m_sampleRate = 0;
}
