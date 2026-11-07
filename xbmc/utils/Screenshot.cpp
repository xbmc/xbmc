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
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

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

// The video-only companion of a composite screenshot, sharing its NNNNN so the
// pair is obvious.
std::string VideoScreenshotFile(const std::string& composite)
{
  return URIUtils::ReplaceExtension(composite, "-video.png");
}

// Where a synchronous screenshot is written until the whole request has succeeded.
std::string StagedScreenshotFile(const std::string& file)
{
  return file + ".tmp.png";
}

// The next auto-numbered name in dir that neither it nor its video companion has taken, written
// or staged; empty when the folder is full.
std::string NextScreenshotFile(const std::string& dir)
{
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(dir, items, ".png",
                                  XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_GET_HIDDEN);
  items.SetFastLookup(true);

  const auto taken = [&items](const std::string& file)
  { return items.Get(file) || items.Get(StagedScreenshotFile(file)); };

  const std::string numbered = URIUtils::AddFileToFolder(dir, "screenshot{:05}.png");
  for (int number = 0; number <= 65535; ++number)
  {
    const std::string file = StringUtils::Format(numbered, number);
    if (!taken(file) && !taken(VideoScreenshotFile(file)))
      return file;
  }
  return {};
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

void CScreenShot::TakeScreenshotBoth()
{
  using namespace KODI::RENDERING::CAPTURE;

  const auto captureService = CServiceBroker::GetCaptureService();
  if (!captureService)
    return;

  auto composite = std::make_shared<std::string>();
  CaptureSpec spec;
  spec.content = CaptureContent::BOTH;
  spec.format = CaptureFormat::NATIVE;
  auto handle = captureService->Submit(
      spec,
      [composite](const CaptureResult& result)
      {
        if (!result.pixels)
          return;
        if (composite->empty())
        {
          const std::string dir = ResolveScreenshotDir();
          if (dir.empty())
          {
            CLog::Log(LOGWARNING, "No screenshot path configured");
            return;
          }
          *composite = NextScreenshotFile(dir);
        }
        if (composite->empty())
        {
          CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
          return;
        }
        // derive the video name from the same NNNNN so the pair is obvious
        const std::string file = result.content == CaptureContent::VIDEO
                                     ? composite->substr(0, composite->length() - 4) + "-video.png"
                                     : *composite;
        CLog::Log(LOGDEBUG, "Saving screenshot {}", CURL::GetRedacted(file));
        WriteCapture(result, file);
      });
  handle->Detach();
}

namespace
{
// A bare .png file name, so nothing outside the configured folder can be named.
bool IsScreenshotName(const std::string& name)
{
  return URIUtils::HasExtension(name, ".png") && name.find_first_of("/\\:") == std::string::npos;
}

// A caller-named target, resolved under dir.
std::string TargetScreenshotFile(const std::string& dir, const std::string& target)
{
  if (!IsScreenshotName(target))
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
  CCriticalSection lock;
  //! The composite's file; without a name from the caller, numbered at the first delivery
  std::string composite;
  //! The files a delivery has started to stage
  std::vector<std::string> started;
  unsigned int written{0};
  //! The request gave up, so a delivery removes again whatever it writes from then on
  bool abandoned{false};
};

// Covers forcing a frame, reading it back and encoding it at the native resolution.
constexpr std::chrono::milliseconds SCREENSHOT_TIMEOUT{5000};
} // namespace

bool CScreenShot::IsScreenshotPath(const std::string& path)
{
  if (!StringUtils::StartsWithNoCase(path, SCREENSHOT_FOLDER))
    return false;

  // With no folder configured, special://screenshots resolves to a relative path
  if (ScreenshotDir().empty())
    return false;

  return IsScreenshotName(path.substr(SCREENSHOT_FOLDER.size()));
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

  auto pending = std::make_shared<PendingScreenshot>();
  if (!target.empty())
  {
    pending->composite = TargetScreenshotFile(dir, target);
    if (pending->composite.empty())
      return {ScreenshotError::BAD_TARGET};
  }

  const auto captureService = CServiceBroker::GetCaptureService();
  if (!captureService)
  {
    CLog::Log(LOGERROR, "Screenshot failed: no capture service");
    return {ScreenshotError::FAILED};
  }

  CaptureSpec spec;
  spec.content = content;
  spec.format = CaptureFormat::NATIVE;

  const bool both = content == CaptureContent::BOTH;
  const unsigned int expected = both ? 2u : 1u;

  // This thread waits for the capture worker's write. A BOTH request delivers its video frame
  // first and ends with the composite.
  auto handle = captureService->Submit(
      spec,
      [pending, both, dir](const CaptureResult& result)
      {
        const bool videoOfBoth = both && result.content == CaptureContent::VIDEO;
        if (!result.pixels)
        {
          pending->done.Set();
          return;
        }

        std::string file;
        {
          std::unique_lock lock(pending->lock);
          // Deliveries are handled one at a time, so a number taken here is taken by no other
          if (!pending->abandoned && pending->composite.empty())
          {
            pending->composite = NextScreenshotFile(dir);
            if (pending->composite.empty())
              CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
          }
          if (!pending->abandoned && !pending->composite.empty())
          {
            file = videoOfBoth ? VideoScreenshotFile(pending->composite) : pending->composite;
            pending->started.emplace_back(file);
          }
        }

        if (!file.empty())
        {
          CLog::Log(LOGDEBUG, "Saving screenshot {}", CURL::GetRedacted(file));
          const std::string staged = StagedScreenshotFile(file);
          const bool written = WriteCapture(result, staged);

          bool abandoned = false;
          {
            std::unique_lock lock(pending->lock);
            if (written)
              ++pending->written;
            abandoned = pending->abandoned;
          }
          if (abandoned && XFILE::CFile::Exists(staged))
            XFILE::CFile::Delete(staged);
        }

        if (!videoOfBoth)
          pending->done.Set();
      });

  // handle is not detached: it stays in scope for the wait, because dropping it
  // cancels the request
  const bool arrived = pending->done.Wait(SCREENSHOT_TIMEOUT);

  bool failed = false;
  std::string composite;
  std::vector<std::string> started;
  {
    std::unique_lock lock(pending->lock);
    failed = !arrived || pending->written != expected;
    if (failed)
      pending->abandoned = true;
    started = pending->started;
    composite = pending->composite;
  }

  // Only a complete screenshot replaces what its names held
  for (auto file = started.begin(); !failed && file != started.end(); ++file)
  {
    failed = (XFILE::CFile::Exists(*file) && !XFILE::CFile::Delete(*file)) ||
             !XFILE::CFile::Rename(StagedScreenshotFile(*file), *file);
  }

  if (failed)
  {
    // A delivery still writing removes its own file once it is done
    for (const std::string& file : started)
    {
      const std::string staged = StagedScreenshotFile(file);
      if (XFILE::CFile::Exists(staged))
        XFILE::CFile::Delete(staged);
    }
    CLog::Log(LOGERROR, "Screenshot did not complete");
    return {ScreenshotError::FAILED};
  }

  // answered as special://screenshots so the caller holds a path that survives
  // being handed to another machine, and one the web server can resolve
  const std::string video = both ? VideoScreenshotFile(composite) : composite;
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
  if (!XFILE::CDirectory::GetDirectory(dir, items, ".png",
                                       XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_GET_HIDDEN))
  {
    CLog::Log(LOGERROR, "Unable to read screenshot folder {}", CURL::GetRedacted(dir));
    return {ScreenshotError::FAILED};
  }

  unsigned int deleted = 0;
  bool failed = false;
  for (const auto& item : items)
  {
    if (item->IsFolder())
      continue;

    if (XFILE::CFile::Delete(item->GetPath()))
      ++deleted;
    else
    {
      failed = true;
      CLog::Log(LOGWARNING, "Unable to delete screenshot {}", CURL::GetRedacted(item->GetPath()));
    }
  }

  return {failed ? ScreenshotError::FAILED : ScreenshotError::NONE, deleted};
}
