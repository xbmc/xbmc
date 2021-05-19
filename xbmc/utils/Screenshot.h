/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IScreenshotSurface.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class CScreenShot
{
public:
  static void Register(const std::function<std::unique_ptr<IScreenshotSurface>()>& createFunc);

  static void TakeScreenshot();
  static void TakeScreenshot(const std::string &filename, bool sync);

private:
  static std::vector<std::function<std::unique_ptr<IScreenshotSurface>()>> m_screenShotSurfaces;
};
