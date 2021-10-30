/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemOSXGL.h"

#include "guilib/Texture.h"
#include "rendering/gl/RenderSystemGL.h"
#include "windowing/WindowSystemFactory.h"

#include <dlfcn.h>
#include <unistd.h>

void CWinSystemOSXGL::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem);
}

std::unique_ptr<CWinSystemBase> CWinSystemOSXGL::CreateWinSystem()
{
  return std::make_unique<CWinSystemOSXGL>();
}

CWinSystemOSXGL::~CWinSystemOSXGL()
{
  if (m_glLibrary)
  {
    dlclose(m_glLibrary);
    m_glLibrary = nullptr;
  }
}

void CWinSystemOSXGL::PresentRenderImpl(bool rendered)
{
  if (rendered)
    FlushBuffer();

  // FlushBuffer does not block if window is obscured
  // in this case we need to throttle the render loop
  if (IsObscured())
    usleep(10000);

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    AnnounceOnResetDevice();
  }
}

void CWinSystemOSXGL::SetVSyncImpl(bool enable)
{
  EnableVSync(false);

  if (enable)
  {
    EnableVSync(true);
  }
}

bool CWinSystemOSXGL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemOSX::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);

  if (m_bVSync)
  {
    EnableVSync(m_bVSync);
  }

  return true;
}

bool CWinSystemOSXGL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemOSX::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight);

  if (m_bVSync)
  {
    EnableVSync(m_bVSync);
  }

  return true;
}

void* CWinSystemOSXGL::GetProcAddressGL(const char* name)
{
  if (!m_glLibrary)
  {
    const char* glLibPath = "/System/Library/Frameworks/OpenGL.framework/OpenGL";

    m_glLibrary = dlopen(glLibPath, RTLD_LAZY);

    if (!m_glLibrary)
      throw std::runtime_error("failed to load OpenGL library: " + std::string(dlerror()));
  }

  return dlsym(m_glLibrary, name);
}
