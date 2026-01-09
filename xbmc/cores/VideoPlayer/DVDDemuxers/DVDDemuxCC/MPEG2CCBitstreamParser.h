/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ICCBitstreamParser.h"

/*!
 * \brief MPEG2 closed caption parser
 *
 * Extracts CEA-608/708 closed caption data from MPEG2 video streams.
 * Searches for:
 * - Picture headers (start code 0x00) to determine picture type (I/P/B)
 * - User data (start code 0xb2) containing GA94 or CC closed caption payloads
 */
class CMPEG2CCBitstreamParser : public ICCBitstreamParser
{
public:
  CMPEG2CCBitstreamParser() = default;
  ~CMPEG2CCBitstreamParser() override = default;

  /*!
   * \brief Parse MPEG2 packet for closed caption data
   *
   * Scans the packet for MPEG2 start codes and extracts CC data from user data sections.
   * Supports both GA94 and CC data formats.
   *
   * \param pPacket MPEG2 video packet with Annex B start codes
   * \param tempBuffer Temporary buffer for CC data from reference frames
   * \param reorderBuffer Reorder buffer for CC data
   * \return Picture type (I_FRAME, P_FRAME, or OTHER)
   */
  CCPictureType ParsePacket(DemuxPacket* pPacket,
                            std::vector<CCaptionBlock*>& tempBuffer,
                            std::vector<CCaptionBlock*>& reorderBuffer) override;

  const char* GetName() const override { return "MPEG2CCBitstreamParser"; }

private:
  /*!
   * \brief Process GA94 format user data
   *
   * GA94 format: 'G''A''9''4' 0x03 flags cc_data...
   * Extracts cc_count and cc_data triplets.
   */
  void ProcessGA94UserData(uint8_t* buf,
                           int len,
                           double pts,
                           CCPictureType picType,
                           std::vector<CCaptionBlock*>& tempBuffer,
                           std::vector<CCaptionBlock*>& reorderBuffer);

  /*!
   * \brief Process CC format user data
   *
   * CC format: 'C''C' 0x01 ... (SCTE-20 format)
   * Used by some broadcast systems, extracts field-based CC data.
   */
  void ProcessCCUserData(uint8_t* buf,
                         int len,
                         double pts,
                         std::vector<CCaptionBlock*>& reorderBuffer);
};
