/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "system.h"

#ifdef HAS_GLX

#include "WinSystemX11GL.h"
#include "utils/log.h"
#include "Application.h"

CWinSystemX11GL::CWinSystemX11GL()
{
  m_glXGetVideoSyncSGI   = NULL;
  m_glXWaitVideoSyncSGI  = NULL;
  m_glXSwapIntervalSGI   = NULL;
  m_glXSwapIntervalMESA  = NULL;
  m_glXGetSyncValuesOML  = NULL;
  m_glXSwapBuffersMscOML = NULL;

  m_iVSyncErrors = 0;
}

CWinSystemX11GL::~CWinSystemX11GL()
{
}

bool CWinSystemX11GL::PresentRenderImpl(const CDirtyRegionList& dirty)
{
  CheckDisplayEvents();

  if(m_iVSyncMode == 3)
  {
    glFinish();
    unsigned int before = 0, after = 0;
    if(m_glXGetVideoSyncSGI(&before) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    glXSwapBuffers(m_dpy, m_glWindow);
    glFinish();

    if(m_glXGetVideoSyncSGI(&after) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    if(after == before)
      m_iVSyncErrors = 1;
    else
      m_iVSyncErrors--;

    if(m_iVSyncErrors > 0)
    {
      CLog::Log(LOGINFO, "GL: retrace count didn't change after buffer swap, switching to vsync mode 4");
      m_iVSyncErrors = 0;
      m_iVSyncMode   = 4;
    }

    if(m_iVSyncErrors < -200)
    {
      CLog::Log(LOGINFO, "GL: retrace count change for %d consecutive buffer swap, switching to vsync mode 2", -m_iVSyncErrors);
      m_iVSyncErrors = 0;
      m_iVSyncMode   = 2;
    }
  }
  else if (m_iVSyncMode == 4)
  {
    glFinish();
    unsigned int before = 0, swap = 0, after = 0;
    if(m_glXGetVideoSyncSGI(&before) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    if(m_glXWaitVideoSyncSGI(2, (before+1)%2, &swap) != 0)
      CLog::Log(LOGERROR, "%s - glXWaitVideoSyncSGI - Returned error", __FUNCTION__);

    glXSwapBuffers(m_dpy, m_glWindow);
    glFinish();

    if(m_glXGetVideoSyncSGI(&after) != 0)
      CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

    if(after == before)
      CLog::Log(LOGERROR, "%s - glXWaitVideoSyncSGI - Woke up early", __FUNCTION__);

    if(after > before + 1)
      m_iVSyncErrors++;
    else
      m_iVSyncErrors = 0;

    if(m_iVSyncErrors > 30)
    {
      CLog::Log(LOGINFO, "GL: retrace count seems to be changing due to the swapbuffers call, switching to vsync mode 3");
      m_iVSyncMode   = 3;
      m_iVSyncErrors = 0;
    }
  }
  else if (m_iVSyncMode == 5)
  {
    int64_t ust, msc, sbc;
    if(m_glXGetSyncValuesOML(m_dpy, m_glWindow, &ust, &msc, &sbc))
      m_glXSwapBuffersMscOML(m_dpy, m_glWindow, msc, 0, 0);
    else
      CLog::Log(LOGERROR, "%s - glXSwapBuffersMscOML - Failed to get current retrace count", __FUNCTION__);
  }
  else
    glXSwapBuffers(m_dpy, m_glWindow);

  return true;
}

void CWinSystemX11GL::SetVSyncImpl(bool enable)
{
  /* turn of current setting first */
  if(m_glXSwapIntervalSGI)
    m_glXSwapIntervalSGI(0);
  if(m_glXSwapIntervalMESA)
    m_glXSwapIntervalMESA(0);

  m_iVSyncErrors = 0;

  CStdString strVendor(m_RenderVendor);
  strVendor.ToLower();

  if(!enable)
    return;

  bool vendor_nvidia = strVendor.find("nvidia") != std::string::npos;
  bool vendor_ati    = strVendor.compare(0, 3, "ati") == 0;

  if (m_glXSwapIntervalMESA && !m_iVSyncMode && vendor_ati)
  {
    if(m_glXSwapIntervalMESA(1) == 0)
      m_iVSyncMode = 2;
    else
      CLog::Log(LOGWARNING, "%s - glXSwapIntervalMESA failed", __FUNCTION__);
  }

  if(m_glXGetSyncValuesOML && m_glXSwapBuffersMscOML && m_glXSwapIntervalMESA && !m_iVSyncMode)
  {
    m_glXSwapIntervalMESA(1);

    int64_t ust, msc, sbc;
    if(m_glXGetSyncValuesOML(m_dpy, m_glWindow, &ust, &msc, &sbc))
      m_iVSyncMode = 5;
    else
      CLog::Log(LOGWARNING, "%s - glXGetSyncValuesOML failed", __FUNCTION__);
  }
  if (m_glXWaitVideoSyncSGI && m_glXGetVideoSyncSGI && !m_iVSyncMode && !vendor_nvidia)
  {
    unsigned int count;
    if(m_glXGetVideoSyncSGI(&count) == 0)
        m_iVSyncMode = 3;
    else
      CLog::Log(LOGWARNING, "%s - glXGetVideoSyncSGI failed, glcontext probably not direct", __FUNCTION__);
  }
  if (m_glXSwapIntervalSGI && !m_iVSyncMode)
  {
    if(m_glXSwapIntervalSGI(1) == 0)
      m_iVSyncMode = 2;
    else
      CLog::Log(LOGWARNING, "%s - glXSwapIntervalSGI failed", __FUNCTION__);
  }
  if (m_glXSwapIntervalMESA && !m_iVSyncMode && !vendor_ati)
  {
    if(m_glXSwapIntervalMESA(1) == 0)
      m_iVSyncMode = 2;
    else
      CLog::Log(LOGWARNING, "%s - glXSwapIntervalMESA failed", __FUNCTION__);
  }

}

bool CWinSystemX11GL::IsExtSupported(const char* extension)
{
  if(strncmp(extension, "GLX_", 4) != 0)
    return CRenderSystemGL::IsExtSupported(extension);

  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

  return m_glxext.find(name) != std::string::npos;
}

bool CWinSystemX11GL::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if(!CWinSystemX11::CreateNewWindow(name, fullScreen, res, userFunction))
    return false;

  m_glxext  = " ";
  m_glxext += (const char*)glXQueryExtensionsString(m_dpy, m_nScreen);
  m_glxext += " ";

  CLog::Log(LOGDEBUG, "GLX_EXTENSIONS:%s", m_glxext.c_str());

  /* any time window is recreated we need new pointers */
  if (IsExtSupported("GLX_OML_sync_control"))
    m_glXGetSyncValuesOML = (Bool (*)(Display*, GLXDrawable, int64_t*, int64_t*, int64_t*))glXGetProcAddress((const GLubyte*)"glXGetSyncValuesOML");
  else
    m_glXGetSyncValuesOML = NULL;

  if (IsExtSupported("GLX_OML_sync_control"))
    m_glXSwapBuffersMscOML = (int64_t (*)(Display*, GLXDrawable, int64_t, int64_t, int64_t))glXGetProcAddress((const GLubyte*)"glXSwapBuffersMscOML");
  else
    m_glXSwapBuffersMscOML = NULL;

  if (IsExtSupported("GLX_SGI_video_sync"))
    m_glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  else
    m_glXWaitVideoSyncSGI = NULL;

  if (IsExtSupported("GLX_SGI_video_sync"))
    m_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  else
    m_glXGetVideoSyncSGI = NULL;

  if (IsExtSupported("GLX_SGI_swap_control") )
    m_glXSwapIntervalSGI = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
  else
    m_glXSwapIntervalSGI = NULL;

  if (IsExtSupported("GLX_MESA_swap_control"))
    m_glXSwapIntervalMESA = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
  else
    m_glXSwapIntervalMESA = NULL;


  return true;
}

bool CWinSystemX11GL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_newGlContext = false;
  CWinSystemX11::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight, false, 0);

  if (m_newGlContext)
    g_application.ReloadSkin();

  return true;
}

bool CWinSystemX11GL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  m_newGlContext = false;
  CWinSystemX11::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);

  if (m_newGlContext)
    g_application.ReloadSkin();

  return true;
}

#endif
