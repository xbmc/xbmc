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

  //! \brief Why a screenshot did not happen, so a caller can say which.
  enum class ScreenshotError
  {
    NONE,
    NO_FOLDER, //!< no screenshot folder is configured
    BAD_TARGET, //!< the requested name is not a plain .png file name
    NOT_FOUND, //!< the named screenshot is not in the folder
    FAILED, //!< the frame did not arrive, or a file could not be written
  };

  //! \brief What a screenshot wrote, as paths under special://screenshots.
  struct ScreenshotFiles
  {
    ScreenshotError error{ScreenshotError::NONE};
    //! the full display output; empty when only the video frame was asked for
    std::string composite;
    //! the video frame alone; empty when only the composite was asked for
    std::string video;
  };

  //! \brief Screenshot of the given content, written before this returns; an unset folder is
  //!        NO_FOLDER, never a prompt.
  //! \param content what to capture
  //! \param target file name under the configured folder, auto-numbered when empty; the video
  //!               frame takes the matching "-video" name
  static ScreenshotFiles TakeScreenshotSync(KODI::RENDERING::CAPTURE::CaptureContent content,
                                            const std::string& target = "");

  //! \brief Whether a path names a screenshot: the special://screenshots folder and a plain .png name
  static bool IsScreenshotPath(const std::string& path);

  //! \brief What a delete removed.
  struct ScreenshotDeletion
  {
    ScreenshotError error{ScreenshotError::NONE};
    unsigned int deleted{0};
  };

  //! \brief Delete screenshots from the configured folder.
  //! \param file one screenshot, named as TakeScreenshotSync answers or as a bare name; every
  //!             .png in the folder when empty
  static ScreenshotDeletion DeleteScreenshots(const std::string& file = "");

private:
  static std::vector<std::function<std::unique_ptr<IScreenshotSurface>()>> m_screenShotSurfaces;
};
