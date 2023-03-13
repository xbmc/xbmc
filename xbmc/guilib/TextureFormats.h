/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// clang-format off
#define XB_FMT_MASK   0xffff ///< mask for format info - other flags are outside this
#define XB_FMT_DXT_MASK   15
#define XB_FMT_UNKNOWN     0
#define XB_FMT_DXT1        1
#define XB_FMT_DXT3        2
#define XB_FMT_DXT5        4
#define XB_FMT_DXT5_YCoCg  8
#define XB_FMT_A8R8G8B8   16 // texture.xbt byte order (matches BGRA8)
#define XB_FMT_R8         32 // Single channel, normally used as R,?,?,?
#define XB_FMT_RGBA8      64
#define XB_FMT_RGB8      128
#define XB_FMT_A8        256 // Single channel, used as 1,1,1,A (alpha)
#define XB_FMT_L8        272 // Single channel, used as L,L,L,1 (luma)
#define XB_FMT_L8A8      288 // Dual channel, used as L,L,L,A (lumaalpha)
#define XB_FMT_OPAQUE  65536
// clang-format on
