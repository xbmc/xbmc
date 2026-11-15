/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cmath>

namespace KODI::TOUCH
{
enum class VideoPinchMode
{
  NONE,
  NORMAL,
  ZOOM,
};

inline int GetVideoDoubleTapSeekSeconds(double x, double width)
{
  if (!std::isfinite(x) || !std::isfinite(width) || width <= 0 || x < 0 || x > width)
    return 0;
  return x < width / 2 ? -10 : 10;
}

inline VideoPinchMode GetVideoPinchMode(double scale)
{
  if (scale >= 1.10 && std::isfinite(scale))
    return VideoPinchMode::ZOOM;
  if (scale > 0 && scale <= 0.90)
    return VideoPinchMode::NORMAL;
  return VideoPinchMode::NONE;
}
} // namespace KODI::TOUCH
