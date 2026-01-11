/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "H264CCBitstreamParser.h"

#include "Bitstream.h"
#include "CaptionBlock.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/log.h"

#include <algorithm>

void CH264CCBitstreamParser::ProcessSEIPayload(uint8_t* buf,
                                               int len,
                                               double pts,
                                               std::vector<CCaptionBlock>& tempBuffer)
{
  if (len < 8)
  {
    CLog::LogF(LOGDEBUG, "SEI payload too small ({})", len);
    return;
  }

  // SEI user data may have ITU-T T.35 prefix or direct GA94 format
  // ITU-T T.35 format: country_code(1 byte) + provider_code(2 bytes) + user_data
  // Example: B5 00 31 47 41 39 34 03 ... (0xB5=USA, 0x0031=ATSC, "GA94"...)
  //
  // Direct GA94 format: 47 41 39 34 03 ... ("GA94"...)

  int gaOffset = -1;

  // Check for ITU-T T.35 format with GA94 at offset 3
  if (len >= 11 && buf[3] == 'G' && buf[4] == 'A' && buf[5] == '9' && buf[6] == '4' && buf[7] == 3)
  {
    gaOffset = 3;
  }
  // Check for direct GA94 format at offset 0
  else if (len >= 8 && buf[0] == 'G' && buf[1] == 'A' && buf[2] == '9' && buf[3] == '4' &&
           buf[4] == 3)
  {
    gaOffset = 0;
  }

  if (gaOffset == -1)
  {
    CLog::LogF(LOGDEBUG, "GA94 marker not found in SEI payload");
    return;
  }

  // GA94 format: 'G' 'A' '9' '4' 0x03 flags cc_data...
  uint8_t* userdata = buf + gaOffset;
  int payloadSize = len - gaOffset;

  // Check process_cc_data_flag (bit 6 of flags byte)
  if (payloadSize < 5 || !(userdata[5] & 0x40))
  {
    CLog::LogF(LOGDEBUG, "Invalid GA94 flags or payload too small");
    return;
  }

  int cc_count = userdata[5] & 0x1f;
  if (cc_count == 0 || payloadSize < 7 + cc_count * 3)
  {
    CLog::LogF(LOGDEBUG, "Invalid cc_count ({}) or insufficient data", cc_count);
    return;
  }

  tempBuffer.emplace_back(cc_count * 3);
  std::copy(userdata + 7, userdata + 7 + cc_count * 3, tempBuffer.back().m_data.data());
  tempBuffer.back().m_pts = pts;
}

CCPictureType CH264CCBitstreamParser::DetectSliceType(uint8_t* buf, int len)
{
  if (len < 2)
  {
    CLog::LogF(LOGDEBUG, "Slice too small ({})", len);
    return CCPictureType::INVALID;
  }

  // Skip NAL header byte - RBSP data starts at buf+1
  CBitstream bs(buf + 1, (len - 1) * 8);

  bs.readGolombUE(); // first_mb_in_slice
  if (bs.hasError())
  {
    CLog::LogF(LOGDEBUG, "Failed to read first_mb_in_slice");
    return CCPictureType::INVALID;
  }

  int sliceType = bs.readGolombUE(); // slice_type
  if (bs.hasError())
  {
    CLog::LogF(LOGDEBUG, "Failed to read slice_type");
    return CCPictureType::INVALID;
  }

  // H.264 slice types:
  // 0,5 = P (predicted)
  // 1,6 = B (bi-directional)
  // 2,7 = I (intra)
  // 3,8 = SP
  // 4,9 = SI

  if (sliceType == 2 || sliceType == 7) // I slice
    return CCPictureType::I_FRAME;
  else if (sliceType == 0 || sliceType == 5) // P slice
    return CCPictureType::P_FRAME;

  return CCPictureType::OTHER;
}
