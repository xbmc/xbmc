/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "H264CCBitstreamParser.h"

/*!
 * \brief H.264 Annex B format closed caption parser
 *
 * Parses H.264 streams in Annex B format used by MPEG-TS and broadcast streams.
 * Annex B uses start codes to delimit NAL units:
 * - 0x000001 (3-byte start code) or 0x00000001 (4-byte start code)
 * - NAL unit data until next start code
 *
 * Extracts CC data from SEI NAL units (type 6) with payload type 4.
 */
class CH264AnnexBBitstreamParser : public CH264CCBitstreamParser
{
public:
  CH264AnnexBBitstreamParser() = default;
  ~CH264AnnexBBitstreamParser() override = default;

  /*!
   * \brief Parse Annex B packet for closed caption data
   *
   * Scans for start codes, extracts CC data from SEI NAL units, and
   * detects picture type from slice headers.
   *
   * \param pPacket H.264 video packet in Annex B format
   * \param tempBuffer Temporary buffer for CC data from reference frames
   * \param reorderBuffer Reorder buffer for CC data
   * \return Picture type (I_FRAME, P_FRAME, or OTHER)
   */
  CCPictureType ParsePacket(DemuxPacket* pPacket,
                            std::vector<CCaptionBlock>& tempBuffer,
                            std::vector<CCaptionBlock>& reorderBuffer) override;

  const char* GetName() const override { return "H264AnnexBBitstreamParser"; }
};
