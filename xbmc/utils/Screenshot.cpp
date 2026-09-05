/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Screenshot.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/TextureFormats.h"
#include "pictures/Picture.h"
#include "rendering/capture/CaptureHandle.h"
#include "rendering/capture/CapturePixels.h"
#include "rendering/capture/CaptureService.h"
#include "rendering/capture/CaptureTypes.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/windows/GUIControlSettings.h"
#include "threads/Event.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

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
// Map a capture's source coding to the encoder's XB_FMT input format.
unsigned int CaptureXbFormat(AVPixelFormat format)
{
  switch (format)
  {
    case AV_PIX_FMT_BGRA:
      return XB_FMT_A8R8G8B8;
    case AV_PIX_FMT_RGBA:
      return XB_FMT_RGBA8;
    case AV_PIX_FMT_RGBA64LE:
      return XB_FMT_RGBA16;
    case AV_PIX_FMT_X2BGR10LE:
      return XB_FMT_X2BGR10;
    default:
      return XB_FMT_UNKNOWN;
  }
}

// Encode a delivered capture to the destination file; runs on the capture
// service's callback worker.
bool WriteCapture(const KODI::RENDERING::CAPTURE::CaptureResult& result,
                  const std::string& filename)
{
  using namespace KODI::RENDERING::CAPTURE;
  if (!result.pixels)
  {
    CLog::Log(LOGERROR, "Unable to write screenshot {}: no pixels", CURL::GetRedacted(filename));
    return false;
  }

  CScopedCapturePixels lock(*result.pixels);
  if (!lock.data())
  {
    CLog::Log(LOGERROR, "Unable to write screenshot {}: pixel lock failed",
              CURL::GetRedacted(filename));
    return false;
  }

  // OPAQUE: the encoder drops the framebuffer alpha through an X-variant source
  const unsigned int format = CaptureXbFormat(result.format) | XB_FMT_OPAQUE;
  const uint8_t* src0 = CaptureSrcRow0(lock.data(), result.stride, result.height);

  if (!CPicture::CreateThumbnailFromSurface(src0, result.width, result.height,
                                            static_cast<unsigned int>(result.stride), filename,
                                            format, result.color))
  {
    CLog::Log(LOGERROR, "Unable to write screenshot {}", CURL::GetRedacted(filename));
    return false;
  }

  return true;
}
} // namespace

void CScreenShot::TakeScreenshot(const std::string& filename,
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

  // failure dispatches an empty result because some consumers must act on
  // it (a bookmark still writes its DB row); here WriteCapture declines it
  auto handle = captureService->Submit(spec, [filename](const CaptureResult& result)
                                       { WriteCapture(result, filename); });
  handle->Detach();
}

namespace
{
std::shared_ptr<CSettingPath> ScreenshotPathSetting()
{
  return std::static_pointer_cast<CSettingPath>(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
          CSettings::SETTING_DEBUG_SCREENSHOTPATH));
}

// The configured screenshot folder as it stands; empty when unset.
std::string ScreenshotDir()
{
  const std::shared_ptr<CSettingPath> setting = ScreenshotPathSetting();
  if (!setting)
    return {};

  std::string dir = setting->GetValue();
  URIUtils::RemoveSlashAtEnd(dir);
  return dir;
}

// Resolve the configured screenshot folder, prompting for it once if unset.
std::string ResolveScreenshotDir()
{
  const std::shared_ptr<CSettingPath> setting = ScreenshotPathSetting();
  if (!setting)
    return {};

  if (setting->GetValue().empty() &&
      !CGUIControlButtonSetting::GetPath(
          setting, &CServiceBroker::GetResourcesComponent().GetLocalizeStrings()))
    return {};

  return ScreenshotDir();
}

// The next free auto-numbered name in dir; empty when the folder is full or unreadable.
std::string NextScreenshotFile(const std::string& dir)
{
  return CUtil::GetNextFilename(URIUtils::AddFileToFolder(dir, "screenshot{:05}.png"), 65535);
}

// The video-only companion of a composite screenshot, sharing its NNNNN so the
// pair is obvious.
std::string VideoScreenshotFile(const std::string& composite)
{
  return URIUtils::ReplaceExtension(composite, "-video.png");
}
} // namespace

void CScreenShot::TakeScreenshot()
{
  TakeScreenshot(KODI::RENDERING::CAPTURE::CaptureContent::COMPOSITE);
}

void CScreenShot::TakeScreenshot(KODI::RENDERING::CAPTURE::CaptureContent content)
{
  using namespace KODI::RENDERING::CAPTURE;

  const auto captureService = CServiceBroker::GetCaptureService();
  if (!captureService)
    return;

  CaptureSpec spec;
  spec.content = content;
  spec.format = CaptureFormat::NATIVE;

  // Submitted before any filesystem work so the readback grabs the current frame;
  // the folder dialog (when the path is unset), naming, and write all run in the
  // callback afterward, so none of them delay the capture.
  auto handle =
      captureService->Submit(spec,
                             [](const CaptureResult& result)
                             {
                               if (!result.pixels)
                                 return;
                               const std::string dir = ResolveScreenshotDir();
                               if (dir.empty())
                               {
                                 CLog::Log(LOGWARNING, "No screenshot path configured");
                                 return;
                               }
                               const std::string file = NextScreenshotFile(dir);
                               if (file.empty())
                               {
                                 CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
                                 return;
                               }
                               CLog::Log(LOGDEBUG, "Saving screenshot {}", CURL::GetRedacted(file));
                               WriteCapture(result, file);
                             });
  handle->Detach();
}

namespace
{
// A caller-named target, resolved under dir: a bare .png file name, so nothing outside the
// configured folder can be named.
std::string TargetScreenshotFile(const std::string& dir, const std::string& target)
{
  if (!URIUtils::HasExtension(target, ".png") || target.find_first_of("/\\:") != std::string::npos)
    return {};

  return URIUtils::AddFileToFolder(dir, target);
}

const std::string SCREENSHOT_FOLDER = "special://screenshots/";

// A written screenshot named through special://screenshots, which resolves to
// the configured folder wherever it is.
std::string SpecialScreenshotPath(const std::string& file)
{
  return URIUtils::AddFileToFolder(SCREENSHOT_FOLDER, URIUtils::GetFileName(file));
}

// Accepts a bare name or the special://screenshots path TakeScreenshotSync hands out.
std::string ScreenshotName(const std::string& file)
{
  if (StringUtils::StartsWithNoCase(file, SCREENSHOT_FOLDER))
    return file.substr(SCREENSHOT_FOLDER.size());

  return file;
}

// Collects the deliveries of a synchronous screenshot. Shared with the callback,
// which outlives a request that gave up waiting.
struct PendingScreenshot
{
  CEvent done;
  std::atomic<unsigned int> delivered{0};
  std::atomic<unsigned int> written{0};
};

// Covers forcing a frame, reading it back and encoding it at the native resolution.
constexpr std::chrono::milliseconds SCREENSHOT_TIMEOUT{5000};
} // namespace

bool CScreenShot::IsScreenshotPath(const std::string& path)
{
  if (!StringUtils::StartsWithNoCase(path, SCREENSHOT_FOLDER))
    return false;

  const std::string name = path.substr(SCREENSHOT_FOLDER.size());
  return name.find_first_of("/\\") == std::string::npos && URIUtils::HasExtension(name, ".png");
}

CScreenShot::ScreenshotFiles CScreenShot::TakeScreenshotSync(
    KODI::RENDERING::CAPTURE::CaptureContent content, const std::string& target)
{
  using namespace KODI::RENDERING::CAPTURE;

  const std::string dir = ScreenshotDir();
  if (dir.empty())
  {
    CLog::Log(LOGWARNING, "No screenshot path configured");
    return {ScreenshotError::NO_FOLDER};
  }

  const std::string composite =
      target.empty() ? NextScreenshotFile(dir) : TargetScreenshotFile(dir, target);
  if (composite.empty())
  {
    if (target.empty())
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
      return {ScreenshotError::FAILED};
    }
    return {ScreenshotError::BAD_TARGET};
  }

  const bool both = content == CaptureContent::BOTH;
  const std::string video = both ? VideoScreenshotFile(composite) : composite;

  const auto captureService = CServiceBroker::GetCaptureService();
  if (!captureService)
  {
    CLog::Log(LOGERROR, "Screenshot failed: no capture service");
    return {ScreenshotError::FAILED};
  }

  CaptureSpec spec;
  spec.content = content;
  spec.format = CaptureFormat::NATIVE;

  const unsigned int expected = both ? 2u : 1u;
  auto pending = std::make_shared<PendingScreenshot>();

  // This thread waits for the capture worker's write; a BOTH request delivers twice.
  auto handle =
      captureService->Submit(spec,
                             [pending, expected, composite, video](const CaptureResult& result)
                             {
                               if (!result.pixels)
                               {
                                 pending->done.Set();
                                 return;
                               }

                               const std::string& file =
                                   result.content == CaptureContent::VIDEO ? video : composite;
                               CLog::Log(LOGDEBUG, "Saving screenshot {}", CURL::GetRedacted(file));
                               if (WriteCapture(result, file))
                                 ++pending->written;
                               if (++pending->delivered == expected)
                                 pending->done.Set();
                             });

  // handle is not detached: it stays in scope for the wait, because dropping it
  // cancels the request
  if (!pending->done.Wait(SCREENSHOT_TIMEOUT) || pending->written != expected)
  {
    CLog::Log(LOGERROR, "Screenshot {} did not complete", CURL::GetRedacted(composite));
    return {ScreenshotError::FAILED};
  }

  // answered as special://screenshots so the caller holds a path that survives
  // being handed to another machine, and one the web server can resolve
  return {ScreenshotError::NONE,
          content == CaptureContent::VIDEO ? "" : SpecialScreenshotPath(composite),
          content == CaptureContent::COMPOSITE ? "" : SpecialScreenshotPath(video)};
}

CScreenShot::ScreenshotDeletion CScreenShot::DeleteScreenshots(const std::string& file)
{
  const std::string dir = ScreenshotDir();

  if (!file.empty())
  {
    if (dir.empty())
      return {ScreenshotError::NOT_FOUND};

    const std::string path = TargetScreenshotFile(dir, ScreenshotName(file));
    if (path.empty())
      return {ScreenshotError::BAD_TARGET};
    if (!XFILE::CFile::Exists(path))
      return {ScreenshotError::NOT_FOUND};
    if (!XFILE::CFile::Delete(path))
    {
      CLog::Log(LOGERROR, "Unable to delete screenshot {}", CURL::GetRedacted(path));
      return {ScreenshotError::FAILED};
    }
    return {ScreenshotError::NONE, 1};
  }

  // with no folder configured nothing was ever written, so there is nothing to clear
  if (dir.empty())
    return {};

  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(dir, items, ".png", XFILE::DIR_FLAG_NO_FILE_DIRS))
  {
    CLog::Log(LOGERROR, "Unable to read screenshot folder {}", CURL::GetRedacted(dir));
    return {ScreenshotError::FAILED};
  }

  unsigned int deleted = 0;
  for (const auto& item : items)
  {
    if (item->IsFolder())
      continue;

    if (XFILE::CFile::Delete(item->GetPath()))
      ++deleted;
    else
      CLog::Log(LOGWARNING, "Unable to delete screenshot {}", CURL::GetRedacted(item->GetPath()));
  }

  return {ScreenshotError::NONE, deleted};
}
