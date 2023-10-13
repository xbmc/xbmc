/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemX11GLContext.h"

#include "GLContextEGL.h"
#include "OptionalsReg.h"
#include "VideoSyncOML.h"
#include "X11DPMSSupport.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationSkinHandling.h"
#include "cores/RetroPlayer/process/X11/RPProcessInfoX11.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Process/X11/ProcessInfoX11.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "guilib/DispResource.h"
#include "rendering/gl/ScreenshotSurfaceGL.h"
#include "windowing/GraphicContext.h"
#include "windowing/WindowSystemFactory.h"

#include <memory>
#include <mutex>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace KODI;
using namespace KODI::WINDOWING::X11;


void CWinSystemX11GLContext::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem, "x11");
}

std::unique_ptr<CWinSystemBase> CWinSystemX11GLContext::CreateWinSystem()
{
  return std::make_unique<CWinSystemX11GLContext>();
}

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
    std::unique_lock<CCriticalSection> lock(m_resourceSection);
    // tell any shared resources
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemX11GLContext::SetVSyncImpl(bool enable)
{
  m_pGLContext->SetVSync(enable);
}

bool CWinSystemX11GLContext::IsExtSupported(const char* extension) const
{
  if(strncmp(extension, m_pGLContext->ExtPrefix().c_str(), 4) != 0)
    return CRenderSystemGL::IsExtSupported(extension);

  return m_pGLContext->IsExtSupported(extension);
}

XID CWinSystemX11GLContext::GetWindow() const
{
  return GLXGetWindow(m_pGLContext);
}

void* CWinSystemX11GLContext::GetGlxContext() const
{
  return GLXGetContext(m_pGLContext);
}

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
    XSync(m_dpy, False);
    CServiceBroker::GetWinSystem()->GetGfxContext().Clear(0);
    CServiceBroker::GetWinSystem()->GetGfxContext().Flip(true, false);
    ResetVSync();

    m_windowDirty = false;
    m_bIsInternalXrr = false;

    if (!m_delayDispReset)
    {
      std::unique_lock<CCriticalSection> lock(m_resourceSection);
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
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appSkin = components.GetComponent<CApplicationSkinHandling>();
    appSkin->ReloadSkin();
  }

  return true;
}

void CWinSystemX11GLContext::FinishWindowResize(int newWidth, int newHeight)
{
  m_newGlContext = false;
  CWinSystemX11::FinishWindowResize(newWidth, newHeight);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);

  if (m_newGlContext)
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appSkin = components.GetComponent<CApplicationSkinHandling>();
    appSkin->ReloadSkin();
  }
}

bool CWinSystemX11GLContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  m_newGlContext = false;
  CWinSystemX11::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight);

  if (m_newGlContext)
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appSkin = components.GetComponent<CApplicationSkinHandling>();
    appSkin->ReloadSkin();
  }

  return true;
}

bool CWinSystemX11GLContext::DestroyWindowSystem()
{
  if (m_pGLContext)
    m_pGLContext->Destroy();
  return CWinSystemX11::DestroyWindowSystem();
}

bool CWinSystemX11GLContext::DestroyWindow()
{
  if (m_pGLContext)
    m_pGLContext->Detach();
  return CWinSystemX11::DestroyWindow();
}

XVisualInfo* CWinSystemX11GLContext::GetVisual()
{
  int count = 0;
  XVisualInfo vTemplate;
  XVisualInfo *visual = nullptr;

  int vMask = VisualScreenMask | VisualDepthMask | VisualClassMask;

  vTemplate.screen = m_screen;
  vTemplate.depth = 24;
  vTemplate.c_class = TrueColor;

  visual = XGetVisualInfo(m_dpy, vMask, &vTemplate, &count);

  if (!visual)
  {
    vTemplate.depth = 30;
    visual = XGetVisualInfo(m_dpy, vMask, &vTemplate, &count);
  }

  return visual;
}

bool CWinSystemX11GLContext::RefreshGLContext(bool force)
{
  bool success = false;
  if (m_pGLContext)
  {
    if (force)
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appSkin = components.GetComponent<CApplicationSkinHandling>();
      appSkin->UnloadSkin();
      CRenderSystemGL::DestroyRenderSystem();
    }
    success = m_pGLContext->Refresh(force, m_screen, m_glWindow, m_newGlContext);
    if (!success)
    {
      success = m_pGLContext->CreatePB();
      m_newGlContext = true;
    }
    if (force)
      CRenderSystemGL::InitRenderSystem();
    return success;
  }

  m_dpms = std::make_shared<CX11DPMSSupport>();
  VIDEOPLAYER::CProcessInfoX11::Register();
  RETRO::CRPProcessInfoX11::Register();
  RETRO::CRPProcessInfoX11::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGL);
  CDVDFactoryCodec::ClearHWAccels();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CLinuxRendererGL::Register();

  CScreenshotSurfaceGL::Register();

  std::string gpuvendor;
  const char* vend = (const char*) glGetString(GL_VENDOR);
  if (vend)
    gpuvendor = vend;
  std::transform(gpuvendor.begin(), gpuvendor.end(), gpuvendor.begin(), ::tolower);
  bool isNvidia = (gpuvendor.compare(0, 6, "nvidia") == 0);
  bool isIntel = (gpuvendor.compare(0, 5, "intel") == 0);
  std::string gli = (getenv("KODI_GL_INTERFACE") != nullptr) ? getenv("KODI_GL_INTERFACE") : "";

  if (gli != "GLX")
  {
    m_pGLContext = new CGLContextEGL(m_dpy, EGL_OPENGL_API);
    success = m_pGLContext->Refresh(force, m_screen, m_glWindow, m_newGlContext);
    if (success)
    {
      if (!isNvidia)
      {
        m_vaapiProxy.reset(VaapiProxyCreate());
        VaapiProxyConfig(m_vaapiProxy.get(), GetDisplay(),
                              static_cast<CGLContextEGL*>(m_pGLContext)->m_eglDisplay);
        bool general = false;
        bool deepColor = false;
        VAAPIRegisterRenderGL(m_vaapiProxy.get(), general, deepColor);
        if (general)
        {
          VAAPIRegister(m_vaapiProxy.get(), deepColor);
          return true;
        }
        if (isIntel || gli == "EGL")
          return true;
      }
    }
    else if (gli == "EGL_PB")
    {
      success = m_pGLContext->CreatePB();
      if (success)
        return true;
    }
  }

  delete m_pGLContext;

  // fallback for vdpau
  m_pGLContext = GLXContextCreate(m_dpy);
  success = m_pGLContext->Refresh(force, m_screen, m_glWindow, m_newGlContext);
  if (success)
  {
    VDPAURegister();
    VDPAURegisterRender();
  }
  return success;
}

std::unique_ptr<CVideoSync> CWinSystemX11GLContext::GetVideoSync(CVideoReferenceClock* clock)
{
  std::unique_ptr<CVideoSync> pVSync;

  if (dynamic_cast<CGLContextEGL*>(m_pGLContext))
  {
    pVSync = std::make_unique<CVideoSyncOML>(clock, *this);
  }
  else
  {
    pVSync.reset(GLXVideoSyncCreate(clock, *this));
  }

  return pVSync;
}

float CWinSystemX11GLContext::GetFrameLatencyAdjustment()
{
  if (m_pGLContext)
  {
    uint64_t msc, interval;
    float micros = m_pGLContext->GetVblankTiming(msc, interval);
    return micros / 1000;
  }
  return 0;
}

uint64_t CWinSystemX11GLContext::GetVblankTiming(uint64_t &msc, uint64_t &interval)
{
  if (m_pGLContext)
  {
    float micros = m_pGLContext->GetVblankTiming(msc, interval);
    return micros;
  }
  msc = 0;
  interval = 0;
  return 0;
}

void CWinSystemX11GLContext::delete_CVaapiProxy::operator()(CVaapiProxy *p) const
{
  VaapiProxyDelete(p);
}
