/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "H264AnnexBBitstreamParser.h"

#include "CaptionBlock.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/log.h"

#include <iterator>
#include <ranges>

CCPictureType CH264AnnexBBitstreamParser::ParsePacket(DemuxPacket* pPacket,
                                                      std::vector<CCaptionBlock>& tempBuffer,
                                                      std::vector<CCaptionBlock>& reorderBuffer)
{
  CCPictureType picType = CCPictureType::OTHER;
  uint32_t startcode = 0xffffffff;
  int p = 0;
  int len;

  // Scan packet for Annex B start codes (0x000001 or 0x00000001)
  while ((len = pPacket->iSize - p) > 3)
  {
    // Check for start code in the form 0x000001xx
    if ((startcode & 0xffffff00) == 0x00000100)
    {
      // Extract NAL unit type (lower 5 bits) while masking out nal_ref_idc (bits 6-5)
      // Mask 0x9F = 0b10011111 keeps forbidden_zero_bit (bit 7) and nal_unit_type (bits 4-0)
      int scode = startcode & 0x9F;

      // Slice NAL units (types 1-5) - determine picture type
      if (scode >= 1 && scode <= 5)
      {
        uint8_t* buf = pPacket->pData + p;

        if (len > 1)
        {
          CCPictureType slicePicType = DetectSliceType(buf, len);

          // If parsing failed due to corrupted Golomb codes, mark entire packet as invalid
          if (slicePicType == CCPictureType::INVALID)
          {
            CLog::LogF(LOGDEBUG, "Corrupted slice header detected, marking packet as invalid");
            // Flush any CC data from tempBuffer to reorderBuffer before returning
            std::ranges::move(std::views::reverse(tempBuffer), std::back_inserter(reorderBuffer));
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
            std::ranges::move(std::views::reverse(tempBuffer), std::back_inserter(reorderBuffer));
          }
        }
      }
      // SEI NAL unit (type 6) - extract closed caption data
      else if (scode == 0x06)
      {
        uint8_t* buf = pPacket->pData + p;

        // Simplified check for GA94 closed caption data in SEI
        // Search for GA94 marker and pass to ProcessSEIPayload for full validation
        // ProcessSEIPayload handles both ITU-T T.35 format and direct GA94 format
        for (int i = 0; i <= len - 8; i++)
        {
          if (buf[i] == 'G' && buf[i + 1] == 'A' && buf[i + 2] == '9' && buf[i + 3] == '4' &&
              buf[i + 4] == 3)
          {
            // Pass from start of potential ITU-T T.35 prefix (3 bytes before GA94)
            // or from GA94 itself if no prefix exists
            int startOffset = (i >= 3) ? i - 3 : i;
            ProcessSEIPayload(buf + startOffset, len - startOffset, pPacket->pts, tempBuffer);
            break;
          }
        }
      }
    }

    startcode = startcode << 8 | pPacket->pData[p++];
  }

  return picType;
}
