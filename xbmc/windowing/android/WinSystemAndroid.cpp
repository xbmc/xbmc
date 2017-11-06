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

#include "WinSystemAndroid.h"

#include <string.h>
#include <float.h>

#include "ServiceBroker.h"
#include "guilib/GraphicContext.h"
#include "guilib/Resolution.h"
#include "settings/Settings.h"
#include "settings/DisplaySettings.h"
#include "guilib/DispResource.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "platform/android/activity/XBMCApp.h"

#include "cores/RetroPlayer/process/android/RPProcessInfoAndroid.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererGuiTexture.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodecAndroidMediaCodec.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererMediaCodec.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererMediaCodecSurface.h"

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

using namespace KODI;

CWinSystemAndroid::CWinSystemAndroid()
{
  m_eWindowSystem = WINDOW_SYSTEM_ANDROID;

  m_nativeDisplay = EGL_NO_DISPLAY;
  m_nativeWindow = nullptr;

  m_displayWidth = 0;
  m_displayHeight = 0;

  m_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_delayDispReset = false;

  m_android = nullptr;
}

CWinSystemAndroid::~CWinSystemAndroid()
{
  if(m_nativeWindow)
  {
    m_nativeWindow = nullptr;
  }
}

bool CWinSystemAndroid::InitWindowSystem()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;

  m_android = new CAndroidUtils();

  CDVDVideoCodecAndroidMediaCodec::Register();
  CDVDAudioCodecAndroidMediaCodec::Register();

  CLinuxRendererGLES::Register();
  RETRO::CRPProcessInfoAndroid::Register();
  RETRO::CRPProcessInfoAndroid::RegisterRendererFactory(new RETRO::CRendererFactoryGuiTexture);
  CRendererMediaCodec::Register();
  CRendererMediaCodecSurface::Register();

  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemAndroid::DestroyWindowSystem()
{
  delete m_android;
  m_android = nullptr;

  return true;
}

bool CWinSystemAndroid::CreateNewWindow(const std::string& name,
                                    bool fullScreen,
                                    RESOLUTION_INFO& res)
{
  RESOLUTION_INFO current_resolution;
  current_resolution.iWidth = current_resolution.iHeight = 0;
  RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();

  m_nWidth        = res.iWidth;
  m_nHeight       = res.iHeight;
  m_displayWidth  = res.iScreenWidth;
  m_displayHeight = res.iScreenHeight;
  m_fRefreshRate  = res.fRefreshRate;

  if ((m_bWindowCreated && m_android->GetNativeResolution(&current_resolution)) &&
    current_resolution.iWidth == res.iWidth && current_resolution.iHeight == res.iHeight &&
    current_resolution.iScreenWidth == res.iScreenWidth && current_resolution.iScreenHeight == res.iScreenHeight &&
    m_bFullScreen == fullScreen && current_resolution.fRefreshRate == res.fRefreshRate &&
    (current_resolution.dwFlags & D3DPRESENTFLAG_MODEMASK) == (res.dwFlags & D3DPRESENTFLAG_MODEMASK) &&
    m_stereo_mode == stereo_mode)
  {
    CLog::Log(LOGDEBUG, "CWinSystemEGL::CreateNewWindow: No need to create a new window");
    return true;
  }

  int delay = CServiceBroker::GetSettings().GetInt("videoscreen.delayrefreshchange");
  if (delay > 0)
  {
    m_delayDispReset = true;
    m_dispResetTimer.Set(delay * 100);
  }

  {
    CSingleLock lock(m_resourceSection);
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    {
      (*i)->OnLostDisplay();
    }
  }

  m_stereo_mode = stereo_mode;
  m_bFullScreen = fullScreen;

  m_nativeWindow = CXBMCApp::GetNativeWindow(2000);

  m_android->SetNativeResolution(res);

  if (!m_delayDispReset)
  {
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    {
      (*i)->OnResetDisplay();
    }
  }

  return true;
}

bool CWinSystemAndroid::DestroyWindow()
{
  return true;
}

void CWinSystemAndroid::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  RESOLUTION_INFO resDesktop, curDisplay;
  std::vector<RESOLUTION_INFO> resolutions;

  if (!m_android->ProbeResolutions(resolutions) || resolutions.empty())
  {
    CLog::Log(LOGWARNING, "%s: ProbeResolutions failed.",__FUNCTION__);
  }

  /* ProbeResolutions includes already all resolutions.
   * Only get desktop resolution so we can replace xbmc's desktop res
   */
  if (m_android->GetNativeResolution(&curDisplay))
  {
    resDesktop = curDisplay;
  }

  RESOLUTION ResDesktop = RES_INVALID;
  RESOLUTION res_index  = RES_DESKTOP;

  for (size_t i = 0; i < resolutions.size(); i++)
  {
    // if this is a new setting,
    // create a new empty setting to fill in.
    if ((int)CDisplaySettings::GetInstance().ResolutionInfoSize() <= res_index)
    {
      RESOLUTION_INFO res;
      CDisplaySettings::GetInstance().AddResolutionInfo(res);
    }

    g_graphicsContext.ResetOverscan(resolutions[i]);
    CDisplaySettings::GetInstance().GetResolutionInfo(res_index) = resolutions[i];

    CLog::Log(LOGNOTICE, "Found resolution %d x %d for display %d with %d x %d%s @ %f Hz\n",
      resolutions[i].iWidth,
      resolutions[i].iHeight,
      resolutions[i].iScreen,
      resolutions[i].iScreenWidth,
      resolutions[i].iScreenHeight,
      resolutions[i].dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "",
      resolutions[i].fRefreshRate);

    if(resDesktop.iWidth == resolutions[i].iWidth &&
       resDesktop.iHeight == resolutions[i].iHeight &&
       resDesktop.iScreenWidth == resolutions[i].iScreenWidth &&
       resDesktop.iScreenHeight == resolutions[i].iScreenHeight &&
       (resDesktop.dwFlags & D3DPRESENTFLAG_MODEMASK) == (resolutions[i].dwFlags & D3DPRESENTFLAG_MODEMASK) &&
       fabs(resDesktop.fRefreshRate - resolutions[i].fRefreshRate) < FLT_EPSILON)
    {
      ResDesktop = res_index;
    }

    res_index = (RESOLUTION)((int)res_index + 1);
  }

  // swap desktop index for desktop res if available
  if (ResDesktop != RES_INVALID)
  {
    CLog::Log(LOGNOTICE, "Found (%dx%d%s@%f) at %d, setting to RES_DESKTOP at %d",
      resDesktop.iWidth, resDesktop.iHeight,
      resDesktop.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "",
      resDesktop.fRefreshRate,
      (int)ResDesktop, (int)RES_DESKTOP);

    RESOLUTION_INFO desktop = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
    CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = CDisplaySettings::GetInstance().GetResolutionInfo(ResDesktop);
    CDisplaySettings::GetInstance().GetResolutionInfo(ResDesktop) = desktop;
  }
}

bool CWinSystemAndroid::Hide()
{
  return false;
}

bool CWinSystemAndroid::Show(bool raise)
{
  return false;
}

void CWinSystemAndroid::Register(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemAndroid::Unregister(IDispResource *resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}
