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

#include "AEBitstreamPacker.h"
#include "AEPackIEC61937.h"
#include "AEStreamInfo.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "utils/log.h"

#define BURST_HEADER_SIZE       8
#define TRUEHD_FRAME_OFFSET     2560
#define MAT_MIDDLE_CODE_OFFSET -4
#define MAT_FRAME_SIZE          61424
#define EAC3_MAX_BURST_PAYLOAD_SIZE (24576 - BURST_HEADER_SIZE)

CAEBitstreamPacker::CAEBitstreamPacker() :
  m_trueHD   (NULL),
  m_trueHDPos(0),
  m_dtsHD    (NULL),
  m_dtsHDSize(0),
  m_eac3     (NULL),
  m_eac3Size (0),
  m_eac3FramesCount(0),
  m_eac3FramesPerBurst(0),
  m_dataSize (0),
  m_pauseDuration(0)
{
  Reset();
}

CAEBitstreamPacker::~CAEBitstreamPacker()
{
  delete[] m_trueHD;
  delete[] m_dtsHD;
  delete[] m_eac3;
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

void CAEBitstreamPacker::PackPause(CAEStreamInfo &info, unsigned int millis, bool iecBursts)
{
  // re-use last buffer
  if (m_pauseDuration == millis)
    return;

  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      m_dataSize = CAEPackIEC61937::PackPause(m_packedBuffer, millis, GetOutputChannelMap(info).Count() * 2, GetOutputRate(info), 4, info.m_sampleRate);
      m_pauseDuration = millis;
      break;

    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
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
}

unsigned int CAEBitstreamPacker::GetSize()
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
  m_trueHDPos = 0;
  m_pauseDuration = 0;
  m_packedBuffer[0] = 0;
}

/* we need to pack 24 TrueHD audio units into the unknown MAT format before packing into IEC61937 */
void CAEBitstreamPacker::PackTrueHD(CAEStreamInfo &info, uint8_t* data, int size)
{
  /* magic MAT format values, meaning is unknown at this point */
  static const uint8_t mat_start_code [20] = { 0x07, 0x9E, 0x00, 0x03, 0x84, 0x01, 0x01, 0x01, 0x80, 0x00, 0x56, 0xA5, 0x3B, 0xF4, 0x81, 0x83, 0x49, 0x80, 0x77, 0xE0 };
  static const uint8_t mat_middle_code[12] = { 0xC3, 0xC1, 0x42, 0x49, 0x3B, 0xFA, 0x82, 0x83, 0x49, 0x80, 0x77, 0xE0 };
  static const uint8_t mat_end_code   [16] = { 0xC3, 0xC2, 0xC0, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x11 };

  /* create the buffer if it doesnt already exist */
  if (!m_trueHD)
  {
    m_trueHD    = new uint8_t[MAT_FRAME_SIZE];
    m_trueHDPos = 0;
  }

  /* setup the frame for the data */
  if (m_trueHDPos == 0)
  {
    memset(m_trueHD, 0, MAT_FRAME_SIZE);
    memcpy(m_trueHD, mat_start_code, sizeof(mat_start_code));
    memcpy(m_trueHD + (12 * TRUEHD_FRAME_OFFSET) - BURST_HEADER_SIZE + MAT_MIDDLE_CODE_OFFSET, mat_middle_code, sizeof(mat_middle_code));
    memcpy(m_trueHD + MAT_FRAME_SIZE - sizeof(mat_end_code), mat_end_code, sizeof(mat_end_code));
  }

  size_t offset;
  if (m_trueHDPos == 0 )
    offset = (m_trueHDPos * TRUEHD_FRAME_OFFSET) + sizeof(mat_start_code);
  else if (m_trueHDPos == 12)
    offset = (m_trueHDPos * TRUEHD_FRAME_OFFSET) + sizeof(mat_middle_code) - BURST_HEADER_SIZE + MAT_MIDDLE_CODE_OFFSET;
  else
    offset = (m_trueHDPos * TRUEHD_FRAME_OFFSET) - BURST_HEADER_SIZE;

  memcpy(m_trueHD + offset, data, size);

  /* if we have a full frame */
  if (++m_trueHDPos == 24)
  {
    m_trueHDPos = 0;
    m_dataSize  = CAEPackIEC61937::PackTrueHD(m_trueHD, MAT_FRAME_SIZE, m_packedBuffer);
  }
}

void CAEBitstreamPacker::PackDTSHD(CAEStreamInfo &info, uint8_t* data, int size)
{
  static const uint8_t dtshd_start_code[10] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xfe };
  unsigned int dataSize = sizeof(dtshd_start_code) + 2 + size;

  if (dataSize > m_dtsHDSize)
  {
    delete[] m_dtsHD;
    m_dtsHDSize = dataSize;
    m_dtsHD     = new uint8_t[dataSize];
    memcpy(m_dtsHD, dtshd_start_code, sizeof(dtshd_start_code));
  }

  m_dtsHD[sizeof(dtshd_start_code) + 0] = ((uint16_t)size & 0xFF00) >> 8;
  m_dtsHD[sizeof(dtshd_start_code) + 1] = ((uint16_t)size & 0x00FF);
  memcpy(m_dtsHD + sizeof(dtshd_start_code) + 2, data, size);

  m_dataSize = CAEPackIEC61937::PackDTSHD(m_dtsHD, dataSize, m_packedBuffer, info.m_dtsPeriod);
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

    if (m_eac3 == NULL)
      m_eac3 = new uint8_t[EAC3_MAX_BURST_PAYLOAD_SIZE];

    unsigned int newsize = m_eac3Size + size;
    bool overrun = newsize > EAC3_MAX_BURST_PAYLOAD_SIZE;

    if (!overrun)
    {
      memcpy(m_eac3 + m_eac3Size, data, size);
      m_eac3Size = newsize;
      m_eac3FramesCount++;
    }

    if (m_eac3FramesCount >= m_eac3FramesPerBurst || overrun)
    {
      m_dataSize = CAEPackIEC61937::PackEAC3(m_eac3, m_eac3Size, m_packedBuffer);
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
      channels = 2;
      break;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
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
