/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <vector>

class CCaptionBlock;
struct DemuxPacket;

/*!
 * \brief H.264/MPEG2 bitstream format for NAL unit framing
 */
enum class CCBitstreamFormat
{
  ANNEXB, //!< Annex B format with start codes (0x000001 or 0x00000001)
  AVCC //!< AVCC format with length-prefixed NAL units (MP4/MKV)
};

/*!
 * \brief Picture type for determining when to decode closed caption data
 *
 * CC data is decoded on reference frames (I/P) to ensure correct temporal
 * ordering, as B-frames may be reordered during video decoding.
 */
enum class CCPictureType
{
  INVALID = -1, //!< Corrupted/malformed frame - parsing error detected
  OTHER = 0, //!< B-frame or frame type that doesn't require CC processing
  I_FRAME = 1, //!< I-frame (Intra-coded picture) - keyframe
  P_FRAME = 2 //!< P-frame (Predicted picture) - forward prediction
};

/*!
 * \brief Base interface for codec-specific closed caption bitstream parsers
 *
 * Implementations extract CEA-608/708 closed caption data from video packets.
 * Each codec (MPEG2, H.264 AVCC, H.264 Annex B) has its own parsing logic.
 */
class ICCBitstreamParser
{
public:
  virtual ~ICCBitstreamParser() = default;

  /*!
   * \brief Parse a video packet and extract closed caption data
   *
   * \param pPacket Video packet to parse (contains raw bitstream data)
   * \param tempBuffer Temporary buffer for CC data (used for reordering)
   * \param reorderBuffer Reorder buffer for CC data (sorted by PTS)
   * \return Picture type of the frame, used to determine when to decode CC
   *
   * Implementations search for SEI (H.264) or user data (MPEG2) NAL units
   * containing GA94 closed caption payloads.
   */
  virtual CCPictureType ParsePacket(DemuxPacket* pPacket,
                                    std::vector<CCaptionBlock*>& tempBuffer,
                                    std::vector<CCaptionBlock*>& reorderBuffer) = 0;

  /*!
   * \brief Get parser name for debugging/logging
   * \return Human-readable parser name
   */
  virtual const char* GetName() const = 0;
};
