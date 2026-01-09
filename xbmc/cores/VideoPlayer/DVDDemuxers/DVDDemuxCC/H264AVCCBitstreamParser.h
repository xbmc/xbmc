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
 * \brief H.264 AVCC format closed caption parser
 *
 * Parses H.264 streams in AVCC (AVC1) format used by MP4/MKV containers.
 * AVCC uses length-prefixed NAL units:
 * - 4-byte big-endian length prefix
 * - NAL unit data (length bytes)
 * - Repeat for each NAL unit
 *
 * Extracts CC data from SEI NAL units (type 6) with payload type 4.
 */
class CH264AVCCBitstreamParser : public CH264CCBitstreamParser
{
public:
  CH264AVCCBitstreamParser() = default;
  ~CH264AVCCBitstreamParser() override = default;

  /*!
   * \brief Parse AVCC packet for closed caption data
   *
   * Iterates through length-prefixed NAL units, extracting CC data from
   * SEI payloads and detecting picture type from slice headers.
   *
   * \param pPacket H.264 video packet in AVCC format
   * \param tempBuffer Temporary buffer for CC data from reference frames
   * \param reorderBuffer Reorder buffer for CC data
   * \return Picture type (I_FRAME, P_FRAME, or OTHER)
   */
  CCPictureType ParsePacket(DemuxPacket* pPacket,
                            std::vector<CCaptionBlock*>& tempBuffer,
                            std::vector<CCaptionBlock*>& reorderBuffer) override;

  const char* GetName() const override { return "H264AVCCBitstreamParser"; }

private:
  /*!
   * \brief Parse SEI NAL unit for closed caption data
   *
   * SEI structure: [payload_type] [payload_size] [payload_data] [repeat...]
   * Looks for payload type 4 (user_data_registered_itu_t_t35) containing GA94 CC data.
   *
   * \param buf Pointer to SEI NAL unit data
   * \param len Length of SEI NAL unit
   * \param pts Presentation timestamp
   * \param tempBuffer Temporary buffer for CC data
   */
  void ParseSEINALUnit(uint8_t* buf, int len, double pts, std::vector<CCaptionBlock*>& tempBuffer);
};
