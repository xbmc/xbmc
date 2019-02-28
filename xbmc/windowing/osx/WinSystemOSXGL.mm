/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/Texture.h"
#include "windowing/WinSystemHeadless.h"
#include "WinSystemOSXGL.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem(bool render)
{
  std::unique_ptr<CWinSystemBase> winSystem(nullptr);
  if (render)
    winSystem.reset(new CWinSystemOSXGL());
  else
  {
    winSystem.reset(new CWinSystemHeadless());
    CLog::Log(LOGWARNING, "HEADLESS MOD NOT TESTED ON THIS BUILD");
  }
  return winSystem;
}

CWinSystemOSXGL::CWinSystemOSXGL()
{
}

CWinSystemOSXGL::~CWinSystemOSXGL()
{
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

