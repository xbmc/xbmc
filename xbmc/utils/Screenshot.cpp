/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Screenshot.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "pictures/Picture.h"
#include "rendering/capture/CaptureHandle.h"
#include "rendering/capture/CaptureService.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/windows/GUIControlSettings.h"
#include "threads/Event.h"
#include "threads/SystemClock.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <chrono>

using namespace XFILE;
using namespace std::chrono_literals;

std::vector<std::function<std::unique_ptr<IScreenshotSurface>()>> CScreenShot::m_screenShotSurfaces;

void CScreenShot::Register(const std::function<std::unique_ptr<IScreenshotSurface>()>& createFunc)
{
  m_screenShotSurfaces.emplace_back(createFunc);
}

std::unique_ptr<IScreenshotSurface> CScreenShot::CreateSurface()
{
  if (m_screenShotSurfaces.empty())
    return {};
  return m_screenShotSurfaces.back()();
}

namespace
{
// Encode a delivered capture to the destination file; runs on the caller's
// thread (sync) or the capture service's callback worker (async).
void WriteCapture(const KODI::RENDERING::CAPTURE::CaptureResult& result,
                  const std::string& filename)
{
  unsigned int format = XB_FMT_A8R8G8B8;
  if (result.bitDepth > 8)
  {
    // 10-bit capture: buffer is already RGBA16 with correct alpha, no swap needed
    format = XB_FMT_RGBA16;
  }
  else
  {
    //set alpha byte to 0xFF
    for (unsigned int y = 0; y < result.height; y++)
    {
      unsigned char* alphaptr = result.pixels.get() + y * result.stride + 3;
      for (unsigned int x = 0; x < result.width; x++, alphaptr += 4)
        *alphaptr = 0xFF;
    }
  }

  if (!CPicture::CreateThumbnailFromSurface(result.pixels.get(), result.width, result.height,
                                            result.stride, filename, format, result.color))
    CLog::Log(LOGERROR, "Unable to write screenshot {}", CURL::GetRedacted(filename));
}
} // namespace

void CScreenShot::TakeScreenshot(const std::string& filename, bool sync)
{
  using namespace KODI::RENDERING::CAPTURE;

  const auto captureService = CServiceBroker::GetCaptureService();
  if (!captureService)
  {
    CLog::Log(LOGERROR, "Screenshot {} failed: no capture service", CURL::GetRedacted(filename));
    return;
  }

  CLog::Log(LOGDEBUG, "Saving screenshot {}", CURL::GetRedacted(filename));

  if (!sync)
  {
    //make sure the file exists to avoid concurrency issues
    XFILE::CFile file;
    if (file.OpenForWrite(filename))
      file.Close();
    else
      CLog::Log(LOGERROR, "Unable to create file {}", CURL::GetRedacted(filename));
  }

  // the capture itself is always async; shared event so a post-timeout
  // straggler callback signals harmlessly
  const auto written = std::make_shared<CEvent>();
  auto handle = captureService->Submit({},
                                       [filename, written](const CaptureResult& result)
                                       {
                                         WriteCapture(result, filename);
                                         written->Set();
                                       });

  if (!sync)
  {
    handle->Detach();
    return;
  }

  // sync contract: return only when the file is written. On the render
  // thread wait by pumping real frames (the busy-dialog pattern); a plain
  // wait there would deadlock the frame the capture needs.
  const auto appMessenger = CServiceBroker::GetAppMessenger();
  const bool processThread = appMessenger && appMessenger->IsProcessThread();
  auto* gui = CServiceBroker::GetGUI();

  XbmcThreads::EndTime<> timeout(2000ms);
  bool done = false;
  while (!done && !timeout.IsTimePast())
  {
    done = written->Wait(processThread ? 5ms : 50ms);
    if (!done && processThread && gui)
      gui->GetWindowManager().ProcessRenderLoop(false);
  }

  if (!done)
    CLog::Log(LOGERROR, "Screenshot {} failed", CURL::GetRedacted(filename));
}

void CScreenShot::TakeScreenshot()
{
  std::shared_ptr<CSettingPath> screenshotSetting = std::static_pointer_cast<CSettingPath>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_DEBUG_SCREENSHOTPATH));
  if (!screenshotSetting)
    return;

  std::string strDir = screenshotSetting->GetValue();
  if (strDir.empty())
  {
    if (!CGUIControlButtonSetting::GetPath(
            screenshotSetting, &CServiceBroker::GetResourcesComponent().GetLocalizeStrings()))
      return;

    strDir = screenshotSetting->GetValue();
  }

  URIUtils::RemoveSlashAtEnd(strDir);

  if (!strDir.empty())
  {
    std::string file =
        CUtil::GetNextFilename(URIUtils::AddFileToFolder(strDir, "screenshot{:05}.png"), 65535);

    if (!file.empty())
    {
      TakeScreenshot(file, false);
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}
