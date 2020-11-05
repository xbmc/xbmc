/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/gles/ScreenshotSurfaceGLES.h"

#include <memory>

class CScreenshotSurfaceDRM : public CScreenshotSurfaceGLES
{
public:
  static void Register();
  static std::unique_ptr<IScreenshotSurface> CreateSurface();

  bool CaptureVideo() override;
};
