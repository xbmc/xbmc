/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// clang-format off
enum XB_FMT
{
  XB_FMT_UNKNOWN     = 0x0,
  XB_FMT_DXT1        = 0x1,
  XB_FMT_DXT3        = 0x2,
  XB_FMT_DXT5        = 0x4,
  XB_FMT_DXT5_YCoCg  = 0x8,
  XB_FMT_DXT_MASK    = 0xF,

  XB_FMT_A8R8G8B8    = 0x10, // texture.xbt byte order (matches BGRA8)
  XB_FMT_A8          = 0x20,
  XB_FMT_RGBA8       = 0x40,
  XB_FMT_RGB8        = 0x80,
  XB_FMT_MASK        = 0xFFFF,
  XB_FMT_OPAQUE      = 0x10000,
};
// clang-format on
