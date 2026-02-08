/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "BitstreamReader.h"
#include "HDR10Plus.h"

struct DisplayPrimary {
  uint16_t x;
  uint16_t y;
};

struct MasteringDisplayColourVolume {
  DisplayPrimary displayPrimaries[3];  // R, G, B
  DisplayPrimary whitePoint;
  uint32_t maxLuminance;
  uint32_t minLuminance;
};

struct ContentLightLevel {
  uint16_t maxContentLightLevel;
  uint16_t maxFrameAverageLightLevel;
};

void HevcAddStartCodeEmulationPrevention3Byte(std::vector<uint8_t>& buf);
void HevcClearStartCodeEmulationPrevention3Byte(const uint8_t* buf,
                                                const size_t len,
                                                std::vector<uint8_t>& out);

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

  // Returns a HDR10+ SEI message if present in the list
  static std::optional<const CHevcSei*> FindHdr10PlusSeiMessage(
      const std::vector<uint8_t>& buf, const std::vector<CHevcSei>& messages);

  // Returns a pair with:
  //   1) a bool for whether or not the NALU SEI payload contains a HDR10+ SEI message.
  //   2) a vector of bytes:
  //      When not empty: the new NALU containing all but the HDR10+ SEI message.
  //      Otherwise: the NALU contained only one HDR10+ SEI and can be discarded.
  static const std::vector<uint8_t> RemoveHdr10PlusFromSeiNalu(
      const uint8_t* inData, const size_t inDataLen);

  static const std::optional<const Hdr10PlusMetadata> ExtractHdr10Plus(
    const std::vector<CHevcSei>& messages,
    const std::vector<uint8_t>& buf);

  static const std::optional<MasteringDisplayColourVolume> ExtractMasteringDisplayColourVolume(
    const std::vector<CHevcSei>& messages,
    const std::vector<uint8_t>& buf);

  static const std::optional<ContentLightLevel> ExtractContentLightLevel(
    const std::vector<CHevcSei>& messages,
    const std::vector<uint8_t>& buf);

private:
  // Parses single SEI message from the reader and pushes it to the list
  static int ParseSeiMessage(CBitstreamReader& br, std::vector<CHevcSei>& messages);

  static std::vector<CHevcSei> ParseSeiRbspInternal(const uint8_t* buf, const size_t len);
};
