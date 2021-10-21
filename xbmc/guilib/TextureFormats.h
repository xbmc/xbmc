/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#define XB_FMT_MASK   0xffff ///< mask for format info - other flags are outside this
#define XB_FMT_DXT_MASK   15
#define XB_FMT_UNKNOWN     0
#define XB_FMT_DXT1        1
#define XB_FMT_DXT3        2
#define XB_FMT_DXT5        4
#define XB_FMT_DXT5_YCoCg  8
#define XB_FMT_A8R8G8B8   16 // texture.xbt byte order (matches BGRA8)
#define XB_FMT_A8         32
#define XB_FMT_RGBA8      64
#define XB_FMT_RGB8      128
#define XB_FMT_ETC1      256
#define XB_FMT_ETC2_R    512
#define XB_FMT_ETC2_RGB 1024
#define XB_FMT_ETC2_RGBA 2048
#define XB_FMT_ASTC_4x4 4096 // LDR version
#define XB_FMT_ASTC_8x8 8192 // LDR version
#define XB_FMT_OPAQUE  65536
