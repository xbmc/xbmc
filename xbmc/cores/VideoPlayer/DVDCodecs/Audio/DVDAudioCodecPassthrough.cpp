/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "utils/log.h"

#include <algorithm>

extern "C" {
#include "libavcodec/avcodec.h"
}

#define TRUEHD_BUF_SIZE 61440

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(CProcessInfo &processInfo, CAEStreamInfo::DataType streamType) :
  CDVDAudioCodec(processInfo)
{
  m_format.m_streamInfo.m_type = streamType;
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_parser.SetCoreOnly(false);
  switch (m_format.m_streamInfo.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      m_codecName = "pt-ac3";
      break;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      m_codecName = "pt-eac3";
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      m_codecName = "pt-dtshd";
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      m_codecName = "pt-dts";
      m_parser.SetCoreOnly(true);
      break;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      m_trueHDBuffer.reset(new uint8_t[TRUEHD_BUF_SIZE]);
      m_codecName = "pt-truehd";
      break;

    default:
      return false;
  }

  m_dataSize = 0;
  m_bufferSize = 0;
  m_backlogSize = 0;
  m_currentPts = DVD_NOPTS_VALUE;
  m_nextPts = DVD_NOPTS_VALUE;
  return true;
}

void CDVDAudioCodecPassthrough::Dispose()
{
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer = NULL;
  }

  free(m_backlogBuffer);
  m_backlogBuffer = nullptr;
  m_backlogBufferSize = 0;

  m_bufferSize = 0;
}

bool CDVDAudioCodecPassthrough::AddData(const DemuxPacket &packet)
{
  int used = 0;
  if (m_backlogSize)
  {
    m_dataSize = m_bufferSize;
    unsigned int consumed = m_parser.AddData(m_backlogBuffer, m_backlogSize, &m_buffer, &m_dataSize);
    m_bufferSize = std::max(m_bufferSize, m_dataSize);
    if (consumed != m_backlogSize)
    {
      memmove(m_backlogBuffer, m_backlogBuffer+consumed, m_backlogSize-consumed);
    }
    m_backlogSize -= consumed;
  }

  unsigned char *pData(const_cast<uint8_t*>(packet.pData));
  int iSize(packet.iSize);

  if (pData)
  {
    if (m_currentPts == DVD_NOPTS_VALUE)
    {
      if (m_nextPts != DVD_NOPTS_VALUE)
      {
        m_currentPts = m_nextPts;
        m_nextPts = packet.pts;
      }
      else if (packet.pts != DVD_NOPTS_VALUE)
      {
        m_currentPts = packet.pts;
      }
    }
    else
    {
      m_nextPts = packet.pts;
    }
  }

  if (pData && !m_backlogSize)
  {
    if (iSize <= 0)
      return true;

    m_dataSize = m_bufferSize;
    used = m_parser.AddData(pData, iSize, &m_buffer, &m_dataSize);
    m_bufferSize = std::max(m_bufferSize, m_dataSize);

    if (used != iSize)
    {
      if (m_backlogBufferSize < static_cast<unsigned int>(iSize - used))
      {
        m_backlogBufferSize = std::max(61440, iSize - used);
        m_backlogBuffer = static_cast<uint8_t*>(realloc(m_backlogBuffer, m_backlogBufferSize));
      }
      m_backlogSize = iSize - used;
      memcpy(m_backlogBuffer, pData + used, m_backlogSize);
      used = iSize;
    }
  }
  else if (pData)
  {
    if (m_backlogBufferSize < (m_backlogSize + iSize))
    {
      m_backlogBufferSize = std::max(61440, static_cast<int>(m_backlogSize + iSize));
      m_backlogBuffer = static_cast<uint8_t*>(realloc(m_backlogBuffer, m_backlogBufferSize));
    }
    memcpy(m_backlogBuffer + m_backlogSize, pData, iSize);
    m_backlogSize += iSize;
    used = iSize;
  }

  if (!m_dataSize)
    return true;

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

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
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

  return true;
}

void CDVDAudioCodecPassthrough::GetData(DVDAudioFrame &frame)
{
  frame.nb_frames = GetData(frame.data);
  frame.framesOut = 0;

  if (frame.nb_frames == 0)
    return;

  frame.passthrough = true;
  frame.format = m_format;
  frame.planes = 1;
  frame.bits_per_sample = 8;
  frame.duration = DVD_MSEC_TO_TIME(frame.format.m_streamInfo.GetDuration());
  frame.pts = m_currentPts;
  m_currentPts = DVD_NOPTS_VALUE;
}

int CDVDAudioCodecPassthrough::GetData(uint8_t** dst)
{
  if (!m_dataSize)
    AddData(DemuxPacket());

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    *dst = m_trueHDBuffer.get();
  else
    *dst = m_buffer;

  int bytes = m_dataSize;
  m_dataSize = 0;
  return bytes;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_trueHDoffset = 0;
  m_dataSize = 0;
  m_bufferSize = 0;
  m_backlogSize = 0;
  m_currentPts = DVD_NOPTS_VALUE;
  m_nextPts = DVD_NOPTS_VALUE;
  m_parser.Reset();
}

int CDVDAudioCodecPassthrough::GetBufferSize()
{
  return (int)m_parser.GetBufferSize();
}
