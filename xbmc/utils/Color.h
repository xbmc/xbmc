/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

namespace UTILS
{

  typedef uint32_t Color;

namespace COLOR
{
  static const Color NONE = 0x00000000;
  static const Color BLACK = 0xFF000000;
  static const Color YELLOW = 0xFFFFFF00;
  static const Color WHITE = 0xFFFFFFFF;
  static const Color LIGHTGREY = 0xFFE5E5E5;
  static const Color GREY = 0xFFC0C0C0;
  static const Color BLUE = 0xFF0099FF;
  static const Color BRIGHTGREEN = 0xFF00FF00;
  static const Color YELLOWGREEN = 0xFFCCFF00;
  static const Color CYAN = 0xFF00FFFF;
} // namespace COLOR
} // namespace UTILS
