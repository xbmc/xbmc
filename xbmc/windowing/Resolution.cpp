/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Resolution.h"
#include "guilib/gui3d.h"
#include "GraphicContext.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "ServiceBroker.h"

#include <cstdlib>

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

RESOLUTION_INFO::RESOLUTION_INFO(const RESOLUTION_INFO& res) :
  Overscan(res.Overscan),
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
  float weight;

  if (!FindResolutionFromOverride(fps, width, is3D, res, weight, false)) //find a refreshrate from overrides
  {
    if (!FindResolutionFromOverride(fps, width, is3D, res, weight, true)) //if that fails find it from a fallback
    {
      FindResolutionFromWhitelist(fps, width, height, is3D, res); //find a refreshrate from whitelist
    }
  }

  CLog::Log(LOGNOTICE, "Display resolution ADJUST : %s (%d) (weight: %.3f)",
            CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(res).strMode.c_str(), res, weight);
  return res;
}

void CResolutionUtils::FindResolutionFromWhitelist(float fps, int width, int height, bool is3D, RESOLUTION &resolution)
{
  RESOLUTION_INFO curr = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(resolution);

  std::vector<CVariant> indexList = CServiceBroker::GetSettings().GetList(CSettings::SETTING_VIDEOSCREEN_WHITELIST);

  CLog::Log(LOGDEBUG, "Trying to find exact refresh rate");

  for (const auto &mode : indexList)
  {
    auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

    // allow resolutions that are exact and have the correct refresh rate
    if (((height == info.iScreenHeight && width <= info.iScreenWidth) ||
         (width == info.iScreenWidth && height <= info.iScreenHeight)) &&
        (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
        MathUtils::FloatEquals(info.fRefreshRate, fps, 0.01f))
    {
      CLog::Log(LOGDEBUG, "Matched exact whitelisted Resolution %s (%d)", info.strMode.c_str(), i);
      resolution = i;
      return;
    }
  }

  CLog::Log(LOGDEBUG, "No exact whitelisted resolution matched, trying double refresh rate");

  for (const auto &mode : indexList)
  {
    auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

    // allow resolutions that are exact and have double the refresh rate
    if (((height == info.iScreenHeight && width <= info.iScreenWidth) ||
         (width == info.iScreenWidth && height <= info.iScreenHeight)) &&
        (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (curr.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
        MathUtils::FloatEquals(info.fRefreshRate, fps * 2, 0.01f))
    {
      CLog::Log(LOGDEBUG, "Matched fuzzy whitelisted Resolution %s (%d)", info.strMode.c_str(), i);
      resolution = i;
      return;
    }
  }

  CLog::Log(LOGDEBUG, "No double refresh rate whitelisted resolution matched, trying current resolution");

  for (const auto &mode : indexList)
  {
    auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

    const RESOLUTION_INFO desktop_info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(CDisplaySettings::GetInstance().GetCurrentResolution());

    // allow resolutions that are desktop resolution but have the correct refresh rate
    if (info.iScreenWidth == desktop_info.iWidth &&
        (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (desktop_info.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
        MathUtils::FloatEquals(info.fRefreshRate, fps, 0.01f))
    {
      CLog::Log(LOGDEBUG, "Matched fuzzy whitelisted Resolution %s (%d)", info.strMode.c_str(), i);
      resolution = i;
      return;
    }
  }

  CLog::Log(LOGDEBUG, "No larger whitelisted resolution matched, trying current resolution with double refreshrate");

  for (const auto &mode : indexList)
  {
    auto i = CDisplaySettings::GetInstance().GetResFromString(mode.asString());
    const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(i);

    const RESOLUTION_INFO desktop_info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(CDisplaySettings::GetInstance().GetCurrentResolution());

    // allow resolutions that are desktop resolution but have double the refresh rate
    if (info.iScreenWidth == desktop_info.iWidth &&
        (info.dwFlags & D3DPRESENTFLAG_MODEMASK) == (desktop_info.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
        MathUtils::FloatEquals(info.fRefreshRate, fps * 2, 0.01f))
    {
      CLog::Log(LOGDEBUG, "Matched fuzzy whitelisted Resolution %s (%d)", info.strMode.c_str(), i);
      resolution = i;
      return;
    }
  }

  CLog::Log(LOGDEBUG, "No whitelisted resolution matched");
}

bool CResolutionUtils::FindResolutionFromOverride(float fps, int width, bool is3D, RESOLUTION &resolution, float& weight, bool fallback)
{
  RESOLUTION_INFO curr = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(resolution);

  //try to find a refreshrate from the override
  for (int i = 0; i < (int)g_advancedSettings.m_videoAdjustRefreshOverrides.size(); i++)
  {
    RefreshOverride& override = g_advancedSettings.m_videoAdjustRefreshOverrides[i];

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
            CLog::Log(LOGDEBUG, "Found Resolution %s (%d) from fallback (refreshmin:%.3f refreshmax:%.3f)",
                      info.strMode.c_str(), resolution,
                      override.refreshmin, override.refreshmax);
          }
          else
          {
            CLog::Log(LOGDEBUG, "Found Resolution %s (%d) from override of fps %.3f (fpsmin:%.3f fpsmax:%.3f refreshmin:%.3f refreshmax:%.3f)",
                      info.strMode.c_str(), resolution, fps,
                      override.fpsmin, override.fpsmax, override.refreshmin, override.refreshmax);
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
  int   round = MathUtils::round_int(div);

  float weight = 0.0f;

  if (round < 1)
    weight = (fps - refresh) / fps;
  else
    weight = (float)fabs(div / round - 1.0);

  // punish higher refreshrates and prefer better matching
  // e.g. 30 fps content at 60 hz is better than
  // 30 fps at 120 hz - as we sometimes don't know if
  // the content is interlaced at the start, only
  // punish when refreshrate > 60 hz to not have to switch
  // twice for 30i content
  if (refresh > 60 && round > 1)
    weight += round / 10000.0;

  return weight;
}

bool CResolutionUtils::HasWhitelist()
{
  std::vector<CVariant> indexList = CServiceBroker::GetSettings().GetList(CSettings::SETTING_VIDEOSCREEN_WHITELIST);
  return !indexList.empty();
}
