/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "H264AVCCBitstreamParser.h"

#include "CaptionBlock.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/log.h"

#include <climits>

CCPictureType CH264AVCCBitstreamParser::ParsePacket(DemuxPacket* pPacket,
                                                    std::vector<CCaptionBlock>& tempBuffer,
                                                    std::vector<CCaptionBlock>& reorderBuffer)
{
  CCPictureType picType = CCPictureType::OTHER;
  int p = 0;

  // Iterate through length-prefixed NAL units
  while (pPacket->iSize >= 4 && p <= pPacket->iSize - 4)
  {
    // Read 4-byte big-endian length prefix
    uint32_t nalSize = (static_cast<uint32_t>(pPacket->pData[p]) << 24) |
                       (static_cast<uint32_t>(pPacket->pData[p + 1]) << 16) |
                       (static_cast<uint32_t>(pPacket->pData[p + 2]) << 8) |
                       static_cast<uint32_t>(pPacket->pData[p + 3]);
    p += 4;

    // Check if we have enough data for this NAL unit
    if (nalSize == 0 || nalSize > INT_MAX || nalSize > static_cast<uint32_t>(pPacket->iSize - p))
    {
      CLog::LogF(LOGDEBUG, "Invalid NAL size {} at position {}, packet size {}", nalSize, p - 4,
                 pPacket->iSize);
      break;
    }

    // Get NAL unit type from lower 5 bits of NAL header
    uint8_t nalType = pPacket->pData[p] & 0x1F;

    // Process slice NAL units (types 1-5) to determine picture type
    if (nalType >= 1 && nalType <= 5)
    {
      uint8_t* buf = pPacket->pData + p;
      int len = nalSize;

      if (len > 1) // Need at least NAL header + RBSP data
      {
        CCPictureType slicePicType = DetectSliceType(buf, len);

        // If parsing failed due to corrupted Golomb codes, mark entire packet as invalid
        if (slicePicType == CCPictureType::INVALID)
        {
          CLog::LogF(LOGDEBUG, "Corrupted slice header detected, marking packet as invalid");
          // Flush any CC data from tempBuffer to reorderBuffer before returning
          while (!tempBuffer.empty())
          {
            reorderBuffer.push_back(std::move(tempBuffer.back()));
            tempBuffer.pop_back();
          }
          return CCPictureType::INVALID;
        }

        // Update picture type (prefer I-frame > P-frame > OTHER)
        if (slicePicType == CCPictureType::I_FRAME)
          picType = CCPictureType::I_FRAME;
        else if (slicePicType == CCPictureType::P_FRAME && picType == CCPictureType::OTHER)
          picType = CCPictureType::P_FRAME;

        // If this is a B-frame, move CC data from temp to reorder buffer
        if (picType == CCPictureType::OTHER)
        {
          while (!tempBuffer.empty())
          {
            reorderBuffer.push_back(std::move(tempBuffer.back()));
            tempBuffer.pop_back();
          }
        }
      }
    }
    // Process SEI NAL unit (type 6) for closed caption data
    else if (nalType == 6)
    {
      uint8_t* buf = pPacket->pData + p;
      int len = nalSize;
      ParseSEINALUnit(buf, len, pPacket->pts, tempBuffer);
    }

    p += nalSize;
  }

  return picType;
}

void CH264AVCCBitstreamParser::ParseSEINALUnit(uint8_t* buf,
                                               int len,
                                               double pts,
                                               std::vector<CCaptionBlock>& tempBuffer)
{
  // SEI payload structure: [payload_type] [payload_size] [payload_data] [repeat...]
  // payload_type and payload_size use 0xFF for values >= 255

  int seiPos = 1; // Skip NAL header byte

  while (seiPos < len)
  {
    // Read payload type (supports extended values with 0xFF bytes)
    int payloadType = 0;
    while (seiPos < len && buf[seiPos] == 0xFF)
    {
      payloadType += 255;
      seiPos++;
    }
    if (seiPos < len)
      payloadType += buf[seiPos++];
    else
      break;

    // Read payload size (supports extended values with 0xFF bytes)
    int payloadSize = 0;
    while (seiPos < len && buf[seiPos] == 0xFF)
    {
      payloadSize += 255;
      seiPos++;
    }
    if (seiPos < len)
      payloadSize += buf[seiPos++];
    else
      break;

    // Check if this is user_data_registered_itu_t_t35 (type 4) with GA94 CC data
    if (payloadType == 4 && seiPos + payloadSize <= len)
    {
      uint8_t* userdata = buf + seiPos;
      ProcessSEIPayload(userdata, payloadSize, pts, tempBuffer);
    }

    seiPos += payloadSize;
  }
}
