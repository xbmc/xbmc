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

#define DEBUG_VERBOSE 1

static enum AEChannel OutputMaps[2][9] = {
  {AE_CH_RAW, AE_CH_RAW, AE_CH_NULL},
  {AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_NULL}
};

#define AC3_DIVISOR 1536
#define DTS_DIVISOR 512
#define TRUEHD_DIVISOR 960
#define CONSTANT_BUFFER_SIZE_SD 16384
#define CONSTANT_BUFFER_SIZE_HD 61440

CDVDAudioCodecPassthroughRaw::CDVDAudioCodecPassthroughRaw(void) :
  m_buffer    (NULL),
  m_bufferSize(0),
  m_bufferUsed(0),
  m_sampleRate(0),
  m_codec(AE_FMT_INVALID),
  m_trueHDoffset(0),
  m_trueHDpos(0)
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
    GetDataFormat();
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

  float unscaledbitrate = (float)(frame.nb_frames*frame.framesize) * frame.encoded_sample_rate;
  switch(m_codec)
  {
    case AE_FMT_AC3 + PT_FORMAT_RAW_CLASS:
    {
      m_sampleRate = (float)(frame.nb_frames*frame.framesize) / 0.032;  // fixed 32ms
      break;
    }
    case AE_FMT_EAC3 + PT_FORMAT_RAW_CLASS:
    {
      m_sampleRate = (unscaledbitrate + AC3_DIVISOR/2)  / AC3_DIVISOR;
      break;
    }
    case AE_FMT_TRUEHD + PT_FORMAT_RAW_CLASS:
    {
        m_sampleRate = (unscaledbitrate + TRUEHD_DIVISOR/2) / TRUEHD_DIVISOR;
      break;
    }

    case AE_FMT_DTS + PT_FORMAT_RAW_CLASS:
    case AE_FMT_DTSHD + PT_FORMAT_RAW_CLASS:
    {
        m_sampleRate = (unscaledbitrate + DTS_DIVISOR/2) / DTS_DIVISOR;
      break;
    }

    default:
      m_sampleRate = GetSampleRate();
      break;
  }
  frame.sample_rate           = GetSampleRate();
  frame.channel_layout        = GetChannelMap();
  frame.channel_count         = GetChannels();

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::GetData samplerate: %d", frame.sample_rate);
#endif

  // compute duration.
  if (frame.sample_rate)
    frame.duration = ((double)frame.nb_frames * DVD_TIME_BASE) / frame.sample_rate;
  else
    frame.duration = 0.0;
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
  if (m_codec != AE_FMT_INVALID)
    return m_codec;

  switch(m_hints.codec)
  {
    case AV_CODEC_ID_AC3:
      m_codec = (AEDataFormat)(AE_FMT_AC3 + PT_FORMAT_RAW_CLASS);
      break;

    case AV_CODEC_ID_DTS:
      if (m_hints.profile >= 50)
        m_codec = (AEDataFormat)(AE_FMT_DTSHD + PT_FORMAT_RAW_CLASS);
      else
        m_codec = (AEDataFormat)(AE_FMT_DTS + PT_FORMAT_RAW_CLASS);
      break;

    case AV_CODEC_ID_EAC3:
      m_codec = (AEDataFormat)(AE_FMT_EAC3 + PT_FORMAT_RAW_CLASS);
      break;

    case AV_CODEC_ID_TRUEHD:
      m_codec = (AEDataFormat)(AE_FMT_TRUEHD + PT_FORMAT_RAW_CLASS);
      break;
  }

  return m_codec;
}

int CDVDAudioCodecPassthroughRaw::GetChannels()
{
  unsigned int codec = m_codec & ~PT_FORMAT_RAW_CLASS;
  switch (codec)
  {
    case AE_FMT_AC3:
    case AE_FMT_EAC3:
    case AE_FMT_DTS:
      return 2;

    default:
      return 8;
  }
}

int CDVDAudioCodecPassthroughRaw::GetEncodedChannels()
{
  return m_hints.channels;
}

CAEChannelInfo CDVDAudioCodecPassthroughRaw::GetChannelMap()
{
  int count = 0;
  unsigned int codec = m_codec & ~PT_FORMAT_RAW_CLASS;
  switch (codec)
  {
    case AE_FMT_AC3:
    case AE_FMT_EAC3:
    case AE_FMT_DTS:
      count = 2;

    default:
      count = 8;
  }

  if (count > 6)
    return CAEChannelInfo(OutputMaps[1]);
  else
    return CAEChannelInfo(OutputMaps[0]);
}

void CDVDAudioCodecPassthroughRaw::Dispose()
{
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer = NULL;
  }

  m_bufferSize = 0;
  m_sampleRate = 0;
}

int CDVDAudioCodecPassthroughRaw::Decode(uint8_t* pData, int iSize)
{
#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::Decode %d", iSize);
#endif
  if (iSize <= 0) return 0;

  // Do pseudo-encapsulation to make bitrate constant
  int constant_size = 0;
  unsigned int codec = m_codec & ~PT_FORMAT_RAW_CLASS;
  if (codec == AE_FMT_TRUEHD || codec == AE_FMT_DTSHD)
    constant_size = CONSTANT_BUFFER_SIZE_HD;
  else
    constant_size = CONSTANT_BUFFER_SIZE_SD;

  if (constant_size)
  {
    if (!m_buffer)
    {
      m_bufferSize = constant_size;
      m_buffer = (uint8_t*)malloc(m_bufferSize);
    }
    if (codec == AE_FMT_TRUEHD)
    {
      if (m_trueHDoffset + iSize > constant_size)
      {
        CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::Decode Error: TrueHD Buffer too small %d(%d)", m_trueHDoffset + iSize, m_trueHDpos);
        return 0;
      }

      memcpy(m_buffer+sizeof(int)+m_trueHDoffset, pData, iSize);
      m_trueHDoffset += iSize;
      m_trueHDpos++;
      if (m_trueHDpos == 24)
      {
        ((int*)m_buffer)[0] = m_trueHDoffset;
        m_bufferUsed = m_bufferSize;
        m_trueHDoffset = m_trueHDpos = 0;
      }
      else
        m_bufferUsed = 0;
    }
    else
    {
      if (iSize > constant_size)
      {
        CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::Decode Error: Buffer too small %d", iSize);
        return 0;
      }

      ((int*)m_buffer)[0] = iSize;
      memcpy(m_buffer+sizeof(int), pData, iSize);
      m_bufferUsed = m_bufferSize;
    }

  }
  else
  {
    if (iSize > m_bufferSize)
    {
      m_bufferSize = iSize;
      m_buffer = (uint8_t*)realloc(m_buffer, m_bufferSize);
    }

    memcpy(m_buffer, pData, iSize);
    m_bufferUsed = iSize;
  }
  return iSize;
}

int CDVDAudioCodecPassthroughRaw::GetData(uint8_t** dst)
{
#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthroughRaw::GetData %d", m_bufferUsed);
#endif
  *dst     = m_buffer;
  return m_bufferUsed;
}

void CDVDAudioCodecPassthroughRaw::Reset()
{
}
