/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Color.h"

class ColorUtils
{
  public:
    /*! \brief Change the opacity of a given color
     \param color The original color
     \param opacity The opacity value as a float
     \return the original color with the changed opacity/alpha value
     */
    static UTILS::Color ChangeOpacity(const UTILS::Color color, const float opacity);

    /*! \brief Convert given ARGB color to RGBA color value
     \param color The original color
     \return the original color converted to RGBA value
     */
    static UTILS::Color ConvertToRGBA(const UTILS::Color argb);

    /*! \brief Convert given RGBA color to ARGB color value
     \param color The original color
     \return the original color converted to ARGB value
     */
    static UTILS::Color ConvertToARGB(const UTILS::Color rgba);

    /*! \brief Convert given ARGB color to BGR color value
     \param color The original color
     \return the original color converted to BGR value
     */
    static UTILS::Color ConvertToBGR(const UTILS::Color argb);
};
