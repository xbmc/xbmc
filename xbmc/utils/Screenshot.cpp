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
#include "threads/SystemClock.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <chrono>

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

void CScreenShot::TakeScreenshot(const std::string& filename,
                                 bool sync,
                                 KODI::RENDERING::CAPTURE::CaptureContent content)
{
  using namespace KODI::RENDERING::CAPTURE;

  const auto captureService = CServiceBroker::GetCaptureService();
  if (!captureService)
  {
    CLog::Log(LOGERROR, "Screenshot {} failed: no capture service", CURL::GetRedacted(filename));
    return;
  }

  CLog::Log(LOGDEBUG, "Saving screenshot {}", CURL::GetRedacted(filename));

  // screenshots are tagged captures whatever their content: native depth
  // kept, the display's coding carried as cICP by the writer
  CaptureSpec spec;
  spec.content = content;
  spec.format = CaptureFormat::NATIVE;

  if (!sync)
  {
    // async: the write runs on the service worker, and ONLY on a successful
    // delivery (the callback never fires on failure), so a failed capture
    // leaves no empty file behind
    auto handle = captureService->Submit(
        spec, [filename](const CaptureResult& result) { WriteCapture(result, filename); });
    handle->Detach();
    return;
  }

  // sync contract: return only when the file is written. Pump the render
  // loop while waiting (see PumpForCapture), then write inline.
  auto handle = captureService->Submit(spec);
  if (PumpForCapture(*handle, 2000ms))
    WriteCapture(handle->GetResult(), filename);
  else
    CLog::Log(LOGERROR, "Screenshot {} failed", CURL::GetRedacted(filename));
}

bool CScreenShot::PumpForCapture(KODI::RENDERING::CAPTURE::CCaptureHandle& handle,
                                 std::chrono::milliseconds timeout)
{
  using namespace KODI::RENDERING::CAPTURE;

  const auto appMessenger = CServiceBroker::GetAppMessenger();
  const bool processThread = appMessenger && appMessenger->IsProcessThread();
  auto* gui = CServiceBroker::GetGUI();

  XbmcThreads::EndTime<> deadline(timeout);
  while (!deadline.IsTimePast())
  {
    if (handle.Wait(processThread ? 5ms : 50ms))
      return true;
    if (handle.GetState() == CaptureState::FAILED)
      return false;
    if (processThread && gui)
      gui->GetWindowManager().ProcessRenderLoop(false);
  }
  return false;
}

namespace
{
// Resolve the configured screenshot folder, prompting for it once if unset.
std::string ResolveScreenshotDir()
{
  std::shared_ptr<CSettingPath> setting = std::static_pointer_cast<CSettingPath>(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
          CSettings::SETTING_DEBUG_SCREENSHOTPATH));
  if (!setting)
    return {};

  std::string dir = setting->GetValue();
  if (dir.empty())
  {
    if (!CGUIControlButtonSetting::GetPath(
            setting, &CServiceBroker::GetResourcesComponent().GetLocalizeStrings()))
      return {};
    dir = setting->GetValue();
  }

  URIUtils::RemoveSlashAtEnd(dir);
  return dir;
}
} // namespace

void CScreenShot::TakeScreenshot()
{
  TakeScreenshot(KODI::RENDERING::CAPTURE::CaptureContent::COMPOSITE);
}

void CScreenShot::TakeScreenshot(KODI::RENDERING::CAPTURE::CaptureContent content)
{
  const std::string dir = ResolveScreenshotDir();
  if (dir.empty())
    return;

  const std::string file =
      CUtil::GetNextFilename(URIUtils::AddFileToFolder(dir, "screenshot{:05}.png"), 65535);
  if (!file.empty())
    TakeScreenshot(file, false, content);
  else
    CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
}

void CScreenShot::TakeScreenshotBoth()
{
  using namespace KODI::RENDERING::CAPTURE;

  const auto captureService = CServiceBroker::GetCaptureService();
  if (!captureService)
    return;

  const std::string dir = ResolveScreenshotDir();
  if (dir.empty())
    return;

  const std::string composite =
      CUtil::GetNextFilename(URIUtils::AddFileToFolder(dir, "screenshot{:05}.png"), 65535);
  if (composite.empty())
  {
    CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    return;
  }
  // derive the video name from the same NNNNN so the pair is obvious
  const std::string video = composite.substr(0, composite.length() - 4) + "-video.png";

  // one request, both taps, same frame; the callback names each file by the
  // content delivered
  CaptureSpec spec;
  spec.content = CaptureContent::BOTH;
  spec.format = CaptureFormat::NATIVE;
  auto handle = captureService->Submit(spec, [composite, video](const CaptureResult& result) {
    WriteCapture(result, result.content == CaptureContent::VIDEO ? video : composite);
  });
  handle->Detach();
}
