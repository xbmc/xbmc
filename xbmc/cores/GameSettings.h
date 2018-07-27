/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace RETRO
{

// NOTE: Only append
enum class SCALINGMETHOD
{
  AUTO = 0,
  NEAREST = 1,
  LINEAR = 2,
  MAX = LINEAR
};

// NOTE: Only append
enum class VIEWMODE
{
  Normal = 0,
  Stretch4x3 = 1,
  Fullscreen = 2,
  Original = 3,
  Max = Original
};

enum class RENDERFEATURE
{
  ROTATION,
  STRETCH,
  ZOOM,
  PIXEL_RATIO,
};

}
}
