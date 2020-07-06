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
