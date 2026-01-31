/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum class RenderStereoView
{
  OFF,
  LEFT,
  RIGHT,
};

enum class RenderStereoMode
{
  OFF,
  SPLIT_HORIZONTAL,
  SPLIT_VERTICAL,
  ANAGLYPH_RED_CYAN,
  ANAGLYPH_GREEN_MAGENTA,
  ANAGLYPH_YELLOW_BLUE,
  INTERLACED,
  CHECKERBOARD,
  HARDWAREBASED,
  MONO,
  COUNT,

  // Pseudo modes
  AUTO = 100,
  UNDEFINED = 999,
};
