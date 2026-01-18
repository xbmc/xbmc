/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ICCBitstreamParser.h"
#include "cores/FFmpeg.h"

#include <memory>

/*!
 * \brief Factory for creating codec-specific closed caption bitstream parsers
 */
class CCBitstreamParserFactory
{
public:
  /*!
   * \brief Create appropriate parser based on codec and extradata
   *
   * \param codec AVCodecID identifying the video codec
   * \param extradata Pointer to codec extradata
   * \param extrasize Size of extradata in bytes
   * \return Parser instance or nullptr if codec is unsupported
   */
  static std::unique_ptr<ICCBitstreamParser> CreateParser(AVCodecID codec,
                                                          std::span<const uint8_t> extradata);
};
