/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ColorUtils.h"

#include "Color.h"

#include <math.h>

UTILS::Color ColorUtils::ChangeOpacity(const UTILS::Color color, const float opacity)
{
  int newAlpha = ceil( ((color >> 24) & 0xff) * opacity);
  return (color & 0x00FFFFFF) | (newAlpha << 24);
};

UTILS::Color ColorUtils::ConvertToRGBA(const UTILS::Color argb)
{
  return ((argb & 0x00FF0000) << 8) | //RR______
         ((argb & 0x0000FF00) << 8) | //__GG____
         ((argb & 0x000000FF) << 8) | //____BB__
         ((argb & 0xFF000000) >> 24); //______AA
}

UTILS::Color ColorUtils::ConvertToARGB(const UTILS::Color rgba)
{
  return ((rgba & 0x000000FF) << 24) | //AA_____
         ((rgba & 0xFF000000) >> 8) | //__RR____
         ((rgba & 0x00FF0000) >> 8) | //____GG__
         ((rgba & 0x0000FF00) >> 8); //______BB
}

UTILS::Color ColorUtils::ConvertToBGR(const UTILS::Color argb)
{
  return (argb & 0x00FF0000) >> 16 | //____RR
         (argb & 0x0000FF00) | //__GG__
         (argb & 0x000000FF) << 16; //BB____
}
