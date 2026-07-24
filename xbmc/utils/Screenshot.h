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

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace KODI::RENDERING::CAPTURE
{
class CCaptureHandle;
}

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
                             bool sync,
                             KODI::RENDERING::CAPTURE::CaptureContent content =
                                 KODI::RENDERING::CAPTURE::CaptureContent::COMPOSITE);

  //! \brief Capture the video-only image and the full composite from the same
  //! rendered frame into two files in the configured folder
  //! (screenshotNNNNN.png and screenshotNNNNN-video.png).
  static void TakeScreenshotBoth();

  //! \brief Block until a one-shot capture is delivered, returning true on
  //! success. On the render/process thread the wait pumps real frames (a
  //! plain wait there would starve the frame the capture needs); off-thread
  //! it waits passively. Returns false as soon as the request fails.
  //! Callers other than screenshots (bookmark thumbnails) route through here
  //! so the private render-loop pump needs only the one friend declaration in
  //! CGUIWindowManager.
  static bool PumpForCapture(KODI::RENDERING::CAPTURE::CCaptureHandle& handle,
                             std::chrono::milliseconds timeout);

private:
  static std::vector<std::function<std::unique_ptr<IScreenshotSurface>()>> m_screenShotSurfaces;
};
