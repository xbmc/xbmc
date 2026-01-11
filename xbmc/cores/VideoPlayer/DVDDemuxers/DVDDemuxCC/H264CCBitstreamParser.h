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
 * \brief Base class for H.264 closed caption bitstream parsers
 *
 * Provides shared functionality for parsing H.264 SEI (Supplemental Enhancement
 * Information) payloads containing CEA-608/708 closed caption data.
 *
 * H.264 CC data is embedded in SEI NAL units (type 6) with payload type 4
 * (user_data_registered_itu_t_t35). The data may be prefixed with ITU-T T.35
 * country/provider codes or contain GA94 data directly.
 */
class CH264CCBitstreamParser : public ICCBitstreamParser
{
protected:
  /*!
   * \brief Process SEI payload containing closed caption data
   *
   * Searches for GA94 closed caption payload within SEI user data.
   * Handles both:
   * - ITU-T T.35 format: country_code(1) + provider_code(2) + 'GA94'...
   * - Direct GA94 format: 'GA94'...
   *
   * \param buf Pointer to SEI payload data
   * \param len Length of SEI payload
   * \param pts Presentation timestamp for CC data
   * \param tempBuffer Temporary buffer for CC data (for reordering)
   */
  void ProcessSEIPayload(uint8_t* buf, int len, double pts, std::vector<CCaptionBlock>& tempBuffer);

  /*!
   * \brief Detect slice type from H.264 slice header
   *
   * Parses the slice header to determine if this is an I-slice, P-slice, or B-slice.
   * Used to determine when to decode CC data (I/P frames only).
   *
   * \param buf Pointer to slice NAL unit data (after NAL header)
   * \param len Length of slice data
   * \return Picture type (I_FRAME, P_FRAME, OTHER, or INVALID if parsing fails)
   */
  CCPictureType DetectSliceType(uint8_t* buf, int len);
};
