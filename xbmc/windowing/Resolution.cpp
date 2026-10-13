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

const char* SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN{"videoscreen.whitelistpulldown"};
const char* SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE{
    "videoscreen.whitelistdoublerefreshrate"};
const char* SETTING_VIDEOSCREEN_WHITELIST_MULTIPLEREFRESHRATE{
    "videoscreen.whitelistmultiplerefreshrate"};
const char* SETTING_VIDEOSCREEN_PREFER_HIGHER_REFRESH_RATES{
    ("videoscreen.preferhigherrefreshrates")};
const char* SETTING_VIDEOSCREEN_MATCH_VIDEO_RESOLUTION{("videoscreen.matchvideoresolution")};

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

RESOLUTION_INFO::RESOLUTION_INFO(int width, int height, float aspect, const std::string &mode) :
  strMode(mode)
{
  iWidth = width;
  iHeight = height;
  iBlanking = 0;
  iScreenWidth = width;
  iScreenHeight = height;
  fPixelRatio = aspect ? aspect / (((float)width) / height) : 1.0f;
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
  CGraphicContext& gfxContext = CServiceBroker::GetWinSystem()->GetGfxContext();
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  CLog::LogF(LOGINFO, "Searching the whitelist for: width: {}, height: {}, fps: {:0.3f}, 3D: {}",
             width, height, fps, is3D ? "true" : "false");

  std::vector<ResolutionCandidate> whitelist;
  for (const auto& mode : settings->GetList(CSettings::SETTING_VIDEOSCREEN_WHITELIST))
  {
    const RESOLUTION res = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    whitelist.push_back({res, gfxContext.GetResInfo(res)});
  }

  std::vector<ResolutionCandidate> allowed;
  if (whitelist.empty())
  {
    std::vector<RESOLUTION> resolutions;
    gfxContext.GetAllowedResolutions(resolutions);
    for (const auto res : resolutions)
      allowed.push_back({res, gfxContext.GetResInfo(res)});
  }

  WhitelistOptions options;
  options.allowDoubleRefreshRate =
      settings->GetBool(SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE);
  options.allowPulldown = settings->GetBool(SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN);
  options.useMultipleRefreshRates =
      settings->GetBool(SETTING_VIDEOSCREEN_WHITELIST_MULTIPLEREFRESHRATE);
  options.matchResolution = settings->GetBool(SETTING_VIDEOSCREEN_MATCH_VIDEO_RESOLUTION);

  const RESOLUTION chosen = SelectFromWhitelist(
      fps, width, height, gfxContext.GetResInfo(resolution),
      gfxContext.GetResInfo(CDisplaySettings::GetInstance().GetCurrentResolution()), whitelist,
      allowed, options);
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
  const bool noWhiteList = whitelist.empty();
  const bool matchResolution = options.matchResolution;

  std::vector<ResolutionCandidate> defaultWhitelist;
  if (noWhiteList)
  {
    CLog::LogF(LOGDEBUG, "Using the default whitelist because the user whitelist is empty");

    for (const auto& c : allowed)
    {
      const RESOLUTION_INFO& info = c.info;
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
  const std::vector<ResolutionCandidate>& modes = noWhiteList ? defaultWhitelist : whitelist;

  // Get additional parameters
  const bool allowDouble = noWhiteList || options.allowDoubleRefreshRate;
  const bool allowPulldown = noWhiteList || options.allowPulldown;
  const bool useMultiple = allowDouble && options.useMultipleRefreshRates;

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

  // Adjust weighting - without multiple refresh rates an exact refresh rate takes priority
  if (!useMultiple)
    MATCH_REFRESH_RATE = 32;

  std::vector<std::tuple<RESOLUTION, int, float, int>>
      resolutions; // index, resolution dfference, frequency, weighting

  CLog::LogF(LOGDEBUG, "Scanning available resolutions");
  for (const auto& [i, info] : modes)
  {
    CLog::LogF(LOGDEBUG, "Considering resolution {}", info.strMode);

    if ((info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (current.dwFlags & D3DPRESENTFLAG_MODEMASK))
    {
      float m{};
      const float f{std::modf(info.fRefreshRate / fps, &m)};
      const float p{std::modf(info.fRefreshRate / (fps * 2.5f), &m)};

      int w{0};
      if (MathUtils::FloatEquals(info.fRefreshRate, fps, FPS_PRECISION))
        w = MATCH_REFRESH_RATE;
      else if (allowDouble && MathUtils::FloatEquals(info.fRefreshRate, fps * 2.0f, FPS_PRECISION))
        w = MATCH_DOUBLE_REFRESH_RATE;
      else if (useMultiple && (f < FPS_MULTIPLE_PRECISION || 1.0f - f < FPS_MULTIPLE_PRECISION))
        w = MATCH_MULTIPLE_REFRESH_RATE;
      else if (allowPulldown &&
               (MathUtils::FloatEquals(info.fRefreshRate, fps * 2.5f, FPS_PRECISION) ||
                (useMultiple && (p < FPS_MULTIPLE_PRECISION || 1.0f - p < FPS_MULTIPLE_PRECISION))))
        w = MATCH_PULLDOWN_RATE;

      // only consider resolutions with a permitted refresh rate
      if (w == 0)
        continue;

      if (MatchesMode(width, height, info))
        w |= MATCH_RESOLUTION;
      else if (info.iScreenWidth == desktop.iScreenWidth &&
               info.iScreenHeight == desktop.iScreenHeight)
        w |= MATCH_DESKTOP_RESOLUTION;

      // Found resolution for consideration
      resolutions.emplace_back(std::make_tuple(
          i, std::abs(info.iScreenWidth - width) + std::abs(info.iScreenHeight - height),
          info.fRefreshRate, w));
      CLog::LogF(LOGDEBUG, "Resolution {} given weighting {} ({})", info.strMode, w, i);
    }
  }

  if (resolutions.empty())
  {
    CLog::LogF(LOGINFO, "No resolution matched");
    return RES_INVALID;
  }

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

  const auto modeName = [&modes](RESOLUTION res)
  {
    const auto it =
        std::find_if(modes.begin(), modes.end(),
                     [res](const ResolutionCandidate& c) { return c.resolution == res; });
    return it != modes.end() ? it->info.strMode : std::string();
  };

  // Log sorted list
  CLog::LogF(LOGDEBUG, "Sorted weighted resolution list");
  for (const auto& r : resolutions)
    CLog::LogF(LOGDEBUG, "Resolution {} Weighting {} ({})", modeName(std::get<0>(r)),
               std::get<3>(r), std::get<0>(r));

  // Choose resolution
  CLog::LogF(LOGINFO, "Chosen resolution {}", modeName(std::get<0>(resolutions[0])));
  return std::get<0>(resolutions[0]);
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
