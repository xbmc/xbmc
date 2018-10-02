/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerUtils.h"

using namespace KODI;
using namespace RETRO;

std::string CRetroPlayerUtils::StretchModeToDescription(STRETCHMODE stretchMode)
{
  switch (stretchMode)
  {
    case STRETCHMODE::Normal:
      return "normal";
    case STRETCHMODE::Stretch4x3:
      return "4:3";
    case STRETCHMODE::Fullscreen:
      return "fullscreen";
    case STRETCHMODE::Original:
      return "original";
    default:
      break;
  }

  return "";
}
