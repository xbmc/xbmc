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
  const float defaultLuminance = 400.0f;
  float lum1 = defaultLuminance;

  unsigned int maxLuminance = static_cast<unsigned int>(defaultLuminance);

  if (hasDisplayMetadata && displayMetadata.has_luminance && displayMetadata.max_luminance.den)
  {
    const uint16_t lum = displayMetadata.max_luminance.num / displayMetadata.max_luminance.den;

    if (lum > 0)
      maxLuminance = lum;
  }

  if (hasLightMetadata)
  {
    float lum2;

    if (lightMetadata.MaxCLL >= maxLuminance)
    {
      lum1 = static_cast<float>(maxLuminance);
      lum2 = static_cast<float>(lightMetadata.MaxCLL);
    }
    else
    {
      lum1 = static_cast<float>(lightMetadata.MaxCLL);
      lum2 = static_cast<float>(maxLuminance);
    }
    const float lum3 = static_cast<float>(lightMetadata.MaxFALL);

    lum1 = (lum1 * 0.5f) + (lum2 * 0.2f) + (lum3 * 0.3f);
  }
  else if (hasDisplayMetadata && displayMetadata.has_luminance)
  {
    lum1 = static_cast<float>(maxLuminance);
  }

  return lum1;
}
