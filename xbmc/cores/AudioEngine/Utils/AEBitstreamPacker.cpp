/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AEBitstreamPacker.h"

#include "AEPackIEC61937.h"
#include "AEStreamInfo.h"
#include "Util.h"
#include "utils/log.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern "C"
{
#include <libavutil/intreadwrite.h>
}

CAEBitstreamPacker::CAEBitstreamPacker()
{
  Reset();
}

CAEBitstreamPacker::~CAEBitstreamPacker()
{
}

void CAEBitstreamPacker::Pack(CAEStreamInfo &info, uint8_t* data, int size)
{
  m_pauseDuration = 0;
  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      PackTrueHD(info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      PackDTSHD (info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_AC3:
      m_dataSize = CAEPackIEC61937::PackAC3(data, size, m_packedBuffer);
      break;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      PackEAC3 (info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
      m_dataSize = CAEPackIEC61937::PackDTS_512(data, size, m_packedBuffer, info.m_dataIsLE);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
      m_dataSize = CAEPackIEC61937::PackDTS_1024(data, size, m_packedBuffer, info.m_dataIsLE);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
      m_dataSize = CAEPackIEC61937::PackDTS_2048(data, size, m_packedBuffer, info.m_dataIsLE);
      break;

    default:
      CLog::Log(LOGERROR, "CAEBitstreamPacker::Pack - no pack function");
  }
}

bool CAEBitstreamPacker::PackPause(CAEStreamInfo &info, unsigned int millis, bool iecBursts)
{
  // re-use last buffer
  if (m_pauseDuration == millis)
    return false;

  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      m_dataSize = CAEPackIEC61937::PackPause(m_packedBuffer, millis, GetOutputChannelMap(info).Count() * 2, GetOutputRate(info), 4, info.m_sampleRate);
      m_pauseDuration = millis;
      break;

    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
      m_dataSize = CAEPackIEC61937::PackPause(m_packedBuffer, millis, GetOutputChannelMap(info).Count() * 2, GetOutputRate(info), 3, info.m_sampleRate);
      m_pauseDuration = millis;
      break;

    default:
      CLog::Log(LOGERROR, "CAEBitstreamPacker::Pack - no pack function");
  }

  if (!iecBursts)
  {
    memset(m_packedBuffer, 0, m_dataSize);
  }

  return true;
}

unsigned int CAEBitstreamPacker::GetSize() const
{
  return m_dataSize;
}

uint8_t* CAEBitstreamPacker::GetBuffer()
{
  return m_packedBuffer;
}

void CAEBitstreamPacker::Reset()
{
  m_dataSize = 0;
  m_pauseDuration = 0;
  m_packedBuffer[0] = 0;
}

/* we need to pack 24 TrueHD audio units into the unknown MAT format before packing into IEC61937 */
void CAEBitstreamPacker::PackTrueHD(CAEStreamInfo &info, uint8_t* data, int size)
{
  /* create the buffer if it doesn't already exist */
  if (m_trueHD[0].size() == 0)
  {
    m_trueHD[0].resize(MAT_FRAME_SIZE);
    m_trueHD[1].resize(MAT_FRAME_SIZE);
    m_thd = {};
  }

  if (size < 10)
    return;

  uint8_t* pBuf = m_trueHD[m_thd.bufferIndex].data();
  const uint8_t* pData = data;

  int totalFrameSize = size;
  int dataRem = size;
  int paddingRem = 0;
  int ratebits = 0;
  int nextCodeIdx = 0;
  uint16_t inputTiming = 0;
  bool havePacket = false;

  if (AV_RB24(data + 4) == 0xf8726f)
  {
    /* major sync unit, fetch sample rate */
    if (data[7] == 0xba)
      ratebits = data[8] >> 4;
    else if (data[7] == 0xbb)
      ratebits = data[9] >> 4;
    else
      return;

    m_thd.samplesPerFrame = 40 << (ratebits & 3);
  }

  if (!m_thd.samplesPerFrame)
    return;

  inputTiming = AV_RB16(data + 2);

  if (m_thd.prevFrameSize)
  {
    uint16_t deltaSamples = inputTiming - m_thd.prevFrameTime;
    /*
     * One multiple-of-48kHz frame is 1/1200 sec and the IEC 61937 rate
     * is 768kHz = 768000*4 bytes/sec.
     * The nominal space per frame is therefore
     * (768000*4 bytes/sec) * (1/1200 sec) = 2560 bytes.
     * For multiple-of-44.1kHz frames: 1/1102.5 sec, 705.6kHz, 2560 bytes.
     *
     * 2560 is divisible by samplesPerFrame.
     */
    int deltaBytes = deltaSamples * 2560 / m_thd.samplesPerFrame;

    /* padding needed before this frame */
    paddingRem = deltaBytes - m_thd.prevFrameSize;

    /* sanity check */
    if (paddingRem < 0 || paddingRem >= MAT_FRAME_SIZE / 2)
    {
      m_thd = {}; // recovering after seek
      return;
    }
  }

  for (nextCodeIdx = 0; nextCodeIdx < ARRAY_SIZE(MatCodes); nextCodeIdx++)
    if (m_thd.bufferFilled <= MatCodes[nextCodeIdx].pos)
      break;

  if (nextCodeIdx >= ARRAY_SIZE(MatCodes))
    return;

  while (paddingRem || dataRem || MatCodes[nextCodeIdx].pos == m_thd.bufferFilled)
  {
    if (MatCodes[nextCodeIdx].pos == m_thd.bufferFilled)
    {
      /* time to insert MAT code */
      int codeLen = MatCodes[nextCodeIdx].len;
      int codeLenRemaining = codeLen;
      memcpy(pBuf + MatCodes[nextCodeIdx].pos, MatCodes[nextCodeIdx].code, codeLen);
      m_thd.bufferFilled += codeLen;

      nextCodeIdx++;
      if (nextCodeIdx == ARRAY_SIZE(MatCodes))
      {
        nextCodeIdx = 0;

        /* this was the last code, move to the next MAT frame */
        havePacket = true;
        m_thd.outputBuffer = pBuf;
        m_thd.bufferIndex ^= 1;
        pBuf = m_trueHD[m_thd.bufferIndex].data();
        m_thd.bufferFilled = 0;

        /* inter-frame gap has to be counted as well, add it */
        codeLenRemaining += MAT_PKT_OFFSET - MAT_FRAME_SIZE;
      }

      if (paddingRem)
      {
        /* consider the MAT code as padding */
        const int countedAsPadding = std::min(paddingRem, codeLenRemaining);
        paddingRem -= countedAsPadding;
        codeLenRemaining -= countedAsPadding;
      }
      /* count the remainder of the code as part of frame size */
      if (codeLenRemaining)
        totalFrameSize += codeLenRemaining;
    }

    if (paddingRem)
    {
      const int paddingLen = std::min(MatCodes[nextCodeIdx].pos - m_thd.bufferFilled, paddingRem);

      memset(pBuf + m_thd.bufferFilled, 0, paddingLen);
      m_thd.bufferFilled += paddingLen;
      paddingRem -= paddingLen;

      if (paddingRem)
        continue; /* time to insert MAT code */
    }

    if (dataRem)
    {
      const int dataLen = std::min(MatCodes[nextCodeIdx].pos - m_thd.bufferFilled, dataRem);

      memcpy(pBuf + m_thd.bufferFilled, pData, dataLen);
      m_thd.bufferFilled += dataLen;
      pData += dataLen;
      dataRem -= dataLen;
    }
  }

  m_thd.prevFrameSize = totalFrameSize;
  m_thd.prevFrameTime = inputTiming;

  if (!havePacket)
    return;

  m_dataSize = CAEPackIEC61937::PackTrueHD(m_thd.outputBuffer, MAT_FRAME_SIZE, m_packedBuffer);
}

void CAEBitstreamPacker::PackDTSHD(CAEStreamInfo &info, uint8_t* data, int size)
{
  static const uint8_t dtshd_start_code[10] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xfe };
  unsigned int dataSize = sizeof(dtshd_start_code) + 2 + size;

  if (dataSize > m_dtsHDSize)
  {
    m_dtsHDSize = dataSize;
    m_dtsHD.resize(dataSize);
    memcpy(m_dtsHD.data(), dtshd_start_code, sizeof(dtshd_start_code));
  }

  m_dtsHD[sizeof(dtshd_start_code) + 0] = ((uint16_t)size & 0xFF00) >> 8;
  m_dtsHD[sizeof(dtshd_start_code) + 1] = ((uint16_t)size & 0x00FF);
  memcpy(m_dtsHD.data() + sizeof(dtshd_start_code) + 2, data, size);

  m_dataSize =
      CAEPackIEC61937::PackDTSHD(m_dtsHD.data(), dataSize, m_packedBuffer, info.m_dtsPeriod);
}

void CAEBitstreamPacker::PackEAC3(CAEStreamInfo &info, uint8_t* data, int size)
{
  unsigned int framesPerBurst = info.m_repeat;

  if (m_eac3FramesPerBurst != framesPerBurst)
  {
    /* switched streams, discard partial burst */
    m_eac3Size = 0;
    m_eac3FramesPerBurst = framesPerBurst;
  }

  if (m_eac3FramesPerBurst == 1)
  {
    /* simple case, just pass through */
    m_dataSize = CAEPackIEC61937::PackEAC3(data, size, m_packedBuffer);
  }
  else
  {
    /* multiple frames needed to achieve 6 blocks as required by IEC 61937-3:2007 */

    if (m_eac3.size() == 0)
      m_eac3.resize(EAC3_MAX_BURST_PAYLOAD_SIZE);

    unsigned int newsize = m_eac3Size + size;
    bool overrun = newsize > EAC3_MAX_BURST_PAYLOAD_SIZE;

    if (!overrun)
    {
      memcpy(m_eac3.data() + m_eac3Size, data, size);
      m_eac3Size = newsize;
      m_eac3FramesCount++;
    }

    if (m_eac3FramesCount >= m_eac3FramesPerBurst || overrun)
    {
      m_dataSize = CAEPackIEC61937::PackEAC3(m_eac3.data(), m_eac3Size, m_packedBuffer);
      m_eac3Size = 0;
      m_eac3FramesCount = 0;
    }
  }
}

unsigned int CAEBitstreamPacker::GetOutputRate(CAEStreamInfo &info)
{
  unsigned int rate;
  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      rate = info.m_sampleRate;
      break;
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      rate = info.m_sampleRate * 4;
      break;
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      if (info.m_sampleRate == 48000 ||
          info.m_sampleRate == 96000 ||
          info.m_sampleRate == 192000)
        rate = 192000;
      else
        rate = 176400;
      break;
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      rate = info.m_sampleRate;
      break;
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      rate = 192000;
      break;
    default:
      rate = 48000;
      break;
  }
  return rate;
}

CAEChannelInfo CAEBitstreamPacker::GetOutputChannelMap(CAEStreamInfo &info)
{
  int channels = 2;
  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      channels = 2;
      break;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      channels = 8;
      break;

    default:
      break;
  }

  CAEChannelInfo channelMap;
  for (int i=0; i<channels; i++)
  {
    channelMap += AE_CH_RAW;
  }

  return channelMap;
}
