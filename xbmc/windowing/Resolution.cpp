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

struct ResolutionEntry
{
  RESOLUTION index;
  int resolutionDifference;
  float frequency;
  int weighting;
};

constexpr const char* SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN{"videoscreen.whitelistpulldown"};
constexpr const char* SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE{
    "videoscreen.whitelistdoublerefreshrate"};
constexpr const char* SETTING_VIDEOSCREEN_WHITELIST_MULTIPLEREFRESHRATE{
    "videoscreen.whitelistmultiplerefreshrate"};
constexpr const char* SETTING_VIDEOSCREEN_MATCH_VIDEO_RESOLUTION{
    ("videoscreen.matchvideoresolution")};

} // namespace

EdgeInsets::EdgeInsets(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b)
{
}

RESOLUTION_INFO::RESOLUTION_INFO(int width, int height, float aspect, std::string mode)
  : iWidth(width),
    iHeight(height),
    iScreenWidth(width),
    iScreenHeight(height),
    fPixelRatio(aspect > 0.0f ? static_cast<float>(width) / static_cast<float>(height) / aspect
                              : 1.0f),
    strMode(std::move(mode))
{
}

float RESOLUTION_INFO::DisplayRatio() const
{
  return static_cast<float>(iWidth) * fPixelRatio / static_cast<float>(iHeight);
}

RESOLUTION CResolutionUtils::ChooseBestResolution(float fps, int width, int height, bool is3D)
{
  const auto& gfxContext{CServiceBroker::GetWinSystem()->GetGfxContext()};
  RESOLUTION res{gfxContext.GetVideoResolution()};

  if (!FindResolutionFromOverride(fps, res, false)) //find a refreshrate from overrides
  {
    if (!FindResolutionFromOverride(fps, res, true)) //if that fails find it from a fallback
    {
      FindResolutionFromWhitelist(fps, width, height, is3D,
                                  res); //find a refreshrate from whitelist
    }
  }

  CLog::LogF(LOGINFO, "Display resolution ADJUST : {} ({})", gfxContext.GetResInfo(res).strMode,
             res);
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
  const bool allowDouble{noWhiteList ||
                         CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                             SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE)};
  const bool allowPulldown{noWhiteList ||
                           CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                               SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN)};
  const bool useMultiple{CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      SETTING_VIDEOSCREEN_WHITELIST_MULTIPLEREFRESHRATE)};

  if (noWhiteList)
    CLog::LogF(LOGDEBUG, "No whitelist - forces allow double refresh rates and pulldown rates ON");
  CLog::LogF(LOGDEBUG, "Allow double refresh rates {}", allowDouble ? "ON" : "OFF");
  CLog::LogF(LOGDEBUG, "Use multiple refresh rates {}", useMultiple ? "ON" : "OFF");
  CLog::LogF(LOGDEBUG, "Allow 3:2 pulldown refresh rates {}", allowPulldown ? "ON" : "OFF");
  CLog::LogF(LOGDEBUG, "Match video resolution {}", matchResolution ? "ON" : "OFF");

  // Precision
  static constexpr float FPS_PRECISION{0.01f};
  static constexpr float FPS_MULTIPLE_PRECISION{0.0005f};

  // Weighting
  enum class Weighting : uint8_t
  {
    MATCH_RESOLUTION = 64,
    MATCH_DESKTOP_RESOLUTION = 32,
    MATCH_MULTIPLE_REFRESH_RATE = 16,
    MATCH_DOUBLE_REFRESH_RATE = 8,
    MATCH_REFRESH_RATE = 4,
    MATCH_PULLDOWN_RATE = 2,
    MATCH_DESKTOP_REFRESH_RATE = 1,
    NO_MATCH = 0
  };

  // Adjust weighting
  using enum Weighting;
  const int multipleRefreshRateMatch{!useMultiple ? static_cast<int>(NO_MATCH)
                                                  : static_cast<int>(MATCH_MULTIPLE_REFRESH_RATE)};
  const int dobuleRefreshRateMatch{!useMultiple || !allowDouble
                                       ? static_cast<int>(NO_MATCH)
                                       : static_cast<int>(MATCH_DOUBLE_REFRESH_RATE)};
  const int pulldownRateMatch{!allowPulldown ? static_cast<int>(NO_MATCH)
                                             : static_cast<int>(MATCH_PULLDOWN_RATE)};

  const RESOLUTION_INFO desktop_info{CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(
      CDisplaySettings::GetInstance().GetCurrentResolution())};

  std::vector<ResolutionEntry> resolutions;

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
        w |= static_cast<int>(MATCH_RESOLUTION);
      else if (info.iScreenWidth == desktop_info.iScreenWidth &&
               info.iScreenHeight == desktop_info.iScreenHeight)
        w |= static_cast<int>(MATCH_DESKTOP_RESOLUTION);
      if (MathUtils::FloatEquals(info.fRefreshRate, fps, FPS_PRECISION))
        w |= static_cast<int>(MATCH_REFRESH_RATE);
      else if (MathUtils::FloatEquals(info.fRefreshRate, fps * 2.0f, FPS_PRECISION))
        w |= dobuleRefreshRateMatch;
      else if (useMultiple && (f < FPS_MULTIPLE_PRECISION || abs(1 - f) < FPS_MULTIPLE_PRECISION))
        w |= multipleRefreshRateMatch;
      else if (allowPulldown &&
               (MathUtils::FloatEquals(info.fRefreshRate, fps * 2.5f, FPS_PRECISION) ||
                (useMultiple &&
                 (p < FPS_MULTIPLE_PRECISION || abs(1 - p) < FPS_MULTIPLE_PRECISION))))
        w |= pulldownRateMatch;
      else if (MathUtils::FloatEquals(info.fRefreshRate, desktop_info.fRefreshRate, FPS_PRECISION))
        w |= static_cast<int>(MATCH_DESKTOP_REFRESH_RATE);

      if (w > 0)
      {
        // Found resolution for consideration
        resolutions.emplace_back(ResolutionEntry{
            .index = i,
            .resolutionDifference = (info.iScreenWidth + info.iScreenHeight - height - width),
            .frequency = info.fRefreshRate,
            .weighting = w});
        CLog::LogF(LOGDEBUG, "Resolution {} given weighting {} ({})", info.strMode, w, i);
      }
    }
  }

  if (!resolutions.empty())
  {
    // Sort list based on weighting (then proximity of resolution, then frequency)
    std::ranges::sort(resolutions,
                      [](const ResolutionEntry& i, const ResolutionEntry& j)
                      {
                        if (i.weighting == j.weighting)
                        {
                          if (i.resolutionDifference == j.resolutionDifference)
                            return i.frequency > j.frequency;
                          return i.resolutionDifference < j.resolutionDifference;
                        }
                        return i.weighting > j.weighting;
                      });

    // Log sorted list
    CLog::LogF(LOGDEBUG, "Sorted weighted resolution list");
    for (const auto& r : resolutions)
    {
      const auto info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(r.index);
      CLog::LogF(LOGDEBUG, "Resolution {} Weighting {} ({})", info.strMode, r.weighting, r.index);
    }

    // Choose resolution
    CLog::LogF(
        LOGINFO, "Chosen resolution {}",
        CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(resolutions[0].index).strMode);
    resolution = resolutions[0].index;
  }
  else
    CLog::LogF(LOGINFO, "No resolution matched");
}

bool CResolutionUtils::FindResolutionFromOverride(float fps, RESOLUTION& resolution, bool fallback)
{
  const auto& gfxContext{CServiceBroker::GetWinSystem()->GetGfxContext()};
  RESOLUTION_INFO curr{gfxContext.GetResInfo(resolution)};

  //try to find a refreshrate from the override
  for (const auto& override :
       CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAdjustRefreshOverrides)
  {
    if (override.fallback != fallback)
      continue;

    //if we're checking for overrides, check if the fps matches
    if (!fallback && (fps < override.fpsmin || fps > override.fpsmax))
      continue;

    for (size_t j = RES_DESKTOP; j < CDisplaySettings::GetInstance().ResolutionInfoSize(); j++)
    {
      RESOLUTION_INFO info{gfxContext.GetResInfo(static_cast<RESOLUTION>(j))};

      if (info.iScreenWidth != curr.iScreenWidth || info.iScreenHeight != curr.iScreenHeight ||
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) != (curr.dwFlags & D3DPRESENTFLAG_MODEMASK) ||
          info.fRefreshRate > override.refreshmax || info.fRefreshRate < override.refreshmin)
        continue;

      resolution = static_cast<RESOLUTION>(j);

      if (fallback)
      {
        CLog::LogF(LOGDEBUG,
                   "Found Resolution {} ({}) from fallback (refreshmin:{:.3f} refreshmax:{:.3f})",
                   info.strMode, resolution, override.refreshmin, override.refreshmax);
      }
      else
      {
        CLog::LogF(LOGDEBUG,
                   "Found Resolution {} ({}) from override of fps {:.3f} (fpsmin:{:.3f} "
                   "fpsmax:{:.3f} refreshmin:{:.3f} refreshmax:{:.3f})",
                   info.strMode, resolution, fps, override.fpsmin, override.fpsmax,
                   override.refreshmin, override.refreshmax);
      }
      return true; //fps and refresh match with this override, use this resolution
    }
  }
  return false; //no override found
}

bool CResolutionUtils::HasWhitelist()
{
  return !CServiceBroker::GetSettingsComponent()
              ->GetSettings()
              ->GetList(CSettings::SETTING_VIDEOSCREEN_WHITELIST)
              .empty();
}

void CResolutionUtils::PrintWhitelist()
{
  const auto indexList{CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
      CSettings::SETTING_VIDEOSCREEN_WHITELIST)};

  if (indexList.empty())
    return;

  auto& displaySettings{CDisplaySettings::GetInstance()};
  const auto& gfxContext{CServiceBroker::GetWinSystem()->GetGfxContext()};
  std::string modeStr;
  for (const auto& mode : indexList)
  {
    const auto& i{displaySettings.GetResFromString(mode.asString())};
    const RESOLUTION_INFO& info{gfxContext.GetResInfo(i)};
    modeStr.append("\n" + info.strMode);
  }

  CLog::LogF(LOGDEBUG, "[WHITELIST] whitelisted modes:{}", modeStr);
}

void CResolutionUtils::GetMaxAllowedScreenResolution(unsigned int& width, unsigned int& height)
{
  if (!CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot())
    return;

  const auto indexList{CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
      CSettings::SETTING_VIDEOSCREEN_WHITELIST)};

  unsigned int maxWidth{0};
  unsigned int maxHeight{0};

  if (!indexList.empty())
  {
    auto& displaySettings{CDisplaySettings::GetInstance()};
    const auto& gfxContext{CServiceBroker::GetWinSystem()->GetGfxContext()};
    for (const auto& mode : indexList)
    {
      RESOLUTION res{displaySettings.GetResFromString(mode.asString())};
      RESOLUTION_INFO resInfo{gfxContext.GetResInfo(res)};
      if (std::cmp_greater(resInfo.iScreenWidth, maxWidth) &&
          std::cmp_greater(resInfo.iScreenHeight, maxHeight))
      {
        maxWidth = static_cast<unsigned int>(resInfo.iScreenWidth);
        maxHeight = static_cast<unsigned int>(resInfo.iScreenHeight);
      }
    }
  }
  else
  {
    std::vector<RESOLUTION> resList;
    auto& gfxContext{CServiceBroker::GetWinSystem()->GetGfxContext()};
    gfxContext.GetAllowedResolutions(resList);

    for (const auto& res : resList)
    {
      RESOLUTION_INFO resInfo{gfxContext.GetResInfo(res)};
      if (std::cmp_greater(resInfo.iScreenWidth, maxWidth) &&
          std::cmp_greater(resInfo.iScreenHeight, maxHeight))
      {
        maxWidth = static_cast<unsigned int>(resInfo.iScreenWidth);
        maxHeight = static_cast<unsigned int>(resInfo.iScreenHeight);
      }
    }
  }

  width = maxWidth;
  height = maxHeight;
}
