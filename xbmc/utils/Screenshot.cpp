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
#include "jobs/JobManager.h"
#include "pictures/Picture.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

using namespace XFILE;

std::vector<std::function<std::unique_ptr<IScreenshotSurface>()>> CScreenShot::m_screenShotSurfaces;

void CScreenShot::Register(const std::function<std::unique_ptr<IScreenshotSurface>()>& createFunc)
{
  m_screenShotSurfaces.emplace_back(createFunc);
}

void CScreenShot::TakeScreenshot(const std::string& filename, bool sync)
{
  if (m_screenShotSurfaces.empty())
  {
    CLog::Log(LOGERROR, "failed to take screenshot: no screenshot surface registered");
    return;
  }

  auto surface = m_screenShotSurfaces.back()();

  if (!surface)
  {
    CLog::Log(LOGERROR, "failed to create screenshot surface");
    return;
  }

  auto* winSystem = CServiceBroker::GetWinSystem();
  auto* gui = CServiceBroker::GetGUI();
  if (!winSystem || !gui)
  {
    CLog::Log(LOGERROR, "Screenshot {} failed: subsystems unavailable",
              CURL::GetRedacted(filename));
    return;
  }

  const ScreenshotContext ctx{*winSystem, gui->GetWindowManager()};

  // Tag color as the display interpreted it. PQ/HLG from the winsystem HDR
  // state; SDR by CTA-861 mode inference (HD/UHD = BT.709, SD = SMPTE-170M).
  using KODI::UTILS::Colorimetry;
  using KODI::UTILS::Eotf;
  ImageColorMetadata color;
  color.primaries = AVCOL_PRI_BT709;
  const Colorimetry colorimetry = winSystem->GetColorimetry();
  if (colorimetry == Colorimetry::BT2020_RGB || colorimetry == Colorimetry::BT2020_YCC ||
      colorimetry == Colorimetry::BT2020_CYCC)
    color.primaries = AVCOL_PRI_BT2020;
  const Eotf eotf = winSystem->GetEotf();
  if (eotf == Eotf::PQ)
    color.transfer = AVCOL_TRC_SMPTE2084;
  else if (eotf == Eotf::HLG)
    color.transfer = AVCOL_TRC_ARIB_STD_B67;
  //! @todo tag AVCOL_TRC_BT2020_12 once a >10-bit output surface exists:
  //! 16-bit unorm AR48/AB48 (DRM_FORMAT_ARGB16161616/ABGR16161616) or
  //! 16-bit float AR4H/AB4H (DRM_FORMAT_ARGB16161616F/ABGR16161616F)
  else if (color.primaries == AVCOL_PRI_BT2020)
    color.transfer = AVCOL_TRC_BT2020_10;
  else if (winSystem->GetGfxContext().GetWidth() > 1024 ||
           winSystem->GetGfxContext().GetHeight() >= 600)
    color.transfer = AVCOL_TRC_BT709;
  else
  {
    color.primaries = AVCOL_PRI_SMPTE170M;
    color.transfer = AVCOL_TRC_SMPTE170M;
  }

  // Display output is limited-range whenever videoscreen.limitedrange is on.
  color.range = winSystem->UseLimitedColor() ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

  if (!surface->Capture(ctx))
  {
    CLog::Log(LOGERROR, "Screenshot {} failed", CURL::GetRedacted(filename));
    return;
  }

  CLog::Log(LOGDEBUG, "Saving screenshot {}", CURL::GetRedacted(filename));

  unsigned int format = XB_FMT_A8R8G8B8;
  if (surface->GetBitDepth() > 8)
  {
    // 10-bit capture: buffer is already RGBA16 with correct alpha, no swap needed
    format = XB_FMT_RGBA16;
  }
  else
  {
    //set alpha byte to 0xFF
    for (int y = 0; y < surface->GetHeight(); y++)
    {
      unsigned char* alphaptr = surface->GetBuffer() - 1 + y * surface->GetStride();
      for (int x = 0; x < surface->GetWidth(); x++)
        *(alphaptr += 4) = 0xFF;
    }
  }

  //if sync is true, the png file needs to be completely written when this function returns
  if (sync)
  {
    if (!CPicture::CreateThumbnailFromSurface(surface->GetBuffer(), surface->GetWidth(),
                                              surface->GetHeight(), surface->GetStride(), filename,
                                              format, color))
      CLog::Log(LOGERROR, "Unable to write screenshot {}", CURL::GetRedacted(filename));

    surface->ReleaseBuffer();
  }
  else
  {
    //make sure the file exists to avoid concurrency issues
    XFILE::CFile file;
    if (file.OpenForWrite(filename))
      file.Close();
    else
      CLog::Log(LOGERROR, "Unable to create file {}", CURL::GetRedacted(filename));

    //write .png file asynchronous with CThumbnailWriter, prevents stalling of the render thread
    //buffer is deleted from CThumbnailWriter
    CThumbnailWriter* thumbnailwriter =
        new CThumbnailWriter(surface->GetBuffer(), surface->GetWidth(), surface->GetHeight(),
                             surface->GetStride(), filename, format, color);
    CServiceBroker::GetJobManager()->AddJob(thumbnailwriter, nullptr);
  }
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
