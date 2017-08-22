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
#include "system.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "WinSystemX11GLContext.h"
#include "GLContextEGL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "guilib/GraphicContext.h"
#include "guilib/DispResource.h"
#include "threads/SingleLock.h"
#include <vector>
#include "Application.h"
#include "VideoSyncDRM.h"

#ifdef HAS_GLX
#include "VideoSyncGLX.h"
#include "GLContextGLX.h"
#endif // HAS_GLX

#include "cores/RetroPlayer/process/X11/RPProcessInfoX11.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Process/X11/ProcessInfoX11.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"

using namespace KODI;

CWinSystemX11GLContext::CWinSystemX11GLContext() = default;

CWinSystemX11GLContext::~CWinSystemX11GLContext()
{
  delete m_pGLContext;
}

void CWinSystemX11GLContext::PresentRenderImpl(bool rendered)
{
  if (rendered)
    m_pGLContext->SwapBuffers();

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemX11GLContext::SetVSyncImpl(bool enable)
{
  m_pGLContext->SetVSync(enable);
}

bool CWinSystemX11GLContext::IsExtSupported(const char* extension)
{
  if(strncmp(extension, m_pGLContext->ExtPrefix().c_str(), 4) != 0)
    return CRenderSystemGL::IsExtSupported(extension);

  return m_pGLContext->IsExtSupported(extension);
}

#ifdef HAS_GLX
GLXWindow CWinSystemX11GLContext::GetWindow() const
{
  return static_cast<CGLContextGLX*>(m_pGLContext)->m_glxWindow;
}

GLXContext CWinSystemX11GLContext::GetGlxContext() const
{
  return static_cast<CGLContextGLX*>(m_pGLContext)->m_glxContext;
}
#endif // HAS_GLX

EGLDisplay CWinSystemX11GLContext::GetEGLDisplay() const
{
  return static_cast<CGLContextEGL*>(m_pGLContext)->m_eglDisplay;
}

EGLSurface CWinSystemX11GLContext::GetEGLSurface() const
{
  return static_cast<CGLContextEGL*>(m_pGLContext)->m_eglSurface;
}

EGLContext CWinSystemX11GLContext::GetEGLContext() const
{
  return static_cast<CGLContextEGL*>(m_pGLContext)->m_eglContext;
}

EGLConfig CWinSystemX11GLContext::GetEGLConfig() const
{
  return static_cast<CGLContextEGL*>(m_pGLContext)->m_eglConfig;
}

bool CWinSystemX11GLContext::SetWindow(int width, int height, bool fullscreen, const std::string &output, int *winstate)
{
  int newwin = 0;
  CWinSystemX11::SetWindow(width, height, fullscreen, output, &newwin);
  if (newwin)
  {
    RefreshGLContext(m_currentOutput.compare(output) != 0);
    XSync(m_dpy, FALSE);
    g_graphicsContext.Clear(0);
    g_graphicsContext.Flip(true, false);
    ResetVSync();

    m_windowDirty = false;
    m_bIsInternalXrr = false;

    if (!m_delayDispReset)
    {
      CSingleLock lock(m_resourceSection);
      // tell any shared resources
      for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
        (*i)->OnResetDisplay();
    }
  }
  return true;
}

bool CWinSystemX11GLContext::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  if(!CWinSystemX11::CreateNewWindow(name, fullScreen, res))
    return false;

  m_pGLContext->QueryExtensions();
  return true;
}

bool CWinSystemX11GLContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_newGlContext = false;
  CWinSystemX11::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);

  if (m_newGlContext)
    g_application.ReloadSkin();

  return true;
}

bool CWinSystemX11GLContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  m_newGlContext = false;
  CWinSystemX11::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight);

  if (m_newGlContext)
    g_application.ReloadSkin();

  return true;
}

bool CWinSystemX11GLContext::DestroyWindowSystem()
{
  m_pGLContext->Destroy();
  return CWinSystemX11::DestroyWindowSystem();
}

bool CWinSystemX11GLContext::DestroyWindow()
{
  m_pGLContext->Detach();
  return CWinSystemX11::DestroyWindow();
}

XVisualInfo* CWinSystemX11GLContext::GetVisual()
{
  int count = 0;
  XVisualInfo vTemplate;
  XVisualInfo *visual = nullptr;

  int vMask = VisualScreenMask | VisualDepthMask | VisualClassMask;

  vTemplate.screen = m_nScreen;
  vTemplate.depth = 24;
  vTemplate.c_class = TrueColor;

  visual = XGetVisualInfo(m_dpy, vMask, &vTemplate, &count);

  return visual;
}

#if defined (HAVE_LIBVA)
#include <va/va_x11.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGL.h"
#endif
#if defined (HAVE_LIBVDPAU)
#include "cores/VideoPlayer/DVDCodecs/Video/VDPAU.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVDPAU.h"
#endif

bool CWinSystemX11GLContext::RefreshGLContext(bool force)
{
  bool success = false;
  if (m_pGLContext)
  {
    success = m_pGLContext->Refresh(force, m_nScreen, m_glWindow, m_newGlContext);
    return success;
  }

  VIDEOPLAYER::CProcessInfoX11::Register();
  RETRO::CRPProcessInfoX11::Register();
  RETRO::CRPProcessInfoX11::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGL);
  CDVDFactoryCodec::ClearHWAccels();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CLinuxRendererGL::Register();

  m_pGLContext = new CGLContextEGL(m_dpy);
  success = m_pGLContext->Refresh(force, m_nScreen, m_glWindow, m_newGlContext);
  if (success)
  {
    std::string gpuvendor;
    const char* vend = (const char*) glGetString(GL_VENDOR);
    if (vend)
      gpuvendor = vend;
    std::transform(gpuvendor.begin(), gpuvendor.end(), gpuvendor.begin(), ::tolower);
    if (gpuvendor.compare(0, 5, "intel") == 0)
    {
#if defined (HAVE_LIBVA)
      EGLDisplay eglDpy = static_cast<CGLContextEGL*>(m_pGLContext)->m_eglDisplay;
      VADisplay vaDpy = GetVaDisplay();
      bool general, hevc;
      CRendererVAAPI::Register(vaDpy, eglDpy, general, hevc);
      if (general)
        VAAPI::CDecoder::Register(hevc);
#endif
      return success;
    }
  }

#ifdef HAS_GLX
  delete m_pGLContext;

  // fallback for vdpau
  m_pGLContext = new CGLContextGLX(m_dpy);
  success = m_pGLContext->Refresh(force, m_nScreen, m_glWindow, m_newGlContext);
  if (success)
  {
#if defined (HAVE_LIBVDPAU)
    VDPAU::CDecoder::Register();
    CRendererVDPAU::Register();
#endif
  }
#endif // HAS_GLX
  return success;
}

std::unique_ptr<CVideoSync> CWinSystemX11GLContext::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync;

  if (dynamic_cast<CGLContextEGL*>(m_pGLContext))
  {
    pVSync.reset(new CVideoSyncDRM(clock));
  }
#ifdef HAS_GLX
  else if (dynamic_cast<CGLContextGLX*>(m_pGLContext))
  {
    pVSync.reset(new CVideoSyncGLX(clock));
  }
#endif // HAS_GLX
  return pVSync;
}

void* CWinSystemX11GLContext::GetVaDisplay()
{
#if defined(HAVE_LIBVA)
  return vaGetDisplay(m_dpy);
#endif
  return nullptr;
}
