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

void CWinSystemOSXGL::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem);
}

std::unique_ptr<CWinSystemBase> CWinSystemOSXGL::CreateWinSystem()
{
  return std::make_unique<CWinSystemOSXGL>();
}

void CWinSystemOSXGL::PresentRenderImpl(bool rendered)
{
  if (rendered)
    FlushBuffer();

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
  CRenderSystemGL::ResetRenderSystem(res.iWidth - res.guiInsets.right - res.guiInsets.left,
                                     res.iHeight - res.guiInsets.top - res.guiInsets.bottom);

  if (m_bVSync)
  {
    EnableVSync(m_bVSync);
  }

  return true;
}

