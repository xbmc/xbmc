/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

namespace UTILS
{
namespace COLOR
{

typedef uint32_t Color;

constexpr Color BLACK = 0xFF000000;
constexpr Color BLUE = 0xFF0099FF;
constexpr Color BRIGHTGREEN = 0xFF00FF00;
constexpr Color CYAN = 0xFF00FFFF;
constexpr Color DARKGREY = 0xFF808080;
constexpr Color GREY = 0xFFC0C0C0;
constexpr Color LIGHTGREY = 0xFFE5E5E5;
constexpr Color NONE = 0x00000000;
constexpr Color WHITE = 0xFFFFFFFF;
constexpr Color YELLOW = 0xFFFFFF00;
constexpr Color YELLOWGREEN = 0xFFCCFF00;

struct ColorInfo
{
  Color colorARGB;
  double hue;
  double saturation;
  double lightness;
};

struct ColorFloats
{
  float red;
  float green;
  float blue;
  float alpha;
};

/*! \brief Change the opacity of a given ARGB color
    \param color The original color
    \param opacity The opacity value as a float
    \return the original color with the changed opacity/alpha value
*/
Color ChangeOpacity(const Color argb, const float opacity);

/*! \brief Convert given ARGB color to RGBA color value
    \param color The original color
    \return the original color converted to RGBA value
*/
Color ConvertToRGBA(const Color argb);

/*! \brief Convert given RGBA color to ARGB color value
    \param color The original color
    \return the original color converted to ARGB value
*/
Color ConvertToARGB(const Color rgba);

/*! \brief Convert given ARGB color to BGR color value
    \param color The original color
    \return the original color converted to BGR value
*/
Color ConvertToBGR(const Color argb);

/*! \brief Convert given hex value to Color value
    \param hexColor The original hex color
    \return the original hex color converted to Color value
*/
Color ConvertHexToColor(const std::string& hexColor);

/*! \brief Convert given RGB int values to RGB color value
    \param r The red value
    \param g The green value
    \param b The blue value
    \return the color as RGB value
*/
Color ConvertIntToRGB(int r, int g, int b);

/*! \brief Create a ColorInfo from an ARGB Color to
           get additional information of the color
           and allow to be sorted with a color comparer
    \param argb The original ARGB color
    \return the ColorInfo
*/
ColorInfo MakeColorInfo(const Color& argb);

/*! \brief Create a ColorInfo from an HEX color value to
           get additional information of the color
           and allow to be sorted with a color comparer
    \param hexColor The original ARGB color
    \return the ColorInfo
*/
ColorInfo MakeColorInfo(const std::string& hexColor);

/*! \brief Comparer for pair string/ColorInfo to sort colors in a hue scale
*/
bool comparePairColorInfo(const std::pair<std::string, ColorInfo>& a,
                          const std::pair<std::string, ColorInfo>& b);

/*! \brief Convert given ARGB color to ColorFloats
    \param color The original color
    \return the original color converted to ColorFloats
*/
ColorFloats ConvertToFloats(const Color argb);
} // namespace COLOR
} // namespace UTILS
