/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MPEG2CCBitstreamParser.h"

#include "CaptionBlock.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/log.h"

#include <algorithm>

CCPictureType CMPEG2CCBitstreamParser::ParsePacket(DemuxPacket* pPacket,
                                                   std::vector<CCaptionBlock>& tempBuffer,
                                                   std::vector<CCaptionBlock>& reorderBuffer)
{
  CCPictureType picType = CCPictureType::OTHER;
  uint32_t startcode = 0xffffffff;
  int p = 0;
  int len;

  // Scan packet for MPEG2 start codes
  while ((len = pPacket->iSize - p) > 3)
  {
    if ((startcode & 0xffffff00) == 0x00000100)
    {
      int scode = startcode & 0xFF;

      // Picture header (0x00) - determine picture type
      if (scode == 0x00)
      {
        if (len > 4)
        {
          uint8_t* buf = pPacket->pData + p;
          int mpegPicType = (buf[1] & 0x38) >> 3;

          // Convert MPEG2 picture type to CCPictureType
          // MPEG2: 1=I-frame, 2=P-frame, 3=B-frame
          if (mpegPicType == 1)
            picType = CCPictureType::I_FRAME;
          else if (mpegPicType == 2)
            picType = CCPictureType::P_FRAME;
          else
            picType = CCPictureType::OTHER;
        }
      }
      // User data (0xb2) - extract closed caption data
      else if (scode == 0xb2)
      {
        uint8_t* buf = pPacket->pData + p;

        // Check for GA94 format
        if (len >= 6 && buf[0] == 'G' && buf[1] == 'A' && buf[2] == '9' && buf[3] == '4')
        {
          ProcessGA94UserData(std::span(buf, len), pPacket->pts, picType, tempBuffer,
                              reorderBuffer);
        }
        // Check for CC format (SCTE-20)
        else if (len >= 6 && buf[0] == 'C' && buf[1] == 'C' && buf[2] == 1)
        {
          ProcessCCUserData(std::span(buf, len), pPacket->pts, reorderBuffer);
          // CC format always uses I-frame timing
          picType = CCPictureType::I_FRAME;
        }
      }
    }

    startcode = startcode << 8 | pPacket->pData[p++];
  }

  return picType;
}

void CMPEG2CCBitstreamParser::ProcessGA94UserData(std::span<const uint8_t> buf,
                                                  double pts,
                                                  CCPictureType picType,
                                                  std::vector<CCaptionBlock>& tempBuffer,
                                                  std::vector<CCaptionBlock>& reorderBuffer)
{
  // GA94 format: 'G' 'A' '9' '4' 0x03 flags ccData...
  // buf[4] should be 0x03 (user_data_type_code)
  // buf[5] contains flags and ccCount
  if (buf[4] != 3 || !(buf[5] & 0x40)) // Check user_data_type_code and process_ccData_flag
  {
    CLog::LogF(LOGDEBUG, "Invalid GA94 user_data_type_code or process_ccData_flag not set");
    return;
  }

  unsigned ccCount = buf[5] & 0x1f;
  if (ccCount == 0 || buf.size() < 7 + ccCount * 3)
  {
    CLog::LogF(LOGDEBUG, "Invalid ccCount ({}) or insufficient data (len={})", ccCount, buf.size());
    return;
  }

  // Reference frames (I/P) go to temp buffer for proper ordering
  // Non-reference frames (B) go directly to reorder buffer
  if (picType == CCPictureType::I_FRAME || picType == CCPictureType::P_FRAME)
  {
    CCaptionBlock& cb = tempBuffer.emplace_back(ccCount * 3);
    std::copy_n(buf.begin() + 7, ccCount * 3, cb.m_data.data());
    cb.m_pts = pts;
  }
  else
  {
    CCaptionBlock& cb = reorderBuffer.emplace_back(ccCount * 3);
    std::copy_n(buf.data() + 7, ccCount * 3, cb.m_data.data());
    cb.m_pts = pts;
  }
}

void CMPEG2CCBitstreamParser::ProcessCCUserData(std::span<const uint8_t> buf,
                                                double pts,
                                                std::vector<CCaptionBlock>& reorderBuffer)
{
  // CC format (SCTE-20): 'C' 'C' 0x01 ...
  // buf[3] is reserved
  // buf[4] contains field information and ccCount

  int oddidx = (buf[4] & 0x80) ? 0 : 1;
  unsigned ccCount = (buf[4] & 0x3e) >> 1;
  int extrafield = buf[4] & 0x01;

  if (extrafield)
    ccCount++;

  if (ccCount == 0 || buf.size() < 5 + ccCount * 3 * 2)
  {
    CLog::LogF(LOGDEBUG, "Invalid CC user data: ccCount={}, len={}", ccCount, buf.size());
    return;
  }

  reorderBuffer.emplace_back(ccCount * 3);
  const uint8_t* src = buf.data() + 5;
  uint8_t* dst = reorderBuffer.back().m_data.data();
  int bytesWritten = 0;

  for (size_t i = 0; i < ccCount; i++)
  {
    for (int j = 0; j < 2; j++)
    {
      if (i == ccCount - 1 && extrafield && j == 1)
        break;

      // Check if this is valid CC data for the current field
      if ((oddidx == j) && (src[0] == 0xFF))
      {
        dst[0] = 0x04; // CEA-608 field indicator
        dst[1] = src[1];
        dst[2] = src[2];
        dst += 3;
        bytesWritten += 3;
      }
      src += 3;
    }
  }

  reorderBuffer.back().m_data.resize(bytesWritten);

  // Only keep in buffer if we actually extracted valid CC data
  if (bytesWritten > 0)
  {
    reorderBuffer.back().m_pts = pts;
  }
  else
  {
    reorderBuffer.pop_back();
  }
}
