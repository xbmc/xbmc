/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ColorUtils.h"

#include "Color.h"
#include "StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>

using namespace UTILS;

namespace
{

void GetHSLValues(ColorInfo& colorInfo)
{
  double r = (colorInfo.colorARGB & 0x00FF0000) >> 16;
  double g = (colorInfo.colorARGB & 0x0000FF00) >> 8;
  double b = (colorInfo.colorARGB & 0x000000FF);
  r /= 255;
  g /= 255;
  b /= 255;
  const double& maxVal = std::max<double>({r, g, b});
  const double& minVal = std::min<double>({r, g, b});
  double h = 0;
  double s = 0;
  double l = (minVal + maxVal) / 2;
  double d = maxVal - minVal;

  if (d == 0)
  {
    h = s = 0; // achromatic
  }
  else
  {
    s = l > 0.5 ? d / (2 - maxVal - minVal) : d / (maxVal + minVal);
    if (maxVal == r)
    {
      h = (g - b) / d + (g < b ? 6 : 0);
    }
    else if (maxVal == g)
    {
      h = (b - r) / d + 2;
    }
    else if (maxVal == b)
    {
      h = (r - g) / d + 4;
    }
    h /= 6;
  }

  colorInfo.hue = h;
  colorInfo.saturation = s;
  colorInfo.lightness = l;
}

} // unnamed namespace

Color UTILS::ChangeOpacity(const Color color, const float opacity)
{
  int newAlpha = ceil(((color >> 24) & 0xff) * opacity);
  return (color & 0x00FFFFFF) | (newAlpha << 24);
};

Color UTILS::ConvertToRGBA(const Color argb)
{
  return ((argb & 0x00FF0000) << 8) | //RR______
         ((argb & 0x0000FF00) << 8) | //__GG____
         ((argb & 0x000000FF) << 8) | //____BB__
         ((argb & 0xFF000000) >> 24); //______AA
}

Color UTILS::ConvertToARGB(const Color rgba)
{
  return ((rgba & 0x000000FF) << 24) | //AA_____
         ((rgba & 0xFF000000) >> 8) | //__RR____
         ((rgba & 0x00FF0000) >> 8) | //____GG__
         ((rgba & 0x0000FF00) >> 8); //______BB
}

Color UTILS::ConvertToBGR(const Color argb)
{
  return (argb & 0x00FF0000) >> 16 | //____RR
         (argb & 0x0000FF00) | //__GG__
         (argb & 0x000000FF) << 16; //BB____
}

Color UTILS::ConvertHexToColor(const std::string& hexColor)
{
  Color value = 0;
  std::sscanf(hexColor.c_str(), "%x", &value);
  return value;
}

ColorInfo UTILS::MakeColorInfo(const Color& argb)
{
  ColorInfo colorInfo;
  colorInfo.colorARGB = argb;
  GetHSLValues(colorInfo);
  return colorInfo;
}

ColorInfo UTILS::MakeColorInfo(const std::string& hexColor)
{
  ColorInfo colorInfo;
  colorInfo.colorARGB = ConvertHexToColor(hexColor);
  GetHSLValues(colorInfo);
  return colorInfo;
}

bool UTILS::comparePairColorInfo(const std::pair<std::string, ColorInfo>& a,
                                 const std::pair<std::string, ColorInfo>& b)
{
  if (a.second.hue == b.second.hue)
  {
    if (a.second.saturation == b.second.saturation)
      return (a.second.lightness < b.second.lightness);
    else
      return (a.second.saturation < b.second.saturation);
  }
  else
    return (a.second.hue < b.second.hue);
}
