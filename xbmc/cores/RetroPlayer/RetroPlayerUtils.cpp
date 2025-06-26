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

const char* CRetroPlayerUtils::StretchModeToIdentifier(STRETCHMODE stretchMode)
{
  switch (stretchMode)
  {
    case STRETCHMODE::Normal:
      return STRETCHMODE_NORMAL_ID;
    case STRETCHMODE::Normal1x1:
      return STRETCHMODE_NORMAL_1_1_ID;
    case STRETCHMODE::Stretch4x3:
      return STRETCHMODE_STRETCH_4_3_ID;
    case STRETCHMODE::Stretch16x9:
      return STRETCHMODE_STRETCH_16_9_ID;
    case STRETCHMODE::Fullscreen:
      return STRETCHMODE_FULLSCREEN_ID;
    case STRETCHMODE::Integer:
      return STRETCHMODE_INTEGER_ID;
    case STRETCHMODE::Integer1x1:
      return STRETCHMODE_INTEGER_1_1_ID;
    case STRETCHMODE::Original:
      return STRETCHMODE_ORIGINAL_ID;
    case STRETCHMODE::Zoom:
      return STRETCHMODE_ZOOM_ID;
    default:
      break;
  }

  return "";
}

STRETCHMODE CRetroPlayerUtils::IdentifierToStretchMode(const std::string& stretchMode)
{
  if (stretchMode == STRETCHMODE_NORMAL_ID)
    return STRETCHMODE::Normal;
  else if (stretchMode == STRETCHMODE_NORMAL_1_1_ID)
    return STRETCHMODE::Normal1x1;
  else if (stretchMode == STRETCHMODE_STRETCH_4_3_ID)
    return STRETCHMODE::Stretch4x3;
  else if (stretchMode == STRETCHMODE_STRETCH_16_9_ID)
    return STRETCHMODE::Stretch16x9;
  else if (stretchMode == STRETCHMODE_FULLSCREEN_ID)
    return STRETCHMODE::Fullscreen;
  else if (stretchMode == STRETCHMODE_INTEGER_ID)
    return STRETCHMODE::Integer;
  else if (stretchMode == STRETCHMODE_INTEGER_1_1_ID)
    return STRETCHMODE::Integer1x1;
  else if (stretchMode == STRETCHMODE_ORIGINAL_ID)
    return STRETCHMODE::Original;
  else if (stretchMode == STRETCHMODE_ZOOM_ID)
    return STRETCHMODE::Zoom;

  return STRETCHMODE::Normal;
}
