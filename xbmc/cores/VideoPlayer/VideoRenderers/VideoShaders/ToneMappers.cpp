/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ToneMappers.h"

float CToneMappers::GetLuminanceValue(bool hasDisplayMetadata,
                                      const AVMasteringDisplayMetadata& displayMetadata,
                                      bool hasLightMetadata,
                                      const AVContentLightMetadata& lightMetadata)
{
  // default for bad quality HDR-PQ sources (missing or invalid metadata)
  constexpr float defaultLuminance = 400.0f;

  float result = defaultLuminance;
  unsigned int maxLuminance = static_cast<unsigned int>(defaultLuminance);

  if (hasDisplayMetadata && displayMetadata.has_luminance && displayMetadata.max_luminance.den)
  {
    const uint16_t lum = displayMetadata.max_luminance.num / displayMetadata.max_luminance.den;

    if (lum > 0)
      maxLuminance = lum;
  }

  if (hasLightMetadata && lightMetadata.MaxCLL)
  {
    const float lum1 = (lightMetadata.MaxCLL >= maxLuminance)
                           ? static_cast<float>(maxLuminance)
                           : static_cast<float>(lightMetadata.MaxCLL);
    const float lum2 = (lightMetadata.MaxCLL >= maxLuminance)
                           ? static_cast<float>(lightMetadata.MaxCLL)
                           : static_cast<float>(maxLuminance);
    const float lum3 = static_cast<float>(lightMetadata.MaxFALL);

    result = (lum1 * 0.5f) + (lum2 * 0.2f) + (lum3 * 0.3f);
  }
  else if (hasDisplayMetadata && displayMetadata.has_luminance)
  {
    result = static_cast<float>(maxLuminance);
  }

  return result;
}
