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
};
