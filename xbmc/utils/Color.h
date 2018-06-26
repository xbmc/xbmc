/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
