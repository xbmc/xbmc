/*
 *  Copyright (C) 2005-2018 Team Kodi
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

#include <cstdlib>
#include <limits>

namespace
{

const char* SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN{"videoscreen.whitelistpulldown"};
const char* SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE{
    "videoscreen.whitelistdoublerefreshrate"};

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

RESOLUTION_INFO::RESOLUTION_INFO(const RESOLUTION_INFO& res)
  : Overscan(res.Overscan),
    guiInsets(res.guiInsets),
    strMode(res.strMode),
    strOutput(res.strOutput),
    strId(res.strId)
{
  bFullScreen = res.bFullScreen;
  iWidth = res.iWidth; iHeight = res.iHeight;
  iScreenWidth = res.iScreenWidth; iScreenHeight = res.iScreenHeight;
  iSubtitles = res.iSubtitles; dwFlags = res.dwFlags;
  fPixelRatio = res.fPixelRatio; fRefreshRate = res.fRefreshRate;
  iBlanking = res.iBlanking;
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

void CResolutionUtils::FindResolutionFromWhitelist(float fps, int width, int height, bool is3D, RESOLUTION &resolution)
{
  RESOLUTION_INFO curr = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(resolution);
  CLog::Log(LOGINFO,
            "[WHITELIST] Searching the whitelist for: width: {}, height: {}, fps: {:0.3f}, 3D: {}",
            width, height, fps, is3D ? "true" : "false");

  std::vector<CVariant> indexList = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(CSettings::SETTING_VIDEOSCREEN_WHITELIST);

  bool noWhiteList = indexList.empty();

  if (noWhiteList)
  {
    CLog::Log(LOGDEBUG,
              "[WHITELIST] Using the default whitelist because the user whitelist is empty");
    std::vector<RESOLUTION> candidates;
    RESOLUTION_INFO info;
    std::string resString;
    CServiceBroker::GetWinSystem()->GetGfxContext().GetAllowedResolutions(candidates);
    for (const auto& c : candidates)
    {
      info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(c);
      if (info.iScreenHeight >= curr.iScreenHeight && info.iScreenWidth >= curr.iScreenWidth &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK))
      {
        // do not add half refreshrates (25, 29.97 by default) as kodi cannot cope with
        // them on playback start. Especially interlaced content is not properly detected
        // and this causes ugly double switching.
        // This won't allow 25p / 30p playback on native refreshrate by default
        if ((info.fRefreshRate > 30) || (MathUtils::FloatEquals(info.fRefreshRate, 24.0f, 0.1f)))
        {
          resString = CDisplaySettings::GetInstance().GetStringFromRes(c);
          indexList.emplace_back(resString);
        }
      }
    }
  }

  CLog::Log(LOGDEBUG, "[WHITELIST] Searching for an exact resolution with an exact refresh rate");

  unsigned int penalty = std::numeric_limits<unsigned int>::max();
  bool found = false;

  for (const auto& mode : indexList)
  {
    auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

    // allow resolutions that are exact and have the correct refresh rate
    // allow macroblock alignment / padding errors (e.g. 1080 mod16 == 8)
    if (((height == info.iScreenHeight && width <= info.iScreenWidth + 8) ||
         (width == info.iScreenWidth && height <= info.iScreenHeight + 8)) &&
        (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
        MathUtils::FloatEquals(info.fRefreshRate, fps, 0.01f))
    {
      CLog::Log(LOGDEBUG,
                "[WHITELIST] Matched an exact resolution with an exact refresh rate {} ({})",
                info.strMode, i);
      unsigned int pen = abs(info.iScreenHeight - height) + abs(info.iScreenWidth - width);
      if (pen < penalty)
      {
        resolution = i;
        found = true;
        penalty = pen;
      }
    }
  }

  if (!found)
    CLog::Log(LOGDEBUG, "[WHITELIST] No match for an exact resolution with an exact refresh rate");

  if (noWhiteList || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE))
  {
    CLog::Log(LOGDEBUG,
              "[WHITELIST] Searching for an exact resolution with double the refresh rate");

    for (const auto& mode : indexList)
    {
      auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
      const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

      // allow resolutions that are exact and have double the refresh rate
      // allow macroblock alignment / padding errors (e.g. 1080 mod16 == 8)
      if (((height == info.iScreenHeight && width <= info.iScreenWidth + 8) ||
           (width == info.iScreenWidth && height <= info.iScreenHeight + 8)) &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
          MathUtils::FloatEquals(info.fRefreshRate, fps * 2, 0.01f))
      {
        CLog::Log(LOGDEBUG,
                  "[WHITELIST] Matched an exact resolution with double the refresh rate {} ({})",
                  info.strMode, i);
        unsigned int pen = abs(info.iScreenHeight - height) + abs(info.iScreenWidth - width);
        if (pen < penalty)
        {
          resolution = i;
          found = true;
          penalty = pen;
        }
      }
    }
    if (found)
      return;

    CLog::Log(LOGDEBUG,
              "[WHITELIST] No match for an exact resolution with double the refresh rate");
  }
  else if (found)
    return;

  if (noWhiteList || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN))
  {
    CLog::Log(LOGDEBUG,
              "[WHITELIST] Searching for an exact resolution with a 3:2 pulldown refresh rate");

    for (const auto& mode : indexList)
    {
      auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
      const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

      // allow resolutions that are exact and have 2.5 times the refresh rate
      // allow macroblock alignment / padding errors (e.g. 1080 mod16 == 8)
      if (((height == info.iScreenHeight && width <= info.iScreenWidth + 8) ||
           (width == info.iScreenWidth && height <= info.iScreenHeight + 8)) &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
          MathUtils::FloatEquals(info.fRefreshRate, fps * 2.5f, 0.01f))
      {
        CLog::Log(
            LOGDEBUG,
            "[WHITELIST] Matched an exact resolution with a 3:2 pulldown refresh rate {} ({})",
            info.strMode, i);
        unsigned int pen = abs(info.iScreenHeight - height) + abs(info.iScreenWidth - width);
        if (pen < penalty)
        {
          resolution = i;
          found = true;
          penalty = pen;
        }
      }
    }
    if (found)
      return;

    CLog::Log(LOGDEBUG, "[WHITELIST] No match for a resolution with a 3:2 pulldown refresh rate");
  }


  CLog::Log(LOGDEBUG, "[WHITELIST] Searching for a desktop resolution with an exact refresh rate");

  const RESOLUTION_INFO desktop_info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(CDisplaySettings::GetInstance().GetCurrentResolution());

  for (const auto& mode : indexList)
  {
    auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

    // allow resolutions that are desktop resolution but have the correct refresh rate
    if (info.iScreenWidth == desktop_info.iScreenWidth &&
        (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (desktop_info.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
        MathUtils::FloatEquals(info.fRefreshRate, fps, 0.01f))
    {
      CLog::Log(LOGDEBUG,
                "[WHITELIST] Matched a desktop resolution with an exact refresh rate {} ({})",
                info.strMode, i);
      resolution = i;
      return;
    }
  }

  CLog::Log(LOGDEBUG, "[WHITELIST] No match for a desktop resolution with an exact refresh rate");

  if (noWhiteList || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          SETTING_VIDEOSCREEN_WHITELIST_DOUBLEREFRESHRATE))
  {
    CLog::Log(LOGDEBUG,
              "[WHITELIST] Searching for a desktop resolution with double the refresh rate");

    for (const auto& mode : indexList)
    {
      auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
      const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

      // allow resolutions that are desktop resolution but have double the refresh rate
      if (info.iScreenWidth == desktop_info.iScreenWidth &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) ==
              (desktop_info.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
          MathUtils::FloatEquals(info.fRefreshRate, fps * 2, 0.01f))
      {
        CLog::Log(LOGDEBUG,
                  "[WHITELIST] Matched a desktop resolution with double the refresh rate {} ({})",
                  info.strMode, i);
        resolution = i;
        return;
      }
    }

    CLog::Log(LOGDEBUG,
              "[WHITELIST] No match for a desktop resolution with double the refresh rate");
  }

  if (noWhiteList || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          SETTING_VIDEOSCREEN_WHITELIST_PULLDOWN))
  {
    CLog::Log(LOGDEBUG,
              "[WHITELIST] Searching for a desktop resolution with a 3:2 pulldown refresh rate");

    for (const auto& mode : indexList)
    {
      auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
      const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

      // allow resolutions that are desktop resolution but have 2.5 times the refresh rate
      if (info.iScreenWidth == desktop_info.iScreenWidth &&
          (info.dwFlags & D3DPRESENTFLAG_MODEMASK) ==
              (desktop_info.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
          MathUtils::FloatEquals(info.fRefreshRate, fps * 2.5f, 0.01f))
      {
        CLog::Log(
            LOGDEBUG,
            "[WHITELIST] Matched a desktop resolution with a 3:2 pulldown refresh rate {} ({})",
            info.strMode, i);
        resolution = i;
        return;
      }
    }

    CLog::Log(LOGDEBUG,
              "[WHITELIST] No match for a desktop resolution with a 3:2 pulldown refresh rate");
  }

  CLog::Log(LOGDEBUG, "[WHITELIST] No resolution matched");
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
