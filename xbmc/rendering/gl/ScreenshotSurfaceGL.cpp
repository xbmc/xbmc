/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceGL.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Screenshot.h"
#include "windowing/GraphicContext.h"

#include <memory>
#include <mutex>
#include <vector>

#include "system_gl.h"

void CScreenshotSurfaceGL::Register()
{
  CScreenShot::Register(CScreenshotSurfaceGL::CreateSurface);
}

std::unique_ptr<IScreenshotSurface> CScreenshotSurfaceGL::CreateSurface()
{
  return std::make_unique<CScreenshotSurfaceGL>();
}

bool CScreenshotSurfaceGL::Capture()
{
  CWinSystemBase* winsystem = CServiceBroker::GetWinSystem();
  if (!winsystem)
    return false;

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (!gui)
    return false;

  std::unique_lock<CCriticalSection> lock(winsystem->GetGfxContext());
  gui->GetWindowManager().Render();

  glReadBuffer(GL_BACK);

  // get current viewport
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  m_width = viewport[2] - viewport[0];
  m_height = viewport[3] - viewport[1];
  m_stride = m_width * 4;
  std::vector<uint8_t> surface(m_stride * m_height);

  // read pixels from the backbuffer
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, static_cast<GLvoid*>(surface.data()));

  // make a new buffer and copy the read image to it with the Y axis inverted
  m_buffer = new unsigned char[m_stride * m_height];
  for (int y = 0; y < m_height; y++)
    memcpy(m_buffer + y * m_stride, surface.data() + (m_height - y - 1) * m_stride, m_stride);

  return m_buffer != nullptr;
}
