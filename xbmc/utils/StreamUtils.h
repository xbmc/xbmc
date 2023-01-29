/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

static constexpr int MP4_BOX_HEADER_SIZE = 8;

class StreamUtils
{
public:
  static int GetCodecPriority(const std::string& codec);

  /*!
   * \brief Make a FourCC code as unsigned integer value
   * \param c1 The first FourCC char
   * \param c2 The second FourCC char
   * \param c3 The third FourCC char
   * \param c4 The fourth FourCC char
   * \return The FourCC as unsigned integer value
   */
  static constexpr uint32_t MakeFourCC(char c1, char c2, char c3, char c4)
  {
    return ((static_cast<uint32_t>(c1) << 24) | (static_cast<uint32_t>(c2) << 16) |
            (static_cast<uint32_t>(c3) << 8) | (static_cast<uint32_t>(c4)));
  }
};
