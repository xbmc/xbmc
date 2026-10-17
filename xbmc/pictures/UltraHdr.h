/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI::PICTURE
{
/*!
 * \brief How to apply an Ultra HDR gain map. The min, max and capacity values are log2.
 */
struct GainMapParams
{
  float min{0.0f};
  float max{1.0f};
  float gamma{1.0f};
  float offsetSdr{1.0f / 64};
  float offsetHdr{1.0f / 64};
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
