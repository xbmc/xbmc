/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>

//! What a decoder does with a picture's ICC profile
enum class ImageColorMode
{
  CONVERT_TO_SRGB, //!< convert the pixels to sRGB
  KEEP_GAMUT, //!< keep the profile's primaries and report them as an ImageColorMatrix
  AS_IS, //!< leave the pixels as they are, e.g. for a gain map
};

//! 3x3 matrix, column-major, as GL and GLES glUniformMatrix3fv take it without transposing
using ImageColorMatrix = std::array<float, 9>;
