/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "WinSystemGbm.h"

#include <string.h>

#include "guilib/GraphicContext.h"
#include "settings/DisplaySettings.h"
#include "utils/log.h"

CWinSystemGbm::CWinSystemGbm() :
  m_gbm(nullptr),
  m_drm(nullptr),
  m_nativeDisplay(nullptr),
  m_nativeWindow(nullptr)
{
  m_eWindowSystem = WINDOW_SYSTEM_GBM;
}

bool CWinSystemGbm::InitWindowSystem()
{
  if (!CGBMUtils::InitDrm())
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::%s - failed to initialize DRM", __FUNCTION__);
    return false;
  }

  m_drm = CGBMUtils::GetDrm();
  m_gbm = CGBMUtils::GetGbm();

  m_nativeDisplay = m_gbm->dev;

  if (!m_drm)
  {
    return false;
  }

  if (!m_gbm)
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - initialized DRM", __FUNCTION__);
  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemGbm::DestroyWindowSystem()
{
  CGBMUtils::DestroyDrm();
  m_nativeDisplay = nullptr;
  m_drm = nullptr;
  m_gbm = nullptr;

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - deinitialized DRM", __FUNCTION__);
  return true;
}

bool CWinSystemGbm::CreateNewWindow(const std::string& name,
                                    bool fullScreen,
                                    RESOLUTION_INFO& res)
{
  if (!CGBMUtils::InitGbm(res))
  {
    CLog::Log(LOGERROR, "CWinSystemGbm::%s - failed to initialize GBM", __FUNCTION__);
    return false;
  }

  m_nativeWindow = m_gbm->surface;

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - initialized GBM", __FUNCTION__);
  return true;
}

bool CWinSystemGbm::DestroyWindow()
{
  CGBMUtils::DestroyGbm();
  m_nativeWindow = nullptr;

  CLog::Log(LOGDEBUG, "CWinSystemGbm::%s - deinitialized GBM", __FUNCTION__);
  return true;
}

void CWinSystemGbm::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP),
                          0,
                          m_drm->mode->hdisplay,
                          m_drm->mode->vdisplay,
                          m_drm->mode->vrefresh);

  std::vector<RESOLUTION_INFO> resolutions;

  if (!CGBMUtils::GetModes(resolutions) || resolutions.empty())
  {
    CLog::Log(LOGWARNING, "CWinSystemGbm::%s - Failed to get resolutions", __FUNCTION__);
  }
  else
  {
    for (auto i = 0; i < resolutions.size(); i++)
    {
      g_graphicsContext.ResetOverscan(resolutions[i]);
      CDisplaySettings::GetInstance().AddResolutionInfo(resolutions[i]);

      CLog::Log(LOGNOTICE, "Found resolution for display %d with %dx%d%s @ %f Hz",
                resolutions[i].iScreen,
                resolutions[i].iScreenWidth,
                resolutions[i].iScreenHeight,
                resolutions[i].dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "",
                resolutions[i].fRefreshRate);
    }
  }

  CDisplaySettings::GetInstance().ApplyCalibrations();
}

bool CWinSystemGbm::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  return true;
}

bool CWinSystemGbm::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  auto ret = CGBMUtils::SetVideoMode(res);

  if (!ret)
  {
    return false;
  }

  return true;
}

bool CWinSystemGbm::Hide()
{
  return false;
}

bool CWinSystemGbm::Show(bool raise)
{
  return true;
}

void CWinSystemGbm::Register(IDispResource * /*resource*/)
{
}

void CWinSystemGbm::Unregister(IDispResource * /*resource*/)
{
}
