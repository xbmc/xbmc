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
#include "utils/log.h"
#include "guilib/gui3d.h"
#include "android/activity/XBMCApp.h"
#include "utils/StringUtils.h"
#include "android/jni/SystemProperties.h"
#include "android/jni/Display.h"
#include "android/jni/View.h"
#include "android/jni/Window.h"
#include "android/jni/WindowManager.h"
#include "android/jni/Build.h"
#include "android/jni/System.h"

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

static bool DeviceCanUseDisplaysize(const std::string &name)
{
  // Devices that can render GUI in 4K
  static const char *devicecanusedisplaysize[] = {
    "foster",
    NULL
  };

  for (const char **ptr = devicecanusedisplaysize; *ptr; ptr++)
  {
    if (!strnicmp(*ptr, name.c_str(), strlen(*ptr)))
      return true;
  }
  return false;
}

void CEGLNativeTypeAndroid::Initialize()
{
  std::string displaySize;
  m_width = m_height = 0;

  // FIXME: Temporary shield specific hack to obtain HDMI resolution
  //        Remove and use New Android M API
  if (DeviceCanUseDisplaysize(CJNIBuild::DEVICE))
    displaySize = CJNISystemProperties::get("sys.display-size", "");

  // Override with xmbc_properties if present
  std::string customdisplaySize = CJNISystem::getProperty("xbmc.display-size", "");
  if (!customdisplaySize.empty())
    displaySize = customdisplaySize;

  if (!displaySize.empty())
  {
    CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: display-size: %s", displaySize.c_str());
    std::vector<std::string> aSize = StringUtils::Split(displaySize, "x");
    if (aSize.size() == 2)
    {
      m_width = StringUtils::IsInteger(aSize[0]) ? atoi(aSize[0].c_str()) : 0;
      m_height = StringUtils::IsInteger(aSize[1]) ? atoi(aSize[1].c_str()) : 0;
    }
  }

  return;
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
  *nativeWindow = (XBNativeWindowType*) CXBMCApp::GetNativeWindow(30000);
  return (*nativeWindow != NULL);
}

bool CEGLNativeTypeAndroid::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeAndroid::DestroyNativeWindow()
{
  return true;
}

static float currentRefreshRate()
{
  CJNIWindow window = CXBMCApp::getWindow();
  if (window)
  {
    float preferredRate = window.getAttributes().getpreferredRefreshRate();
    if (preferredRate > 1.0)
    {
      return preferredRate;
    }
    CJNIView view(window.getDecorView());
    if (view) {
      CJNIDisplay display(view.getDisplay());
      if (display)
      {
        float reportedRate = display.getRefreshRate();
        return reportedRate;
      }
    }
  }
  CLog::Log(LOGDEBUG, "found no refresh rate");
  return 60.0;
}

bool CEGLNativeTypeAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
  EGLNativeWindowType *nativeWindow = (EGLNativeWindowType*)CXBMCApp::GetNativeWindow(30000);
  if (!nativeWindow)
    return false;

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

  res->fRefreshRate = currentRefreshRate();
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
  res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGNOTICE,"Current resolution: %s\n",res->strMode.c_str());
  return true;
}

bool CEGLNativeTypeAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  CLog::Log(LOGDEBUG, "CEGLNativeTypeAndroid: SetNativeResolution: %dx%d", m_width, m_height);
  if (m_width && m_height)
    CXBMCApp::SetBuffersGeometry(m_width, m_height, 0);

  if (abs(currentRefreshRate() - res.fRefreshRate) > 0.0001)
    CXBMCApp::SetRefreshRate(res.fRefreshRate);

  return true;
}

bool CEGLNativeTypeAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
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
    }

    if (refreshRates.size())
    {
      for (unsigned int i = 0; i < refreshRates.size(); i++)
      {
        res.fRefreshRate = refreshRates[i];
        resolutions.push_back(res);
      }
    }
    else
    {
      /* No refresh rate list available, just provide the current one */
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
