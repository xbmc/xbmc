/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <string>

namespace KODI::PICTURE
{
/*!
 * \brief A value per colour channel: red, green and blue.
 */
using GainMapChannels = std::array<float, 3>;

/*!
 * \brief How to apply an Ultra HDR gain map. The min, max and capacity values are log2.
 */
struct GainMapParams
{
  GainMapChannels min{0.0f, 0.0f, 0.0f};
  GainMapChannels max{1.0f, 1.0f, 1.0f};
  GainMapChannels gamma{1.0f, 1.0f, 1.0f};
  GainMapChannels offsetSdr{1.0f / 64, 1.0f / 64, 1.0f / 64};
  GainMapChannels offsetHdr{1.0f / 64, 1.0f / 64, 1.0f / 64};
  float capacityMin{0.0f};
  float capacityMax{1.0f};
};

struct GainMap
{
  std::string image; //!< The gain map JPEG, empty if the picture has none
  GainMapParams params;
};

/*!
 * \brief Read the gain map that makes an Ultra HDR JPEG an HDR picture.
 */
GainMap ReadGainMap(const std::string& path);
} // namespace KODI::PICTURE
