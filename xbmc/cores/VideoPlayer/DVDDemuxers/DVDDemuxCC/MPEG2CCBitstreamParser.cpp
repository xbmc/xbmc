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

#include <cstring>

CCPictureType CMPEG2CCBitstreamParser::ParsePacket(DemuxPacket* pPacket,
                                                   std::vector<CCaptionBlock*>& tempBuffer,
                                                   std::vector<CCaptionBlock*>& reorderBuffer)
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
          ProcessGA94UserData(buf, len, pPacket->pts, picType, tempBuffer, reorderBuffer);
        }
        // Check for CC format (SCTE-20)
        else if (len >= 6 && buf[0] == 'C' && buf[1] == 'C' && buf[2] == 1)
        {
          ProcessCCUserData(buf, len, pPacket->pts, reorderBuffer);
          // CC format always uses I-frame timing
          picType = CCPictureType::I_FRAME;
        }
      }
    }

    startcode = startcode << 8 | pPacket->pData[p++];
  }

  return picType;
}

void CMPEG2CCBitstreamParser::ProcessGA94UserData(uint8_t* buf,
                                                  int len,
                                                  double pts,
                                                  CCPictureType picType,
                                                  std::vector<CCaptionBlock*>& tempBuffer,
                                                  std::vector<CCaptionBlock*>& reorderBuffer)
{
  // GA94 format: 'G' 'A' '9' '4' 0x03 flags cc_data...
  // buf[4] should be 0x03 (user_data_type_code)
  // buf[5] contains flags and cc_count
  if (buf[4] != 3 || !(buf[5] & 0x40)) // Check user_data_type_code and process_cc_data_flag
  {
    CLog::LogF(LOGDEBUG, "Invalid GA94 user_data_type_code or process_cc_data_flag not set");
    return;
  }

  int cc_count = buf[5] & 0x1f;
  if (cc_count == 0 || len < 7 + cc_count * 3)
  {
    CLog::LogF(LOGDEBUG, "Invalid cc_count ({}) or insufficient data (len={})", cc_count, len);
    return;
  }

  CCaptionBlock* cc = new CCaptionBlock(cc_count * 3);
  memcpy(cc->m_data.data(), buf + 7, cc_count * 3);
  cc->m_pts = pts;

  // Reference frames (I/P) go to temp buffer for proper ordering
  // Non-reference frames (B) go directly to reorder buffer
  if (picType == CCPictureType::I_FRAME || picType == CCPictureType::P_FRAME)
    tempBuffer.push_back(cc);
  else
    reorderBuffer.push_back(cc);
}

void CMPEG2CCBitstreamParser::ProcessCCUserData(uint8_t* buf,
                                                int len,
                                                double pts,
                                                std::vector<CCaptionBlock*>& reorderBuffer)
{
  // CC format (SCTE-20): 'C' 'C' 0x01 ...
  // buf[3] is reserved
  // buf[4] contains field information and cc_count

  int oddidx = (buf[4] & 0x80) ? 0 : 1;
  int cc_count = (buf[4] & 0x3e) >> 1;
  int extrafield = buf[4] & 0x01;

  if (extrafield)
    cc_count++;

  if (cc_count == 0 || len < 5 + cc_count * 3 * 2)
  {
    CLog::LogF(LOGDEBUG, "Invalid CC user data: cc_count={}, len={}", cc_count, len);
    return;
  }

  CCaptionBlock* cc = new CCaptionBlock(cc_count * 3);
  uint8_t* src = buf + 5;
  uint8_t* dst = cc->m_data.data();
  int bytesWritten = 0;

  for (int i = 0; i < cc_count; i++)
  {
    for (int j = 0; j < 2; j++)
    {
      if (i == cc_count - 1 && extrafield && j == 1)
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

  cc->m_data.resize(bytesWritten);

  // Only add to buffer if we actually extracted valid CC data
  if (bytesWritten > 0)
  {
    cc->m_pts = pts;
    reorderBuffer.push_back(cc);
  }
  else
  {
    delete cc;
  }
}
