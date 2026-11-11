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
#include "cores/AudioEngine/Utils/PackerMAT.h"
#include "utils/log.h"

#include <algorithm>

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace
{
constexpr auto TRUEHD_BUF_SIZE = 61440;
}

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(CProcessInfo &processInfo, CAEStreamInfo::DataType streamType) :
  CDVDAudioCodec(processInfo)
{
  m_format.m_streamInfo.m_type = streamType;
  m_deviceIsRAW = processInfo.WantsRawPassthrough();

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
  {
    m_trueHDBuffer.resize(TRUEHD_BUF_SIZE);

    if (!m_deviceIsRAW)
      m_packerMAT = std::make_unique<CPackerMAT>();
  }
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_parser.Reset();
  m_parser.SetCoreOnly(false);
  m_packetTimestampSpans.clear();
  m_nextInputOffset = 0;
  m_lastPtsSpanStart = UINT64_MAX;
  m_lastOriginalPtsSpanStart = UINT64_MAX;
  m_lastAccessUnitPts = DVD_NOPTS_VALUE;
  m_lastAccessUnitOriginalPts = DVD_NOPTS_VALUE;
  m_lastPtsFrameTime = 0;
  m_lastOriginalPtsFrameTime = 0;
  m_haveLastPtsFrameTime = false;
  m_haveLastOriginalPtsFrameTime = false;
  if (m_packerMAT)
    m_packerMAT->Reset();
  switch (m_format.m_streamInfo.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      m_codecName = "pt-ac3";
      break;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      m_codecName = "pt-eac3";
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      m_codecName = "pt-dtshd";
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      m_codecName = "pt-dtshd";
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      m_codecName = "pt-dts";
      m_parser.SetCoreOnly(true);
      break;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      m_codecName = "pt-truehd";
      CLog::LogF(LOGDEBUG, "passthrough output device is {}", m_deviceIsRAW ? "RAW" : "IEC");
      break;

    default:
      return false;
  }

  m_dataSize = 0;
  m_bufferSize = 0;
  m_backlogSize = 0;
  m_trueHDoffset = 0;
  m_trueHDframes = 0;
  m_currentPts = DVD_NOPTS_VALUE;
  m_currentOriginalPts = DVD_NOPTS_VALUE;
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
  Reset();
}

bool CDVDAudioCodecPassthrough::AddData(const DemuxPacket &packet)
{
  if (packet.pData && packet.iSize > 0)
  {
    const uint64_t size = static_cast<uint64_t>(packet.iSize);
    m_packetTimestampSpans.push_back(
        {m_nextInputOffset, m_nextInputOffset + size, packet.pts, packet.originalPts});
    m_nextInputOffset += size;
  }

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

  if (pData && !m_backlogSize && !m_dataSize)
  {
    if (iSize <= 0)
      return true;

    m_dataSize = m_bufferSize;
    int used = m_parser.AddData(pData, iSize, &m_buffer, &m_dataSize);
    m_bufferSize = std::max(m_bufferSize, m_dataSize);

    if (used != iSize)
    {
      if (m_backlogBufferSize < static_cast<unsigned int>(iSize - used))
      {
        m_backlogBufferSize = std::max(TRUEHD_BUF_SIZE, iSize - used);
        m_backlogBuffer = static_cast<uint8_t*>(realloc(m_backlogBuffer, m_backlogBufferSize));
      }
      m_backlogSize = iSize - used;
      memcpy(m_backlogBuffer, pData + used, m_backlogSize);
    }
  }
  else if (pData)
  {
    if (m_backlogBufferSize < (m_backlogSize + iSize))
    {
      m_backlogBufferSize = std::max(TRUEHD_BUF_SIZE, static_cast<int>(m_backlogSize + iSize));
      m_backlogBuffer = static_cast<uint8_t*>(realloc(m_backlogBuffer, m_backlogBufferSize));
    }
    memcpy(m_backlogBuffer + m_backlogSize, pData, iSize);
    m_backlogSize += iSize;
  }

  if (!m_dataSize && !m_backlogSize)
  {
    m_dataSize = m_bufferSize;
    m_parser.AddData(nullptr, 0, &m_buffer, &m_dataSize);
    m_bufferSize = std::max(m_bufferSize, m_dataSize);
  }

  if (!m_dataSize)
    return true;

  m_currentPts = GetPacketTimestampForAccessUnit(false);
  m_currentOriginalPts = GetPacketTimestampForAccessUnit(true);

  m_format.m_dataFormat = AE_FMT_RAW;
  m_format.m_streamInfo = m_parser.GetStreamInfo();
  m_format.m_sampleRate = m_parser.GetSampleRate();
  m_format.m_frameSize = 1;
  CAEChannelInfo layout;
  for (unsigned int i = 0; i < m_parser.GetChannels(); i++)
  {
    layout += AE_CH_RAW;
  }
  m_format.m_channelLayout = layout;

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
  {
    if (m_trueHDBuffer.empty())
    {
      m_trueHDBuffer.resize(TRUEHD_BUF_SIZE);

      if (!m_deviceIsRAW && !m_packerMAT)
        m_packerMAT = std::make_unique<CPackerMAT>();
    }

    if (m_deviceIsRAW) // RAW
    {
      m_dataSize = PackTrueHD();
    }
    else // IEC
    {
      const double duration = DVD_MSEC_TO_TIME(m_format.m_streamInfo.GetDuration());
      if (m_packerMAT->PackTrueHD(m_buffer, m_dataSize, m_currentPts, m_currentOriginalPts,
                                  duration, m_currentPts != DVD_NOPTS_VALUE,
                                  m_currentOriginalPts != DVD_NOPTS_VALUE))
      {
        bool hasPts{false};
        bool hasOriginalPts{false};
        m_trueHDBuffer =
            m_packerMAT->GetOutputFrame(m_currentPts, hasPts, m_currentOriginalPts, hasOriginalPts);
        if (!hasPts)
          m_currentPts = DVD_NOPTS_VALUE;
        if (!hasOriginalPts)
          m_currentOriginalPts = DVD_NOPTS_VALUE;
        m_dataSize = TRUEHD_BUF_SIZE;
      }
      else
        m_dataSize = 0;
    }
  }

  return true;
}

unsigned int CDVDAudioCodecPassthrough::PackTrueHD()
{
  unsigned int dataSize{0};

  if (m_trueHDoffset == 0)
  {
    m_trueHDframes = 0;
    m_trueHDCurrentPts = m_currentPts;
    m_trueHDCurrentOriginalPts = m_currentOriginalPts;
  }

  memcpy(m_trueHDBuffer.data() + m_trueHDoffset, m_buffer, m_dataSize);

  m_trueHDoffset += m_dataSize;
  m_trueHDframes++;

  if (m_trueHDframes == 24)
  {
    dataSize = m_trueHDoffset;
    m_trueHDoffset = 0;
    m_trueHDframes = 0;
    m_currentPts = m_trueHDCurrentPts;
    m_currentOriginalPts = m_trueHDCurrentOriginalPts;
    return dataSize;
  }

  return 0;
}

double CDVDAudioCodecPassthrough::GetPacketTimestampForAccessUnit(bool original)
{
  const uint64_t packetOffset = m_parser.GetPacketOffset();
  while (!m_packetTimestampSpans.empty() && m_packetTimestampSpans.front().end <= packetOffset)
    m_packetTimestampSpans.pop_front();

  const auto span =
      std::find_if(m_packetTimestampSpans.begin(), m_packetTimestampSpans.end(),
                   [packetOffset](const PacketTimestampSpan& candidate)
                   { return candidate.start <= packetOffset && packetOffset < candidate.end; });

  const double packetPts = span == m_packetTimestampSpans.end()
                               ? DVD_NOPTS_VALUE
                               : (original ? span->originalPts : span->pts);
  uint64_t& lastSpanStart = original ? m_lastOriginalPtsSpanStart : m_lastPtsSpanStart;
  double& lastAccessUnitPts = original ? m_lastAccessUnitOriginalPts : m_lastAccessUnitPts;
  uint16_t& lastFrameTime = original ? m_lastOriginalPtsFrameTime : m_lastPtsFrameTime;
  bool& haveLastFrameTime = original ? m_haveLastOriginalPtsFrameTime : m_haveLastPtsFrameTime;
  const uint16_t frameTime =
      static_cast<uint16_t>((static_cast<uint16_t>(m_buffer[2]) << 8) | m_buffer[3]);
  double pts = DVD_NOPTS_VALUE;

  if (span != m_packetTimestampSpans.end() && span->start != lastSpanStart &&
      packetPts != DVD_NOPTS_VALUE)
  {
    pts = packetPts;
  }
  else if (lastAccessUnitPts != DVD_NOPTS_VALUE && haveLastFrameTime)
  {
    const unsigned int sampleRate = m_parser.GetSampleRate();
    if (m_parser.GetDataType() == CAEStreamInfo::STREAM_TYPE_TRUEHD && sampleRate > 0)
    {
      unsigned int baseSampleRate = sampleRate;
      while (baseSampleRate > 48000 && (baseSampleRate % 2) == 0)
        baseSampleRate /= 2;

      if (baseSampleRate == 44100 || baseSampleRate == 48000)
      {
        uint16_t frameTimeDelta = static_cast<uint16_t>(frameTime - lastFrameTime);
        const uint16_t nominalFrameSamples =
            static_cast<uint16_t>(40 * (sampleRate / baseSampleRate));
        // A large jump indicates a branch or seek rather than one access-unit duration.
        if (frameTimeDelta == 0 || frameTimeDelta > sampleRate / 20)
          frameTimeDelta = nominalFrameSamples;
        pts =
            lastAccessUnitPts + DVD_TIME_BASE * (static_cast<double>(frameTimeDelta) / sampleRate);
      }
    }
    else
    {
      const double duration = DVD_MSEC_TO_TIME(m_parser.GetStreamInfo().GetDuration());
      if (duration > 0.0)
        pts = lastAccessUnitPts + duration;
    }
  }
  else if (packetPts != DVD_NOPTS_VALUE)
  {
    pts = packetPts;
  }

  if (span != m_packetTimestampSpans.end())
    lastSpanStart = span->start;
  if (pts != DVD_NOPTS_VALUE)
  {
    lastAccessUnitPts = pts;
    lastFrameTime = frameTime;
    haveLastFrameTime = true;
  }

  return pts;
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
  frame.originalPts = m_currentOriginalPts;
  m_currentPts = DVD_NOPTS_VALUE;
  m_currentOriginalPts = DVD_NOPTS_VALUE;
}

int CDVDAudioCodecPassthrough::GetData(uint8_t** dst)
{
  if (!m_dataSize)
  {
    if (m_packerMAT && m_packerMAT->HasOutputFrame())
    {
      bool hasPts{false};
      bool hasOriginalPts{false};
      m_trueHDBuffer =
          m_packerMAT->GetOutputFrame(m_currentPts, hasPts, m_currentOriginalPts, hasOriginalPts);
      if (!hasPts)
        m_currentPts = DVD_NOPTS_VALUE;
      if (!hasOriginalPts)
        m_currentOriginalPts = DVD_NOPTS_VALUE;
      m_dataSize = TRUEHD_BUF_SIZE;
    }
    else
      AddData(DemuxPacket());
  }

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    *dst = m_trueHDBuffer.data();
  else
    *dst = m_buffer;

  int bytes = m_dataSize;
  m_dataSize = 0;
  return bytes;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_trueHDoffset = 0;
  m_trueHDframes = 0;
  m_dataSize = 0;
  m_bufferSize = 0;
  m_backlogSize = 0;
  m_currentPts = DVD_NOPTS_VALUE;
  m_currentOriginalPts = DVD_NOPTS_VALUE;
  m_packetTimestampSpans.clear();
  m_nextInputOffset = 0;
  m_lastPtsSpanStart = UINT64_MAX;
  m_lastOriginalPtsSpanStart = UINT64_MAX;
  m_lastAccessUnitPts = DVD_NOPTS_VALUE;
  m_lastAccessUnitOriginalPts = DVD_NOPTS_VALUE;
  m_lastPtsFrameTime = 0;
  m_lastOriginalPtsFrameTime = 0;
  m_haveLastPtsFrameTime = false;
  m_haveLastOriginalPtsFrameTime = false;
  m_parser.Reset();
  if (m_packerMAT)
    m_packerMAT->Reset();
}

int CDVDAudioCodecPassthrough::GetBufferSize()
{
  return (int)m_parser.GetBufferSize();
}
