/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncAndroid.h"
#include "WinSystemAndroidGLESContext.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "platform/android/activity/XBMCApp.h"

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemAndroidGLESContext());
  return winSystem;
}

bool CWinSystemAndroidGLESContext::InitWindowSystem()
{
  if (!CWinSystemAndroid::InitWindowSystem())
  {
    return false;
  }

  if (!m_pGLContext.CreateDisplay(m_nativeDisplay))
  {
    return false;
  }

  if (!m_pGLContext.InitializeDisplay(EGL_OPENGL_ES_API))
  {
    return false;
  }

  if (!m_pGLContext.ChooseConfig(EGL_OPENGL_ES2_BIT))
  {
    return false;
  }

  CEGLAttributesVec contextAttribs;
  contextAttribs.Add({{EGL_CONTEXT_CLIENT_VERSION, 2}});

  if (!m_pGLContext.CreateContext(contextAttribs))
  {
    return false;
  }

  return true;
}

bool CWinSystemAndroidGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.DestroySurface();

  if (!CWinSystemAndroid::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!m_pGLContext.CreateSurface(m_nativeWindow))
  {
    return false;
  }

  if (!m_pGLContext.BindContext())
  {
    return false;
  }

  return true;
}

bool CWinSystemAndroidGLESContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
  return true;
}

bool CWinSystemAndroidGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CreateNewWindow("", fullScreen, res);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);
  return true;
}

void CWinSystemAndroidGLESContext::SetVSyncImpl(bool enable)
{
  // We use Choreographer for timing
  m_pGLContext.SetVSync(false);
}

void CWinSystemAndroidGLESContext::PresentRenderImpl(bool rendered)
{
  if (!m_nativeWindow)
  {
    usleep(10000);
    return;
  }

  // Mode change finalization was triggered by timer
  if (IsHdmiModeTriggered())
    SetHdmiState(true);

  // Ignore EGL_BAD_SURFACE: It seems to happen during/after mode changes, but
  // we can't actually do anything about it
  if (rendered && !m_pGLContext.TrySwapBuffers())
    CEGLUtils::LogError("eglSwapBuffers failed");
  CXBMCApp::get()->WaitVSync(1000);
}

float CWinSystemAndroidGLESContext::GetFrameLatencyAdjustment()
{
  return CXBMCApp::GetFrameLatencyMs();
}

EGLDisplay CWinSystemAndroidGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.GetEGLDisplay();
}

EGLSurface CWinSystemAndroidGLESContext::GetEGLSurface() const
{
  return m_pGLContext.GetEGLSurface();
}

EGLContext CWinSystemAndroidGLESContext::GetEGLContext() const
{
  return m_pGLContext.GetEGLContext();
}

EGLConfig  CWinSystemAndroidGLESContext::GetEGLConfig() const
{
  return m_pGLContext.GetEGLConfig();
}

std::unique_ptr<CVideoSync> CWinSystemAndroidGLESContext::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncAndroid(clock));
  return pVSync;
}

