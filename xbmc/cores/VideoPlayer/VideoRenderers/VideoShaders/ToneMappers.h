/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C"
{
#include <libavutil/mastering_display_metadata.h>
}

class CToneMappers
{
public:
  static float GetLuminanceValue(bool hasDisplayMetadata,
                                 const AVMasteringDisplayMetadata& displayMetadata,
                                 bool hasLightMetadata,
                                 const AVContentLightMetadata& lightMetadata);
};
