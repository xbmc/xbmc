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

#include "DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "utils/log.h"

#include <algorithm>

#include "cores/AudioEngine/AEFactory.h"

#define TRUEHD_BUF_SIZE 61440

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void) :
  m_buffer(NULL),
  m_bufferSize(0),
  m_trueHDoffset(0)
{
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_RAW;
  format.m_sampleRate = hints.samplerate;
  switch (hints.codec)
  {
    case AV_CODEC_ID_AC3:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
      format.m_streamInfo.m_sampleRate = hints.samplerate;
      break;

    case AV_CODEC_ID_EAC3:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_EAC3;
      format.m_streamInfo.m_sampleRate = hints.samplerate;
      break;

    case AV_CODEC_ID_DTS:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD;
      format.m_streamInfo.m_sampleRate = hints.samplerate;
      break;

    case AV_CODEC_ID_TRUEHD:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
      format.m_streamInfo.m_sampleRate = hints.samplerate;
      m_trueHDBuffer.reset(new uint8_t[TRUEHD_BUF_SIZE]);
      break;

    default:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_NULL;
  }

  bool ret = CAEFactory::SupportsRaw(format);

  m_parser.SetCoreOnly(false);
  if (!ret && hints.codec == AV_CODEC_ID_DTS)
  {
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_CORE;
    ret = CAEFactory::SupportsRaw(format);

    // only get the dts core from the parser if we don't support dtsHD
    m_parser.SetCoreOnly(true);
    m_bufferSize = 0;
  }

  return ret;
}

void CDVDAudioCodecPassthrough::Dispose()
{
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer = NULL;
  }

  m_bufferSize = 0;
}

int CDVDAudioCodecPassthrough::Decode(uint8_t* pData, int iSize)
{
  if (iSize <= 0)
    return 0;

  unsigned int used;
  m_dataSize = m_bufferSize;
  used = m_parser.AddData(pData, iSize, &m_buffer, &m_dataSize);
  m_bufferSize = std::max(m_bufferSize, m_dataSize);

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD && m_dataSize)
  {
    if (!m_trueHDoffset)
      memset(m_trueHDBuffer.get(), 0, TRUEHD_BUF_SIZE);

    memcpy(&(m_trueHDBuffer.get())[m_trueHDoffset], m_buffer, m_dataSize);
    uint8_t highByte = (m_dataSize >> 8) & 0xFF;
    uint8_t lowByte = m_dataSize & 0xFF;
    m_trueHDBuffer[m_trueHDoffset+2560-2] = highByte;
    m_trueHDBuffer[m_trueHDoffset+2560-1] = lowByte;
    m_trueHDoffset += 2560;

    if (m_trueHDoffset / 2560 == 24)
    {
      m_dataSize = m_trueHDoffset;
      m_trueHDoffset = 0;
    }
    else
      m_dataSize = 0;
  }

  if (m_dataSize)
  {
    m_format.m_dataFormat = AE_FMT_RAW;
    m_format.m_streamInfo = m_parser.GetStreamInfo();
    m_format.m_sampleRate = m_parser.GetSampleRate();
    m_format.m_frameSize = 1;
    CAEChannelInfo layout;
    for (unsigned int i=0; i<m_parser.GetChannels(); i++)
    {
      layout += AE_CH_RAW;
    }
    m_format.m_channelLayout = layout;
  }

  return used;
}

void CDVDAudioCodecPassthrough::GetData(DVDAudioFrame &frame)
{
  frame.nb_frames = GetData(frame.data);

  if (frame.nb_frames == 0)
    return;

  frame.passthrough = true;
  frame.format = m_format;
  frame.planes = 1;
  frame.bits_per_sample = 8;
  frame.duration = DVD_MSEC_TO_TIME(frame.format.m_streamInfo.GetDuration());
  frame.pts = DVD_NOPTS_VALUE;
}

int CDVDAudioCodecPassthrough::GetData(uint8_t** dst)
{
  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    *dst = m_trueHDBuffer.get();
  else
    *dst = m_buffer;
  return m_dataSize;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_trueHDoffset = 0;
}

int CDVDAudioCodecPassthrough::GetBufferSize()
{
  return (int)m_parser.GetBufferSize();
}
