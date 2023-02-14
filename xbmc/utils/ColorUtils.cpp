/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ColorUtils.h"

#include "StringUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace UTILS::COLOR;

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

Color UTILS::COLOR::ChangeOpacity(const Color argb, const float opacity)
{
  int newAlpha = static_cast<int>(std::ceil(((argb >> 24) & 0xff) * opacity));
  return (argb & 0x00FFFFFF) | (newAlpha << 24);
};

Color UTILS::COLOR::ConvertToRGBA(const Color argb)
{
  return ((argb & 0x00FF0000) << 8) | //RR______
         ((argb & 0x0000FF00) << 8) | //__GG____
         ((argb & 0x000000FF) << 8) | //____BB__
         ((argb & 0xFF000000) >> 24); //______AA
}

Color UTILS::COLOR::ConvertToARGB(const Color rgba)
{
  return ((rgba & 0x000000FF) << 24) | //AA_____
         ((rgba & 0xFF000000) >> 8) | //__RR____
         ((rgba & 0x00FF0000) >> 8) | //____GG__
         ((rgba & 0x0000FF00) >> 8); //______BB
}

Color UTILS::COLOR::ConvertToBGR(const Color argb)
{
  return (argb & 0x00FF0000) >> 16 | //____RR
         (argb & 0x0000FF00) | //__GG__
         (argb & 0x000000FF) << 16; //BB____
}

Color UTILS::COLOR::ConvertHexToColor(const std::string& hexColor)
{
  Color value = 0;
  std::sscanf(hexColor.c_str(), "%x", &value);
  return value;
}

Color UTILS::COLOR::ConvertIntToRGB(int r, int g, int b)
{
  return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

ColorInfo UTILS::COLOR::MakeColorInfo(const Color& argb)
{
  ColorInfo colorInfo;
  colorInfo.colorARGB = argb;
  GetHSLValues(colorInfo);
  return colorInfo;
}

ColorInfo UTILS::COLOR::MakeColorInfo(const std::string& hexColor)
{
  ColorInfo colorInfo;
  colorInfo.colorARGB = ConvertHexToColor(hexColor);
  GetHSLValues(colorInfo);
  return colorInfo;
}

bool UTILS::COLOR::comparePairColorInfo(const std::pair<std::string, ColorInfo>& a,
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

ColorFloats UTILS::COLOR::ConvertToFloats(const Color argb)
{
  ColorFloats c;
  c.alpha = static_cast<float>((argb >> 24) & 0xFF) * (1.0f / 255.0f);
  c.red = static_cast<float>((argb >> 16) & 0xFF) * (1.0f / 255.0f);
  c.green = static_cast<float>((argb >> 8) & 0xFF) * (1.0f / 255.0f);
  c.blue = static_cast<float>(argb & 0xFF) * (1.0f / 255.0f);
  return c;
}

std::string UTILS::COLOR::ConvertToHexRGB(const Color argb)
{
  return StringUtils::Format("{:06X}", argb & ~0xFF000000);
}
