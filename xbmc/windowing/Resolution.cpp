/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Resolution.h"

#include "GraphicContext.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/MathUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <cstdlib>
#include <limits>

namespace
{

const char* SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN{"videoscreen.whitelistpulldown"};
const char* SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE{
    "videoscreen.whitelistdoublerefreshrate"};
const char* SETTING_VIDEOSCREEN_WHITELIST_MULTIPLEREFRESHRATE{
    "videoscreen.whitelistmultiplerefreshrate"};
const char* SETTING_VIDEOSCREEN_PREFER_HIGHER_REFRESH_RATES{
    ("videoscreen.preferhigherrefreshrates")};
const char* SETTING_VIDEOSCREEN_MATCH_VIDEO_RESOLUTION{("videoscreen.matchvideoresolution")};

} // namespace

EdgeInsets::EdgeInsets(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b)
{
}

RESOLUTION_INFO::RESOLUTION_INFO(int width, int height, float aspect, const std::string &mode) :
  strMode(mode)
{
  iWidth = width;
  iHeight = height;
  iBlanking = 0;
  iScreenWidth = width;
  iScreenHeight = height;
  fPixelRatio = aspect ? ((float)width)/height / aspect : 1.0f;
  bFullScreen = true;
  fRefreshRate = 0;
  dwFlags = iSubtitles = 0;
}

float RESOLUTION_INFO::DisplayRatio() const
{
  return iWidth * fPixelRatio / iHeight;
}

RESOLUTION CResolutionUtils::ChooseBestResolution(float fps, int width, int height, bool is3D)
{
  RESOLUTION res = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
  float weight = 0.0f;

  if (!FindResolutionFromOverride(fps, width, is3D, res, weight, false)) //find a refreshrate from overrides
  {
    if (!FindResolutionFromOverride(fps, width, is3D, res, weight, true)) //if that fails find it from a fallback
    {
      FindResolutionFromWhitelist(fps, width, height, is3D, res); //find a refreshrate from whitelist
    }
  }

  CLog::Log(LOGINFO, "Display resolution ADJUST : {} ({}) (weight: {:.3f})",
            CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(res).strMode, res, weight);
  return res;
}

void CResolutionUtils::FindResolutionFromWhitelist(
    float fps, int width, int height, bool is3D, RESOLUTION& resolution)
{
  RESOLUTION_INFO curr = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(resolution);
  CLog::LogF(LOGINFO, "Searching the whitelist for: width: {}, height: {}, fps: {:0.3f}, 3D: {}",
             width, height, fps, is3D ? "true" : "false");

  std::vector<CVariant> indexList = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
      CSettings::SETTING_VIDEOSCREEN_WHITELIST);

  bool noWhiteList = indexList.empty();
  const bool matchResolution = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      SETTING_VIDEOSCREEN_MATCH_VIDEO_RESOLUTION);

  if (noWhiteList)
  {
    CLog::LogF(LOGDEBUG, "Using the default whitelist because the user whitelist is empty");

    std::vector<RESOLUTION> candidates;
    RESOLUTION_INFO info;
    CServiceBroker::GetWinSystem()->GetGfxContext().GetAllowedResolutions(candidates);

    for (const auto& c : candidates)
    {
      info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(c);
      if (info.iScreenHeight >= (matchResolution ? height : curr.iScreenHeight) &&
          info.iScreenWidth >= (matchResolution ? width : curr.iScreenWidth) &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK))
      {
        // do not add half refreshrates (25, 29.97 by default) as kodi cannot cope with
        // them on playback start. Especially interlaced content is not properly detected
        // and this causes ugly double switching.
        // This won't allow 25p / 30p playback on native refreshrate by default
        if ((info.fRefreshRate > 30) || (MathUtils::FloatEquals(info.fRefreshRate, 24.0f, 0.1f)))
          indexList.push_back(CDisplaySettings::GetInstance().GetStringFromRes(c));
      }
    }
  }

  // Get additional parameters
  const bool allowDouble =
      noWhiteList || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                         SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE);
  const bool allowPulldown =
      noWhiteList || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                         SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN);
  const bool useMultiple = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      SETTING_VIDEOSCREEN_WHITELIST_MULTIPLEREFRESHRATE);

  if (noWhiteList)
    CLog::LogF(LOGDEBUG, "No whitelist - forces allow double refresh rates and pulldown rates ON");
  CLog::LogF(LOGDEBUG, "Allow double refresh rates {}", allowDouble ? "ON" : "OFF");
  CLog::LogF(LOGDEBUG, "Use multiple refresh rates {}", useMultiple ? "ON" : "OFF");
  CLog::LogF(LOGDEBUG, "Allow 3:2 pulldown refresh rates {}", allowPulldown ? "ON" : "OFF");
  CLog::LogF(LOGDEBUG, "Match video resolution {}", matchResolution ? "ON" : "OFF");

  // Precision
  static constexpr float FPS_PRECISION = 0.01f;
  static constexpr float FPS_MULTIPLE_PRECISION = 0.0005f;

  // Weighting
  int MATCH_RESOLUTION = 128;
  int MATCH_DESKTOP_RESOLUTION = 64;
  // reserved (32)
  int MATCH_MULTIPLE_REFRESH_RATE = 16;
  int MATCH_DOUBLE_REFRESH_RATE = 8;
  int MATCH_REFRESH_RATE = 4;
  int MATCH_PULLDOWN_RATE = 2;
  int MATCH_DESKTOP_REFRESH_RATE = 1;

  // Adjust weighting
  if (!useMultiple)
  {
    MATCH_MULTIPLE_REFRESH_RATE = 0;
    MATCH_REFRESH_RATE = 32;
  }
  if (!allowDouble)
    MATCH_DOUBLE_REFRESH_RATE = 0;
  if (!allowPulldown)
    MATCH_PULLDOWN_RATE = 0;

  const RESOLUTION_INFO desktop_info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(
      CDisplaySettings::GetInstance().GetCurrentResolution());

  std::vector<std::tuple<RESOLUTION, int, float, int>>
      resolutions; // index, resolution dfference, frequency, weighting

  CLog::LogF(LOGDEBUG, "Scanning available resolutions");
  for (const auto& mode : indexList)
  {
    const RESOLUTION i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

    CLog::LogF(LOGDEBUG, "Considering resolution {}", info.strMode);

    if ((info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK))
    {
      int w{0};
      float m{};
      const float f{modff(info.fRefreshRate / fps, &m)};
      const float p{modff(info.fRefreshRate / (fps * 2.5f), &m)};

      // note: height has greater tolerance due to 32/64px boundaries e.g. 1080?1088 or 2160?2176
      if ((height == info.iScreenHeight && width <= info.iScreenWidth + 8) ||
          (width == info.iScreenWidth && height <= info.iScreenHeight + 32))
        w |= MATCH_RESOLUTION;
      else if (info.iScreenWidth == desktop_info.iScreenWidth &&
               info.iScreenHeight == desktop_info.iScreenHeight)
        w |= MATCH_DESKTOP_RESOLUTION;
      if (MathUtils::FloatEquals(info.fRefreshRate, fps, FPS_PRECISION))
        w |= MATCH_REFRESH_RATE;
      else if (MathUtils::FloatEquals(info.fRefreshRate, fps * 2.0f, FPS_PRECISION))
        w |= MATCH_DOUBLE_REFRESH_RATE;
      else if (useMultiple && (f < FPS_MULTIPLE_PRECISION || abs(1 - f) < FPS_MULTIPLE_PRECISION))
        w |= MATCH_MULTIPLE_REFRESH_RATE;
      else if (allowPulldown &&
               (MathUtils::FloatEquals(info.fRefreshRate, fps * 2.5f, FPS_PRECISION) ||
                (useMultiple &&
                 (p < FPS_MULTIPLE_PRECISION || abs(1 - p) < FPS_MULTIPLE_PRECISION))))
        w |= MATCH_PULLDOWN_RATE;
      else if (MathUtils::FloatEquals(info.fRefreshRate, desktop_info.fRefreshRate, FPS_PRECISION))
        w |= MATCH_DESKTOP_REFRESH_RATE;

      if (w > 0)
      {
        // Found resolution for consideration
        resolutions.emplace_back(std::make_tuple(
            i, (info.iScreenWidth + info.iScreenHeight - height - width), info.fRefreshRate, w));
        CLog::LogF(LOGDEBUG, "Resolution {} given weighting {} ({})", info.strMode, w, i);
      }
    }
  }

  if (!resolutions.empty())
  {
    // Sort list based on weighting (then proximity of resolution, then frequency)
    std::sort(resolutions.begin(), resolutions.end(),
              [&](const std::tuple<RESOLUTION, int, float, int>& i,
                  const std::tuple<RESOLUTION, int, float, int>& j)
              {
                if (std::get<3>(i) == std::get<3>(j)) // weighting
                  if (std::get<1>(i) == std::get<1>(j)) // resolution proximity
                    return std::get<2>(i) > std::get<2>(j); // frequency
                  else
                    return std::get<1>(i) < std::get<1>(j);
                else
                  return std::get<3>(i) > std::get<3>(j);
              });

    // Log sorted list
    CLog::LogF(LOGDEBUG, "Sorted weighted resolution list");
    for (const auto& r : resolutions)
    {
      const auto info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(std::get<0>(r));
      CLog::LogF(LOGDEBUG, "Resolution {} Weighting {} ({})", info.strMode, std::get<3>(r),
                 std::get<0>(r));
    }

    // Choose resolution
    CLog::LogF(LOGINFO, "Chosen resolution {}",
               CServiceBroker::GetWinSystem()
                   ->GetGfxContext()
                   .GetResInfo(std::get<0>(resolutions[0]))
                   .strMode);
    resolution = std::get<0>(resolutions[0]);
  }
  else
    CLog::LogF(LOGINFO, "No resolution matched");

  return;
}

bool CResolutionUtils::FindResolutionFromOverride(float fps, int width, bool is3D, RESOLUTION &resolution, float& weight, bool fallback)
{
  RESOLUTION_INFO curr = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(resolution);

  //try to find a refreshrate from the override
  for (int i = 0; i < (int)CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAdjustRefreshOverrides.size(); i++)
  {
    RefreshOverride& override = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAdjustRefreshOverrides[i];

    if (override.fallback != fallback)
      continue;

    //if we're checking for overrides, check if the fps matches
    if (!fallback && (fps < override.fpsmin || fps > override.fpsmax))
      continue;

    for (size_t j = (int)RES_DESKTOP; j < CDisplaySettings::GetInstance().ResolutionInfoSize(); j++)
    {
      RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo((RESOLUTION)j);

      if (info.iScreenWidth  == curr.iScreenWidth &&
          info.iScreenHeight == curr.iScreenHeight &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK))
      {
        if (info.fRefreshRate <= override.refreshmax &&
            info.fRefreshRate >= override.refreshmin)
        {
          resolution = (RESOLUTION)j;

          if (fallback)
          {
            CLog::Log(
                LOGDEBUG,
                "Found Resolution {} ({}) from fallback (refreshmin:{:.3f} refreshmax:{:.3f})",
                info.strMode, resolution, override.refreshmin, override.refreshmax);
          }
          else
          {
            CLog::Log(LOGDEBUG,
                      "Found Resolution {} ({}) from override of fps {:.3f} (fpsmin:{:.3f} "
                      "fpsmax:{:.3f} refreshmin:{:.3f} refreshmax:{:.3f})",
                      info.strMode, resolution, fps, override.fpsmin, override.fpsmax,
                      override.refreshmin, override.refreshmax);
          }

          weight = RefreshWeight(info.fRefreshRate, fps);

          return true; //fps and refresh match with this override, use this resolution
        }
      }
    }
  }

  return false; //no override found
}

//distance of refresh to the closest multiple of fps (multiple is 1 or higher), as a multiplier of fps
float CResolutionUtils::RefreshWeight(float refresh, float fps)
{
  float div   = refresh / fps;
  int round = MathUtils::round_int(static_cast<double>(div));

  float weight = 0.0f;

  if (round < 1)
    weight = (fps - refresh) / fps;
  else
    weight = fabs(div / round - 1.0f);

  // punish higher refreshrates and prefer better matching
  // e.g. 30 fps content at 60 hz is better than
  // 30 fps at 120 hz - as we sometimes don't know if
  // the content is interlaced at the start, only
  // punish when refreshrate > 60 hz to not have to switch
  // twice for 30i content
  if (refresh > 60 && round > 1)
    weight += round / 10000.0f;

  return weight;
}

bool CResolutionUtils::HasWhitelist()
{
  std::vector<CVariant> indexList = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(CSettings::SETTING_VIDEOSCREEN_WHITELIST);
  return !indexList.empty();
}

void CResolutionUtils::PrintWhitelist()
{
  std::string modeStr;
  auto indexList = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
      CSettings::SETTING_VIDEOSCREEN_WHITELIST);
  if (!indexList.empty())
  {
    for (const auto& mode : indexList)
    {
      auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
      const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);
      modeStr.append("\n" + info.strMode);
    }

    CLog::Log(LOGDEBUG, "[WHITELIST] whitelisted modes:{}", modeStr);
  }
}

void CResolutionUtils::GetMaxAllowedScreenResolution(unsigned int& width, unsigned int& height)
{
  if (!CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot())
    return;

  std::vector<RESOLUTION_INFO> resList;

  auto indexList = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
      CSettings::SETTING_VIDEOSCREEN_WHITELIST);

  unsigned int maxWidth{0};
  unsigned int maxHeight{0};

  if (!indexList.empty())
  {
    for (const auto& mode : indexList)
    {
      RESOLUTION res = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
      RESOLUTION_INFO resInfo{CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(res)};
      if (static_cast<unsigned int>(resInfo.iScreenWidth) > maxWidth &&
          static_cast<unsigned int>(resInfo.iScreenHeight) > maxHeight)
      {
        maxWidth = static_cast<unsigned int>(resInfo.iScreenWidth);
        maxHeight = static_cast<unsigned int>(resInfo.iScreenHeight);
      }
    }
  }
  else
  {
    std::vector<RESOLUTION> resList;
    CServiceBroker::GetWinSystem()->GetGfxContext().GetAllowedResolutions(resList);

    for (const auto& res : resList)
    {
      RESOLUTION_INFO resInfo{CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(res)};
      if (static_cast<unsigned int>(resInfo.iScreenWidth) > maxWidth &&
          static_cast<unsigned int>(resInfo.iScreenHeight) > maxHeight)
      {
        maxWidth = static_cast<unsigned int>(resInfo.iScreenWidth);
        maxHeight = static_cast<unsigned int>(resInfo.iScreenHeight);
      }
    }
  }

  width = maxWidth;
  height = maxHeight;
}
