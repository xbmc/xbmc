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

#include <algorithm>
#include <cmath>
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

// allow hardware decoder surface padding due to codec block alignment and GPU requirements
// (height has greater tolerance due to 32/64px boundaries e.g. 1080→1088 or 2160→2176) and
// encoder cropping of up to 2% e.g. 3840→3832
bool MatchesMode(int width, int height, const RESOLUTION_INFO& info)
{
  const auto withinTolerance = [](int v, int screen, int pad)
  { return v <= screen + pad && v >= screen * 0.98; };

  return (withinTolerance(height, info.iScreenHeight, 32) && width <= info.iScreenWidth + 8) ||
         (withinTolerance(width, info.iScreenWidth, 8) && height <= info.iScreenHeight + 32);
}

} // namespace

EdgeInsets::EdgeInsets(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b)
{
}

RESOLUTION_INFO::RESOLUTION_INFO(int width, int height, float aspect, std::string mode)
  : iWidth(width),
    iHeight(height),
    iScreenWidth(width),
    iScreenHeight(height),
    fPixelRatio(aspect > 0.0f ? aspect / (static_cast<float>(width) / static_cast<float>(height))
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
  CGraphicContext& gfxContext{CServiceBroker::GetWinSystem()->GetGfxContext()};
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};

  CLog::LogF(LOGINFO, "Searching the whitelist for: width: {}, height: {}, fps: {:0.3f}, 3D: {}",
             width, height, fps, is3D ? "true" : "false");

  std::vector<ResolutionCandidate> whitelist;
  for (const auto& mode : settings->GetList(CSettings::SETTING_VIDEOSCREEN_WHITELIST))
  {
    const RESOLUTION res{CDisplaySettings::GetInstance().GetResFromString(mode.asString())};
    whitelist.push_back({.resolution = res, .info = gfxContext.GetResInfo(res)});
  }

  std::vector<ResolutionCandidate> allowed;
  if (whitelist.empty())
  {
    std::vector<RESOLUTION> resolutions;
    gfxContext.GetAllowedResolutions(resolutions);
    for (const auto res : resolutions)
      allowed.push_back({.resolution = res, .info = gfxContext.GetResInfo(res)});
  }

  const WhitelistOptions options{
      .allowDoubleRefreshRate = settings->GetBool(SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE),
      .allowPulldown = settings->GetBool(SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN),
      .useMultipleRefreshRates =
          settings->GetBool(SETTING_VIDEOSCREEN_WHITELIST_MULTIPLEREFRESHRATE),
      .matchResolution = settings->GetBool(SETTING_VIDEOSCREEN_MATCH_VIDEO_RESOLUTION)};

  const RESOLUTION chosen{SelectFromWhitelist(
      fps, width, height, gfxContext.GetResInfo(resolution),
      gfxContext.GetResInfo(CDisplaySettings::GetInstance().GetCurrentResolution()), whitelist,
      allowed, options)};
  if (chosen != RES_INVALID)
    resolution = chosen;
}

RESOLUTION CResolutionUtils::SelectFromWhitelist(float fps,
                                                 int width,
                                                 int height,
                                                 const RESOLUTION_INFO& current,
                                                 const RESOLUTION_INFO& desktop,
                                                 const std::vector<ResolutionCandidate>& whitelist,
                                                 const std::vector<ResolutionCandidate>& allowed,
                                                 const WhitelistOptions& options)
{
  const bool noWhiteList{whitelist.empty()};
  const bool matchResolution{options.matchResolution};

  std::vector<ResolutionCandidate> defaultWhitelist;
  if (noWhiteList)
  {
    CLog::LogF(LOGDEBUG, "Using the default whitelist because the user whitelist is empty");

    for (const auto& c : allowed)
    {
      const RESOLUTION_INFO& info{c.info};
      const bool largeEnough{info.iScreenHeight >=
                                 (matchResolution ? height : current.iScreenHeight) &&
                             info.iScreenWidth >= (matchResolution ? width : current.iScreenWidth)};
      if ((largeEnough || (matchResolution && MatchesMode(width, height, info))) &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (current.dwFlags & D3DPRESENTFLAG_MODEMASK))
      {
        // do not add half refreshrates (25, 29.97 by default) as kodi cannot cope with
        // them on playback start. Especially interlaced content is not properly detected
        // and this causes ugly double switching.
        // This won't allow 25p / 30p playback on native refreshrate by default
        if ((info.fRefreshRate > 30) || (MathUtils::FloatEquals(info.fRefreshRate, 24.0f, 0.1f)))
          defaultWhitelist.push_back(c);
      }
    }
  }
  const std::vector<ResolutionCandidate>& modes{noWhiteList ? defaultWhitelist : whitelist};

  // Get additional parameters
  const bool allowDouble{noWhiteList || options.allowDoubleRefreshRate};
  const bool allowPulldown{noWhiteList || options.allowPulldown};
  const bool useMultiple{allowDouble && options.useMultipleRefreshRates};

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
    NO_MATCH = 0
  };

  // Adjust weighting
  using enum Weighting;
  // without multiple refresh rates an exact refresh rate takes priority over double
  const int refreshRateMatch{!useMultiple ? static_cast<int>(MATCH_MULTIPLE_REFRESH_RATE)
                                          : static_cast<int>(MATCH_REFRESH_RATE)};

  std::vector<ResolutionEntry> resolutions;

  CLog::LogF(LOGDEBUG, "Scanning available resolutions");
  for (const auto& [i, info] : modes)
  {
    CLog::LogF(LOGDEBUG, "Considering resolution {}", info.strMode);

    if ((info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (current.dwFlags & D3DPRESENTFLAG_MODEMASK))
    {
      float m{};
      const float f{std::modf(info.fRefreshRate / fps, &m)};
      const float p{std::modf(info.fRefreshRate / (fps * 2.5f), &m)};

      int w{static_cast<int>(NO_MATCH)};
      if (MathUtils::FloatEquals(info.fRefreshRate, fps, FPS_PRECISION))
        w = refreshRateMatch;
      else if (allowDouble && MathUtils::FloatEquals(info.fRefreshRate, fps * 2.0f, FPS_PRECISION))
        w = static_cast<int>(MATCH_DOUBLE_REFRESH_RATE);
      else if (useMultiple && (f < FPS_MULTIPLE_PRECISION || 1.0f - f < FPS_MULTIPLE_PRECISION))
        w = static_cast<int>(MATCH_MULTIPLE_REFRESH_RATE);
      else if (allowPulldown &&
               (MathUtils::FloatEquals(info.fRefreshRate, fps * 2.5f, FPS_PRECISION) ||
                (useMultiple && (p < FPS_MULTIPLE_PRECISION || 1.0f - p < FPS_MULTIPLE_PRECISION))))
        w = static_cast<int>(MATCH_PULLDOWN_RATE);

      // only consider resolutions with a permitted refresh rate
      if (w == static_cast<int>(NO_MATCH))
        continue;

      if (MatchesMode(width, height, info))
        w |= static_cast<int>(MATCH_RESOLUTION);
      else if (info.iScreenWidth == desktop.iScreenWidth &&
               info.iScreenHeight == desktop.iScreenHeight)
        w |= static_cast<int>(MATCH_DESKTOP_RESOLUTION);

      // Found resolution for consideration
      resolutions.emplace_back(
          ResolutionEntry{.index = i,
                          .resolutionDifference = std::abs(info.iScreenWidth - width) +
                                                  std::abs(info.iScreenHeight - height),
                          .frequency = info.fRefreshRate,
                          .weighting = w});
      CLog::LogF(LOGDEBUG, "Resolution {} given weighting {} ({})", info.strMode, w, i);
    }
  }

  if (resolutions.empty())
  {
    CLog::LogF(LOGINFO, "No resolution matched");
    return RES_INVALID;
  }

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

  const auto modeName{
      [&modes](RESOLUTION res)
      {
        const auto it{std::ranges::find(modes, res, &ResolutionCandidate::resolution)};
        return it != modes.end() ? it->info.strMode : std::string{};
      }};

  // Log sorted list
  CLog::LogF(LOGDEBUG, "Sorted weighted resolution list");
  for (const auto& r : resolutions)
    CLog::LogF(LOGDEBUG, "Resolution {} Weighting {} ({})", modeName(r.index), r.weighting,
               r.index);

  // Choose resolution
  CLog::LogF(LOGINFO, "Chosen resolution {}", modeName(resolutions[0].index));
  return resolutions[0].index;
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
