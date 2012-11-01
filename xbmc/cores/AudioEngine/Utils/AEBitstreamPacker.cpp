/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#define BURST_HEADER_SIZE       8
#define TRUEHD_FRAME_OFFSET     2560
#define MAT_MIDDLE_CODE_OFFSET -4
#define MAT_FRAME_SIZE          61424

CAEBitstreamPacker::CAEBitstreamPacker() :
  m_trueHD   (NULL),
  m_trueHDPos(0),
  m_dtsHD    (NULL),
  m_dtsHDSize(0),
  m_dataSize (0)
{
}

CAEBitstreamPacker::~CAEBitstreamPacker()
{
  delete[] m_trueHD;
  delete[] m_dtsHD;
}

void CAEBitstreamPacker::Pack(CAEStreamInfo &info, uint8_t* data, int size)
{
  switch (info.GetDataType())
  {
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      PackTrueHD(info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      PackDTSHD (info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
      m_dataSize = CAEPackIEC61937::PackDTS_512(data, size, m_packedBuffer, info.IsLittleEndian());
      break;

    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
      m_dataSize = CAEPackIEC61937::PackDTS_1024(data, size, m_packedBuffer, info.IsLittleEndian());
      break;

    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
      m_dataSize = CAEPackIEC61937::PackDTS_2048(data, size, m_packedBuffer, info.IsLittleEndian());
      break;

    default:
      /* pack the data into an IEC61937 frame */
      CAEPackIEC61937::PackFunc pack = info.GetPackFunc();
      if (pack)
        m_dataSize = pack(data, size, m_packedBuffer);
  }
}

unsigned int CAEBitstreamPacker::GetSize()
{
  return m_dataSize;
}

uint8_t* CAEBitstreamPacker::GetBuffer()
{
  m_dataSize = 0;
  return m_packedBuffer;
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

  m_dataSize = CAEPackIEC61937::PackDTSHD(m_dtsHD, dataSize, m_packedBuffer, info.GetDTSPeriod());
}

