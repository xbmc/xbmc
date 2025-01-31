/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"

namespace KODI
{
namespace RETRO
{
class CRPRendererDMAUtils
{
public:
  /*!
   * \brief Check if the DMA renderer supports the specified scaling method
   *
   * Note how the graphics API (OpenGL vs. OpenGLES) is abstracted here. The
   * reason is because this interface is exposed to the DMA buffer code, which
   * is currently independent of graphics API (i.e. no use of `HAS_GLES` in
   * the DMA buffer code).
   */
  static bool SupportsScalingMethod(SCALINGMETHOD method);
};
} // namespace RETRO
} // namespace KODI
