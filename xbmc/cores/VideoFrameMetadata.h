/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"

#include <cstdint>
#include <string_view>

/*!
 * \brief Dolby Vision enhancement layer type, derived from the RPU data header.
 */
enum class DoviElType
{
  NONE, ///< The frame carries no enhancement layer
  MEL, ///< Minimal enhancement layer
  FEL, ///< Full enhancement layer
};

/*!
 * \brief Get the display representation of a Dolby Vision enhancement layer type.
 * \param elType The enhancement layer type
 * \return "MEL", "FEL", or an empty string when there is no enhancement layer
 */
constexpr std::string_view DoviElTypeToString(DoviElType elType)
{
  switch (elType)
  {
    case DoviElType::MEL:
      return "MEL";
    case DoviElType::FEL:
      return "FEL";
    default:
      return "";
  }
}

/*!
 * \brief Dolby Vision RPU metadata of a single video frame.
 *
 * Flattened from AV_FRAME_DATA_DOVI_METADATA. AVDOVIMetadata is a variable
 * length allocation addressed by internal byte offsets, so it cannot be value
 * copied; the values the player exposes are extracted into this trivially
 * copyable record instead.
 */
struct DoviFrameMetadata
{
  bool valid{false}; ///< True when the frame carried a Dolby Vision RPU
  DoviElType elType{DoviElType::NONE};

  bool hasLevel1{false}; ///< True when the RPU carried a level 1 extension block
  uint16_t level1MinPq{0}; ///< Per frame minimum brightness, 12 bit PQ encoded [0, 4095]
  uint16_t level1MaxPq{0}; ///< Per frame maximum brightness, 12 bit PQ encoded [0, 4095]
  uint16_t level1AvgPq{0}; ///< Per frame average brightness, 12 bit PQ encoded [0, 4095]

  bool hasLevel5{false}; ///< True when the RPU carried a level 5 extension block
  uint16_t level5LeftOffset{0}; ///< Active area left offset in pixels
  uint16_t level5RightOffset{0}; ///< Active area right offset in pixels
  uint16_t level5TopOffset{0}; ///< Active area top offset in pixels
  uint16_t level5BottomOffset{0}; ///< Active area bottom offset in pixels

  bool hasLevel6{false}; ///< True when the RPU carried a level 6 extension block
  uint16_t level6MaxCll{0}; ///< Maximum content light level in cd/m2
  uint16_t level6MaxFall{0}; ///< Maximum frame average light level in cd/m2

  bool operator==(const DoviFrameMetadata&) const = default;
};

/*!
 * \brief Per frame video metadata published for the frame currently on screen.
 */
struct VideoFrameMetadata
{
  StreamHdrType hdrType{StreamHdrType::HDR_TYPE_NONE}; ///< HDR type of the presented frame
  DoviFrameMetadata dovi;

  bool operator==(const VideoFrameMetadata&) const = default;
};
