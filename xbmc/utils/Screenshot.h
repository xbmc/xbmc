/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IScreenshotSurface.h"
#include "rendering/capture/CaptureTypes.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class CScreenShot
{
public:
  static void Register(const std::function<std::unique_ptr<IScreenshotSurface>()>& createFunc);

  //! \brief Create the platform's capture readback surface, null when none registered.
  static std::unique_ptr<IScreenshotSurface> CreateSurface();

  static void TakeScreenshot();
  //! \brief Screenshot of the given content into the configured folder.
  static void TakeScreenshot(KODI::RENDERING::CAPTURE::CaptureContent content);
  static void TakeScreenshot(const std::string& filename,
                             KODI::RENDERING::CAPTURE::CaptureContent content =
                                 KODI::RENDERING::CAPTURE::CaptureContent::COMPOSITE);

  //! \brief Capture the video-only image and the full composite from the same
  //! rendered frame into two files in the configured folder
  //! (screenshotNNNNN.png and screenshotNNNNN-video.png).
  static void TakeScreenshotBoth();

private:
  static std::vector<std::function<std::unique_ptr<IScreenshotSurface>()>> m_screenShotSurfaces;
};
