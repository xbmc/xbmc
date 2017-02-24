/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <stdlib.h>

#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeAndroid.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "guilib/gui3d.h"
#include "platform/android/activity/XBMCApp.h"
#include "utils/StringUtils.h"
#include "androidjni/SystemProperties.h"
#include "androidjni/Display.h"
#include "androidjni/View.h"
#include "androidjni/Window.h"
#include "androidjni/WindowManager.h"
#include "androidjni/Build.h"
#include "androidjni/System.h"

#include "utils/SysfsUtils.h"

static bool s_hasModeApi = false;
static std::vector<RESOLUTION_INFO> s_res_displayModes;
static RESOLUTION_INFO s_res_cur_displayMode;

static float currentRefreshRate()
{
  if (s_hasModeApi)
    return s_res_cur_displayMode.fRefreshRate;

  CJNIWindow window = CXBMCApp::getWindow();
  if (window)
  {
    float preferredRate = window.getAttributes().getpreferredRefreshRate();
    if (preferredRate > 20.0 && preferredRate < 70.0)
    {
      CLog::Log(LOGINFO, "CEGLNativeTypeAndroid: Preferred refresh rate: %f", preferredRate);
      return preferredRate;
    }
    CJNIView view(window.getDecorView());
    if (view) {
      CJNIDisplay display(view.getDisplay());
      if (display)
      {
        float reportedRate = display.getRefreshRate();
        if (reportedRate > 20.0 && reportedRate < 70.0)
        {
          CLog::Log(LOGINFO, "CEGLNativeTypeAndroid: Current display refresh rate: %f", reportedRate);
          return reportedRate;
        }
      }
    }
  }
  CLog::Log(LOGDEBUG, "found no refresh rate");
  return 60.0;
}

static void fetchDisplayModes()
{
  s_hasModeApi = false;
  s_res_displayModes.clear();

  CJNIDisplay display = CXBMCApp::getWindow().getDecorView().getDisplay();

  if (display)
  {
    CJNIDisplayMode m = display.getMode();
    if (m)
    {
      if (m.getPhysicalWidth() > m.getPhysicalHeight())   // Assume unusable if portrait is returned
      {
        s_hasModeApi = true;

        CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: current mode: %d: %dx%d@%f", m.getModeId(), m.getPhysicalWidth(), m.getPhysicalHeight(), m.getRefreshRate());
        s_res_cur_displayMode.strId = StringUtils::Format("%d", m.getModeId());
        s_res_cur_displayMode.iWidth = s_res_cur_displayMode.iScreenWidth = m.getPhysicalWidth();
        s_res_cur_displayMode.iHeight = s_res_cur_displayMode.iScreenHeight = m.getPhysicalHeight();
        s_res_cur_displayMode.fRefreshRate = m.getRefreshRate();
        s_res_cur_displayMode.dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
        s_res_cur_displayMode.iScreen       = 0;
        s_res_cur_displayMode.bFullScreen   = true;
        s_res_cur_displayMode.iSubtitles    = (int)(0.965 * s_res_cur_displayMode.iHeight);
        s_res_cur_displayMode.fPixelRatio   = 1.0f;
        s_res_cur_displayMode.strMode       = StringUtils::Format("%dx%d @ %.6f%s - Full Screen", s_res_cur_displayMode.iScreenWidth, s_res_cur_displayMode.iScreenHeight, s_res_cur_displayMode.fRefreshRate,
                                                                  s_res_cur_displayMode.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

        std::vector<CJNIDisplayMode> modes = display.getSupportedModes();
        for (auto m : modes)
        {
          CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: available mode: %d: %dx%d@%f", m.getModeId(), m.getPhysicalWidth(), m.getPhysicalHeight(), m.getRefreshRate());

          RESOLUTION_INFO res;
          res.strId = StringUtils::Format("%d", m.getModeId());
          res.iWidth = res.iScreenWidth = m.getPhysicalWidth();
          res.iHeight = res.iScreenHeight = m.getPhysicalHeight();
          res.fRefreshRate = m.getRefreshRate();
          res.dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
          res.iScreen       = 0;
          res.bFullScreen   = true;
          res.iSubtitles    = (int)(0.965 * res.iHeight);
          res.fPixelRatio   = 1.0f;
          res.strMode       = StringUtils::Format("%dx%d @ %.6f%s - Full Screen", res.iScreenWidth, res.iScreenHeight, res.fRefreshRate,
                                                  res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

          s_res_displayModes.push_back(res);
        }
      }
    }
  }
}

CEGLNativeTypeAndroid::CEGLNativeTypeAndroid()
  : m_width(0), m_height(0)
{
}

CEGLNativeTypeAndroid::~CEGLNativeTypeAndroid()
{
}

bool CEGLNativeTypeAndroid::CheckCompatibility()
{
  return true;
}

void CEGLNativeTypeAndroid::Initialize()
{
  std::string displaySize;
  m_width = m_height = 0;

  if (CJNIBuild::DEVICE != "foster" || CJNIBase::GetSDKVersion() >= 24)   // Buggy implementation of DisplayMode API on SATV
  {
    fetchDisplayModes();
    for (auto res : s_res_displayModes)
    {
      if (res.iWidth > m_width || res.iHeight > m_height)
      {
        m_width = res.iWidth;
        m_height = res.iHeight;
      }
    }
  }

  if (!m_width || !m_height)
  {
    // Property available on some devices
    displaySize = CJNISystemProperties::get("sys.display-size", "");
    if (!displaySize.empty())
    {
      std::vector<std::string> aSize = StringUtils::Split(displaySize, "x");
      if (aSize.size() == 2)
      {
        m_width = StringUtils::IsInteger(aSize[0]) ? atoi(aSize[0].c_str()) : 0;
        m_height = StringUtils::IsInteger(aSize[1]) ? atoi(aSize[1].c_str()) : 0;
      }
      CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: display-size: %s(%dx%d)", displaySize.c_str(), m_width, m_height);
    }
  }

  CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: maximum/current resolution: %dx%d", m_width, m_height);
  int limit = CServiceBroker::GetSettings().GetInt("videoscreen.limitgui");
  switch (limit)
  {
    case 0: // auto
      m_width = 0;
      m_height = 0;
      break;

    case 9999:  // unlimited
      break;

    case 720:
      if (m_height > 720)
      {
        m_width = 1280;
        m_height = 720;
      }
      break;

    case 1080:
      if (m_height > 1080)
      {
        m_width = 1920;
        m_height = 1080;
      }
      break;
  }
  CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: selected resolution: %dx%d", m_width, m_height);
}

void CEGLNativeTypeAndroid::Destroy()
{
  return;
}

bool CEGLNativeTypeAndroid::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeAndroid::CreateNativeWindow()
{
  // Android hands us a window, we don't have to create it
  return true;
}

bool CEGLNativeTypeAndroid::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeAndroid::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) CXBMCApp::GetNativeWindow(2000);
  return (*nativeWindow != NULL && **nativeWindow != NULL);
}

bool CEGLNativeTypeAndroid::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeAndroid::DestroyNativeWindow()
{
  return true;
}

bool CEGLNativeTypeAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
  EGLNativeWindowType *nativeWindow = (EGLNativeWindowType*)CXBMCApp::GetNativeWindow(30000);
  if (!nativeWindow)
    return false;

  if (!*nativeWindow)
    return false;

  if (s_hasModeApi)
  {
    *res = s_res_cur_displayMode;
    return true;
  }

  if (!m_width || !m_height)
  {
    ANativeWindow_acquire(*nativeWindow);
    res->iWidth = ANativeWindow_getWidth(*nativeWindow);
    res->iHeight= ANativeWindow_getHeight(*nativeWindow);
    ANativeWindow_release(*nativeWindow);
  }
  else
  {
    res->iWidth = m_width;
    res->iHeight = m_height;
  }

  res->strId = "-1";
  res->fRefreshRate = currentRefreshRate();
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode       = StringUtils::Format("%dx%d @ %.6f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
                                           res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGNOTICE,"CEGLNativeTypeAndroid: Current resolution: %s\n",res->strMode.c_str());
  return true;
}

bool CEGLNativeTypeAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: SetNativeResolution: %s: %dx%d@%f", res.strId.c_str(), res.iWidth, res.iHeight, res.fRefreshRate);


  if (s_hasModeApi)
  {
    CXBMCApp::SetDisplayMode(atoi(res.strId.c_str()));
    s_res_cur_displayMode = res;
  }
  else if (abs(currentRefreshRate() - res.fRefreshRate) > 0.0001)
    CXBMCApp::SetRefreshRate(res.fRefreshRate);
  CXBMCApp::SetBuffersGeometry(res.iWidth, res.iHeight, 0);

  return true;
}

bool CEGLNativeTypeAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  if (s_hasModeApi)
  {
    resolutions.insert(resolutions.end(), s_res_displayModes.begin(), s_res_displayModes.end());
    return true;
  }

  RESOLUTION_INFO res;
  bool ret = GetNativeResolution(&res);

  if (ret && res.iWidth > 1 && res.iHeight > 1)
  {
    std::vector<float> refreshRates;
    CJNIWindow window = CXBMCApp::getWindow();
    if (window)
    {
      CJNIView view = window.getDecorView();
      if (view)
      {
        CJNIDisplay display = view.getDisplay();
        if (display)
        {
          refreshRates = display.getSupportedRefreshRates();
        }
      }

      if (!refreshRates.empty())
      {
        for (unsigned int i = 0; i < refreshRates.size(); i++)
        {
          if (refreshRates[i] < 20.0 || refreshRates[i] > 70.0)
            continue;
          res.fRefreshRate = refreshRates[i];
          res.strMode      = StringUtils::Format("%dx%d @ %.6f%s - Full Screen", res.iScreenWidth, res.iScreenHeight, res.fRefreshRate,
                                                 res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
          resolutions.push_back(res);
        }
      }
    }
    if (resolutions.empty())
    {
      /* No valid refresh rates available, just provide the current one */
      resolutions.push_back(res);
    }
    return true;
  }
  return false;
}

bool CEGLNativeTypeAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeAndroid::ShowWindow(bool show)
{
  return false;
}
