/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "system.h"

#ifdef HAS_EGL

#include "WinSystemGLES.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"
#include "utils/log.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////
CWinSystemGLES::CWinSystemGLES() : CWinSystemBase()
{
  m_window = NULL;
  m_eglplatform = new CWinEGLPlatform();
  m_eWindowSystem = WINDOW_SYSTEM_EGL;
}

CWinSystemGLES::~CWinSystemGLES()
{
  DestroyWindowSystem();
  delete m_eglplatform;
}

bool CWinSystemGLES::InitWindowSystem()
{
  m_display = EGL_DEFAULT_DISPLAY;
  m_window = m_eglplatform->InitWindowSystem(m_display, 1920, 1080, 8);
  
  // Initialize the display
  // This needs to happen before the call to CWinSystemBase::InitWindowSystem()
  // (at least for Android)
  if (!m_eglplatform->InitializeDisplay())
    return false;
  
  // Create a window to get valid width and height values
  // This needs to happen before the call to CWinSystemBase::InitWindowSystem()
  // (at least for Android)
#if defined(TARGET_ANDROID)
  if(!m_eglplatform->CreateWindow())
    return false;
#endif

  if (!CWinSystemBase::InitWindowSystem())
    return false;

  return true;
}

bool CWinSystemGLES::DestroyWindowSystem()
{
  m_eglplatform->DestroyWindowSystem(m_window);
  m_window = NULL;

  return true;
}

bool CWinSystemGLES::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if (m_bWindowCreated &&
    m_nWidth == res.iWidth && m_nHeight == res.iHeight &&
    m_nScreenWidth == res.iScreenWidth && m_nScreenHeight == res.iScreenHeight &&
    m_bFullScreen  == fullScreen &&
    m_fRefreshRate == res.fRefreshRate)
  {
    CLog::Log(LOGDEBUG, "CWinSystemGLES::CreateNewWindow: No need to create a new window");
    return true;
  }

  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_nScreenWidth  = res.iScreenWidth;
  m_nScreenHeight = res.iScreenHeight;
  m_bFullScreen   = fullScreen;
  m_fRefreshRate  = res.fRefreshRate;
  
  // Destroy any existing window
  if (m_bWindowCreated)
    m_eglplatform->DestroyWindow();

  m_eglplatform->SetDisplayResolution(res);
    
  // If we previously destroyed an existing window we need to create a new one
  // (otherwise this is taken care of by InitWindowSystem())
  if (m_bWindowCreated)
    m_eglplatform->CreateWindow();

  if (!m_eglplatform->BindSurface())
  {
    m_eglplatform->DestroyWindow();
    return false;
  }

  m_bWindowCreated = true;

  return true;
}

bool CWinSystemGLES::DestroyWindow()
{
  if (!m_eglplatform->DestroyWindow())
    return false;

  m_bWindowCreated = false;

  return true;
}

bool CWinSystemGLES::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, true, 0);
  return true;
}

bool CWinSystemGLES::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CreateNewWindow("", fullScreen, res, NULL);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);
  return true;
}

void CWinSystemGLES::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  std::vector<RESOLUTION_INFO> resolutions;

  m_eglplatform->ProbeDisplayResolutions(resolutions);

  RESOLUTION_INFO resDesktop = m_eglplatform->GetDesktopRes();

  RESOLUTION ResDesktop = RES_INVALID;
  RESOLUTION res_index  = RES_DESKTOP;

  for (size_t i = 0; i < resolutions.size(); i++)
  {
    int gui_width  = resolutions[i].iWidth;
    int gui_height = resolutions[i].iHeight;

    m_eglplatform->ClampToGUIDisplayLimits(gui_width, gui_height);

    resolutions[i].iWidth = gui_width;
    resolutions[i].iHeight = gui_height;

    // if this is a new setting,
    // create a new empty setting to fill in.
    if ((int)g_settings.m_ResInfo.size() <= res_index)
    {
      RESOLUTION_INFO res;

      g_settings.m_ResInfo.push_back(res);
    }

    g_graphicsContext.ResetOverscan(resolutions[i]);
    g_settings.m_ResInfo[res_index] = resolutions[i];

    CLog::Log(LOGNOTICE, "Found resolution %d x %d for display %d with %d x %d%s @ %f Hz\n",
      resolutions[i].iWidth,
      resolutions[i].iHeight,
      resolutions[i].iScreen,
      resolutions[i].iScreenWidth,
      resolutions[i].iScreenHeight,
      resolutions[i].dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "",
      resolutions[i].fRefreshRate);

    if(m_eglplatform->FixedDesktop())
    {
      if(resDesktop.iWidth == resolutions[i].iWidth &&
         resDesktop.iHeight == resolutions[i].iHeight &&
         resDesktop.iScreenWidth == resolutions[i].iScreenWidth &&
         resDesktop.iScreenHeight == resolutions[i].iScreenHeight &&
         resDesktop.fRefreshRate == resolutions[i].fRefreshRate &&
         resDesktop.dwFlags == resolutions[i].dwFlags)
      {
        ResDesktop = res_index;
      }
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

    RESOLUTION_INFO desktop = g_settings.m_ResInfo[RES_DESKTOP];
    g_settings.m_ResInfo[RES_DESKTOP] = g_settings.m_ResInfo[ResDesktop];
    g_settings.m_ResInfo[ResDesktop] = desktop;
  }
}

bool CWinSystemGLES::IsExtSupported(const char* extension)
{
  if(strncmp(extension, "EGL_", 4) != 0)
    return CRenderSystemGLES::IsExtSupported(extension);

  return m_eglplatform->IsExtSupported(extension);
}

bool CWinSystemGLES::PresentRenderImpl(const CDirtyRegionList &dirty)
{
  m_eglplatform->SwapBuffers();

  return true;
}

void CWinSystemGLES::SetVSyncImpl(bool enable)
{
  m_iVSyncMode = enable ? 10 : 0;
  if (m_eglplatform->SetVSync(enable) == FALSE)
    CLog::Log(LOGERROR, "CWinSystemDFB::SetVSyncImpl: Could not set egl vsync");
}

void CWinSystemGLES::ShowOSMouse(bool show)
{
}

bool CWinSystemGLES::HasCursor()
{
#ifdef TARGET_ANDROID
  return false;
#else
  return true;
#endif
}

void CWinSystemGLES::NotifyAppActiveChange(bool bActivated)
{
}

bool CWinSystemGLES::Minimize()
{
  Hide();
  return true;
}

bool CWinSystemGLES::Restore()
{
  Show(true);
  return false;
}

bool CWinSystemGLES::Hide()
{
  return m_eglplatform->ShowWindow(false);
}

bool CWinSystemGLES::Show(bool raise)
{
  return m_eglplatform->ShowWindow(true);
}

EGLDisplay CWinSystemGLES::GetEGLDisplay()
{
  return m_eglplatform->GetEGLDisplay();
}

EGLContext CWinSystemGLES::GetEGLContext()
{
  return m_eglplatform->GetEGLContext();
}

bool CWinSystemGLES::Support3D(int width, int height, uint32_t mode) const
{
  bool bFound = false;
  int searchMode = 0;
  int searchWidth = width;
  int searchHeight = height;

  if(mode & D3DPRESENTFLAG_MODE3DSBS)
  {
    searchWidth /= 2;
    searchMode = D3DPRESENTFLAG_MODE3DSBS;
  }
  else if(mode & D3DPRESENTFLAG_MODE3DTB)
  {
    searchHeight /= 2;
    searchMode = D3DPRESENTFLAG_MODE3DTB;
  }

  for(int i = 0; i < g_settings.m_ResInfo.size(); i++)
  {
    RESOLUTION_INFO res = g_settings.m_ResInfo[i];

    if(res.iWidth == searchWidth && res.iHeight == searchHeight && (res.dwFlags & searchMode))
    {
      return true;
    }
  }

  return bFound;
}

bool CWinSystemGLES::ClampToGUIDisplayLimits(int &width, int &height)
{
  return m_eglplatform->ClampToGUIDisplayLimits(width, height);
}


#endif
