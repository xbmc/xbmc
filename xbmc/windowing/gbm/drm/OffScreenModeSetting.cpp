/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OffScreenModeSetting.h"

#include "utils/log.h"

using namespace KODI::WINDOWING::GBM;

namespace
{
constexpr int DISPLAY_WIDTH = 1280;
constexpr int DISPLAY_HEIGHT = 720;
constexpr float DISPLAY_REFRESH = 50.0f;
} // namespace

bool COffScreenModeSetting::InitDrm()
{
  if (!CDRMUtils::OpenDrm(false))
    return false;

  CLog::Log(LOGDEBUG, "COffScreenModeSetting::{} - initialized offscreen DRM", __FUNCTION__);
  return true;
}

std::vector<RESOLUTION_INFO> COffScreenModeSetting::GetModes()
{
    std::vector<RESOLUTION_INFO> resolutions;
    resolutions.push_back(GetCurrentMode());
    return resolutions;
}

RESOLUTION_INFO COffScreenModeSetting::GetCurrentMode()
{
  RESOLUTION_INFO res;
  res.iScreenWidth = DISPLAY_WIDTH;
  res.iWidth = DISPLAY_WIDTH;
  res.iScreenHeight = DISPLAY_HEIGHT;
  res.iHeight = DISPLAY_HEIGHT;
  res.fRefreshRate = DISPLAY_REFRESH;
  res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
  res.fPixelRatio = 1.0f;
  res.bFullScreen = true;
  res.strId = "0";

  return res;
}
