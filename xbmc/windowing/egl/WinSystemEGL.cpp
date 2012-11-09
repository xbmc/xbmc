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

#include "WinSystemEGL.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "EGLWrapper.h"
#include "EGLQuirks.h"
#include <vector>
////////////////////////////////////////////////////////////////////////////////////////////
CWinSystemEGL::CWinSystemEGL() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_EGL;

  m_displayWidth      = 0;
  m_displayHeight     = 0;

  m_display           = EGL_NO_DISPLAY;
  m_surface           = EGL_NO_SURFACE;
  m_context           = EGL_NO_CONTEXT;
  m_config            = NULL;

  m_egl               = NULL;
  m_iVSyncMode        = false;
}

CWinSystemEGL::~CWinSystemEGL()
{
  if (m_egl)
  {
    DestroyWindowSystem();
    delete m_egl;
  }
}

bool CWinSystemEGL::InitWindowSystem()
{
  RESOLUTION_INFO preferred_resolution;
  if (!m_egl)
    m_egl = new CEGLWrapper;

  if (!m_egl)
  {
    CLog::Log(LOGERROR, "%s: EGL not in a good state",__FUNCTION__);
    return false;
  }

  if (!m_egl->Initialize("auto"))
  {
    CLog::Log(LOGERROR, "%s: Could not initialize",__FUNCTION__);
    return false;
  }

  CLog::Log(LOGNOTICE, "%s: Using EGL Implementation: %s",__FUNCTION__,m_egl->GetNativeName().c_str());

  if (!m_egl->CreateNativeDisplay())
  {
    CLog::Log(LOGERROR, "%s: Could not get native display",__FUNCTION__);
    return false;
  }

  if (!m_egl->CreateNativeWindow())
  {
    CLog::Log(LOGERROR, "%s: Could not get native window",__FUNCTION__);
    return false;
  }

  if (!m_egl->InitDisplay(&m_display))
  {
    CLog::Log(LOGERROR, "%s: Could not create display",__FUNCTION__);
    return false;
  }

  EGLint configAttrs [] = {
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,     16,
        EGL_STENCIL_SIZE,    0,
        EGL_SAMPLE_BUFFERS,  0,
        EGL_SAMPLES,         0,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
  };

  if (!m_egl->ChooseConfig(m_display, configAttrs, &m_config))
  {
    CLog::Log(LOGERROR, "%s: Could not find a compatible configuration",__FUNCTION__);
    return false;
  }

  // Some platforms require a surface before we can probe the resolution.
  // Create the window here, then the creation in CreateNewWindow() will be skipped.
  int quirks;
  m_egl->GetQuirks(&quirks);
  if (quirks & EGL_QUIRK_NEED_WINDOW_FOR_RES)
  {
    RESOLUTION_INFO temp;
    CreateWindow(temp);
  }

  m_extensions = m_egl->GetExtensions(m_display);
  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemEGL::CreateWindow(RESOLUTION_INFO &res)
{
  if (!m_egl)
  {
    CLog::Log(LOGERROR, "CWinSystemEGL::CreateWindow no EGL!");
    return false;
  }

  if(m_egl)
    m_egl->SetNativeResolution(res);

  if (!m_egl->CreateSurface(m_display, m_config, &m_surface))
  {
    CLog::Log(LOGNOTICE, "%s: Could not create a surface. Trying with a fresh Native Window.",__FUNCTION__);
    m_egl->DestroyNativeWindow();
    if (!m_egl->CreateNativeWindow())
    {
      CLog::Log(LOGERROR, "%s: Could not get native window",__FUNCTION__);
      return false;
    }

    if (!m_egl->CreateSurface(m_display, m_config, &m_surface))
    {
      CLog::Log(LOGERROR, "%s: Could not create surface",__FUNCTION__);
      return false;
    }
  }

  int width = 0, height = 0;
  if (!m_egl->GetSurfaceSize(m_display, m_surface, &width, &height))
  {
    CLog::Log(LOGERROR, "%s: Surface is invalid",__FUNCTION__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s: Created surface of size %ix%i",__FUNCTION__, width, height);

  EGLint contextAttrs[] =
  {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  if (!m_egl->BindAPI(EGL_OPENGL_ES_API))
  {
    CLog::Log(LOGERROR, "%s: Could not bind %i api",__FUNCTION__, EGL_OPENGL_ES_API);
    return false;
  }

  if (m_context == EGL_NO_CONTEXT)
  {
    if (!m_egl->CreateContext(m_display, m_config, contextAttrs, &m_context))
    {
      CLog::Log(LOGERROR, "%s: Could not create context",__FUNCTION__);
      return false;
    }
  }

  if (!m_egl->BindContext(m_display, m_surface, m_context))
  {
    CLog::Log(LOGERROR, "%s: Could not bind to context",__FUNCTION__);
    return false;
  }

  m_bWindowCreated = true;

  return true;
}

bool CWinSystemEGL::DestroyWindowSystem()
{
  if (!m_egl)
    return true;

  DestroyWindow();

  if (m_context != EGL_NO_CONTEXT)
    m_egl->DestroyContext(m_display, m_context);
  m_context = EGL_NO_CONTEXT;

  if (m_display != EGL_NO_DISPLAY)
    m_egl->DestroyDisplay(m_display);
  m_display = EGL_NO_DISPLAY;

  m_egl->DestroyNativeWindow();

  m_egl->DestroyNativeDisplay();

  m_egl->Destroy();
  delete m_egl;
  m_egl = NULL;

  return true;
}

bool CWinSystemEGL::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  RESOLUTION_INFO current_resolution;
  current_resolution.iWidth = current_resolution.iHeight = 0;

  m_nWidth        = res.iWidth;    
  m_nHeight       = res.iHeight;
  m_displayWidth  = res.iScreenWidth;
  m_displayHeight = res.iScreenHeight;
  m_fRefreshRate  = res.fRefreshRate;

  if ((m_bWindowCreated && m_egl && m_egl->GetNativeResolution(&current_resolution)) &&
    current_resolution.iWidth == res.iWidth && current_resolution.iHeight == res.iHeight &&
    current_resolution.iScreenWidth == res.iScreenWidth && current_resolution.iScreenHeight == res.iScreenHeight &&
    m_bFullScreen == fullScreen && current_resolution.fRefreshRate == res.fRefreshRate)
  {
    CLog::Log(LOGDEBUG, "CWinSystemEGL::CreateNewWindow: No need to create a new window");
    return true;
  }

  m_bFullScreen   = fullScreen;
  // Destroy any existing window
  if (m_surface != EGL_NO_SURFACE)
    DestroyWindow();

  // If we previously destroyed an existing window we need to create a new one
  // (otherwise this is taken care of by InitWindowSystem())
  if (!CreateWindow(res))
  {
    CLog::Log(LOGERROR, "%s: Could not create new window",__FUNCTION__);
    return false;
  }
  Show();

  return true;
}

bool CWinSystemEGL::DestroyWindow()
{
  if (!m_egl)
    return false;

  m_egl->ReleaseContext(m_display);
  if (m_surface != EGL_NO_SURFACE)
    m_egl->DestroySurface(m_surface, m_display);

  int quirks;
  m_egl->GetQuirks(&quirks);
  if (quirks & EGL_QUIRK_DESTROY_NATIVE_WINDOW_WITH_SURFACE)
    m_egl->DestroyNativeWindow();

  m_surface = EGL_NO_SURFACE;
  m_bWindowCreated = false;
  return true;
}

bool CWinSystemEGL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, true, 0);
  SetVSyncImpl(m_iVSyncMode);
  return true;
}

bool CWinSystemEGL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CreateNewWindow("", fullScreen, res, NULL);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);
  SetVSyncImpl(m_iVSyncMode);
  return true;
}

void CWinSystemEGL::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  RESOLUTION_INFO resDesktop, curDisplay;
  std::vector<RESOLUTION_INFO> resolutions;

  if (!m_egl->ProbeResolutions(resolutions) || !resolutions.size())
  {
    CLog::Log(LOGERROR, "%s: Could not find any possible resolutions",__FUNCTION__);
    return;
  }

  /* ProbeResolutions includes already all resolutions.
   * Only get desktop resolution so we can replace xbmc's desktop res
   */
  if (m_egl->GetNativeResolution(&curDisplay))
    resDesktop = curDisplay;


  RESOLUTION ResDesktop = RES_INVALID;
  RESOLUTION res_index  = RES_DESKTOP;

  for (size_t i = 0; i < resolutions.size(); i++)
  {
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

    if(resDesktop.iWidth == resolutions[i].iWidth &&
       resDesktop.iHeight == resolutions[i].iHeight &&
       resDesktop.iScreenWidth == resolutions[i].iScreenWidth &&
       resDesktop.iScreenHeight == resolutions[i].iScreenHeight &&
       resDesktop.fRefreshRate == resolutions[i].fRefreshRate)
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

    RESOLUTION_INFO desktop = g_settings.m_ResInfo[RES_DESKTOP];
    g_settings.m_ResInfo[RES_DESKTOP] = g_settings.m_ResInfo[ResDesktop];
    g_settings.m_ResInfo[ResDesktop] = desktop;
  }
}

bool CWinSystemEGL::IsExtSupported(const char* extension)
{
  std::string name;

  name  = " ";
  name += extension;
  name += " ";

  return m_extensions.find(name) != std::string::npos;
}

bool CWinSystemEGL::PresentRenderImpl(const CDirtyRegionList &dirty)
{
  m_egl->SwapBuffers(m_display, m_surface);
  return true;
}

void CWinSystemEGL::SetVSyncImpl(bool enable)
{
  if (!m_egl->SetVSync(m_display, enable))
    CLog::Log(LOGERROR, "%s,Could not set egl vsync", __FUNCTION__);
}

void CWinSystemEGL::ShowOSMouse(bool show)
{
}

bool CWinSystemEGL::HasCursor()
{
#ifdef TARGET_ANDROID
  return false;
#else
  return true;
#endif
}

void CWinSystemEGL::NotifyAppActiveChange(bool bActivated)
{
}

bool CWinSystemEGL::Minimize()
{
  Hide();
  return true;
}

bool CWinSystemEGL::Restore()
{
  Show(true);
  return false;
}

bool CWinSystemEGL::Hide()
{
  return m_egl->ShowWindow(false);
}

bool CWinSystemEGL::Show(bool raise)
{
  return m_egl->ShowWindow(true);
}

EGLDisplay CWinSystemEGL::GetEGLDisplay()
{
  return m_display;
}

EGLContext CWinSystemEGL::GetEGLContext()
{
  return m_context;
}

bool CWinSystemEGL::Support3D(int width, int height, uint32_t mode) const
{
  bool bFound = false;
  int searchMode = 0;
  int searchWidth = width;
  int searchHeight = height;

  if (mode & D3DPRESENTFLAG_MODE3DSBS)
  {
    searchWidth /= 2;
    searchMode = D3DPRESENTFLAG_MODE3DSBS;
  }
  else if (mode & D3DPRESENTFLAG_MODE3DTB)
  {
    searchHeight /= 2;
    searchMode = D3DPRESENTFLAG_MODE3DTB;
  }

  for (unsigned int i = 0; i < g_settings.m_ResInfo.size(); i++)
  {
    RESOLUTION_INFO res = g_settings.m_ResInfo[i];

    if(res.iWidth == searchWidth && res.iHeight == searchHeight && (res.dwFlags & searchMode))
      return true;
  }

  return bFound;
}

bool CWinSystemEGL::ClampToGUIDisplayLimits(int &width, int &height)
{
  width = width > m_nWidth ? m_nWidth : width;
  height = height > m_nHeight ? m_nHeight : height;
  return true;
}

#endif
