/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Color.h"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace UTILS
{

/*! \brief Change the opacity of a given color
    \param color The original color
    \param opacity The opacity value as a float
    \return the original color with the changed opacity/alpha value
*/
UTILS::Color ChangeOpacity(const UTILS::Color color, const float opacity);

/*! \brief Convert given ARGB color to RGBA color value
    \param color The original color
    \return the original color converted to RGBA value
*/
UTILS::Color ConvertToRGBA(const UTILS::Color argb);

/*! \brief Convert given RGBA color to ARGB color value
    \param color The original color
    \return the original color converted to ARGB value
*/
UTILS::Color ConvertToARGB(const UTILS::Color rgba);

/*! \brief Convert given ARGB color to BGR color value
    \param color The original color
    \return the original color converted to BGR value
*/
UTILS::Color ConvertToBGR(const UTILS::Color argb);

/*! \brief Convert given hex value to Color value
    \param hexColor The original hex color
    \return the original hex color converted to Color value
*/
UTILS::Color ConvertHexToColor(const std::string& hexColor);

/*! \brief Create a ColorInfo from an ARGB Color to
           get additional information of the color
           and allow to be sorted with a color comparer
    \param argb The original ARGB color
    \return the ColorInfo
*/
UTILS::ColorInfo MakeColorInfo(const UTILS::Color& argb);

/*! \brief Create a ColorInfo from an HEX color value to
           get additional information of the color
           and allow to be sorted with a color comparer
    \param hexColor The original ARGB color
    \return the ColorInfo
*/
UTILS::ColorInfo MakeColorInfo(const std::string& hexColor);

/*! \brief Comparer for pair string/ColorInfo to sort colors in a hue scale
*/
bool comparePairColorInfo(const std::pair<std::string, UTILS::ColorInfo>& a,
                          const std::pair<std::string, UTILS::ColorInfo>& b);

} // namespace UTILS
