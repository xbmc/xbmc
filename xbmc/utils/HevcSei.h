/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BitstreamReader.h"

#include <optional>
#include <vector>

/*!
 * \brief Parses HEVC SEI messages for supplemental video information.
 *
 * The CHevcSei class is used to interpret and handle Supplemental Enhancement
 * Information (SEI) messages found in High Efficiency Video Coding (HEVC)
 * bitstreams. It is particularly useful for extracting HDR10+ metadata and
 * other types of supplemental data from HEVC encoded video streams.
 *
 * \note This class deals with SEI messages in HEVC streams and does not
 * process the video content itself.
 */
class CHevcSei
{
public:
  CHevcSei() = default;
  ~CHevcSei() = default;

  uint8_t m_payloadType{0};
  size_t m_payloadSize{0};

  // In relation to the input SEI rbsp payload
  size_t m_msgOffset{0};
  size_t m_payloadOffset{0};

  // Parses SEI payload assumed to not have emulation prevention 3 bytes
  static std::vector<CHevcSei> ParseSeiRbsp(const uint8_t* buf, const size_t len);

  // Clears emulation prevention 3 bytes and fills in the passed buf
  static std::vector<CHevcSei> ParseSeiRbspUnclearedEmulation(const uint8_t* inData,
                                                              const size_t inDataLen,
                                                              std::vector<uint8_t>& buf);

private:
  // Parses single SEI message from the reader and pushes it to the list
  static int ParseSeiMessage(CBitstreamReader& br, std::vector<CHevcSei>& messages);

  static std::vector<CHevcSei> ParseSeiRbspInternal(const uint8_t* buf, const size_t len);
};
