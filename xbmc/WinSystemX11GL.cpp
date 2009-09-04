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
#include "stdafx.h"

#ifdef HAS_GLX

#include "WinSystemX11GL.h"
#include "utils/log.h"

CWinSystemX11GL g_Windowing;

CWinSystemX11GL::CWinSystemX11GL()
{
  m_glXGetVideoSyncSGI   = NULL;
  m_glXWaitVideoSyncSGI  = NULL;
  m_glXSwapIntervalSGI   = NULL;
  m_glXSwapIntervalMESA  = NULL;
  m_glXGetSyncValuesOML  = NULL;
  m_glXSwapBuffersMscOML = NULL;
}

CWinSystemX11GL::~CWinSystemX11GL()
{
}

bool CWinSystemX11GL::PresentRenderImpl()
{
  glXSwapBuffers(m_dpy, m_glWindow);

  return true;
}

void CWinSystemX11GL::SetVSyncImpl(bool enable)
{

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

bool CWinSystemX11GL::CreateNewWindow(const CStdString& name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  if(!CWinSystemX11::CreateNewWindow(name, width, height, fullScreen, userFunction))
    return false;

  m_glxext  = " ";
  m_glxext += (const char*)glXQueryExtensionsString(m_dpy, DefaultScreen(m_dpy));
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
  CWinSystemX11::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);  

  return true;
}

bool CWinSystemX11GL::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{
  CWinSystemX11::SetFullScreen(fullScreen, screen, width, height, blankOtherDisplays, alwaysOnTop);
  CRenderSystemGL::ResetRenderSystem(width, height);  
  
  return true;
}

#endif
