/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceGLES.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Screenshot.h"
#include "windowing/GraphicContext.h"

#include <memory>
#include <mutex>
#include <vector>

#include "system_gl.h"

void CScreenshotSurfaceGLES::Register()
{
  CScreenShot::Register(CScreenshotSurfaceGLES::CreateSurface);
}

std::unique_ptr<IScreenshotSurface> CScreenshotSurfaceGLES::CreateSurface()
{
  return std::make_unique<CScreenshotSurfaceGLES>();
}

bool CScreenshotSurfaceGLES::Capture()
{
  CWinSystemBase* winsystem = CServiceBroker::GetWinSystem();
  if (!winsystem)
    return false;

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (!gui)
    return false;

  std::unique_lock<CCriticalSection> lock(winsystem->GetGfxContext());
  gui->GetWindowManager().Render();

  //get current viewport
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  m_width = viewport[2] - viewport[0];
  m_height = viewport[3] - viewport[1];
  m_stride = m_width * 4;
  std::vector<uint8_t> surface(m_stride * m_height);

  //read pixels from the backbuffer
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLvoid*>(surface.data()));

  //make a new buffer and copy the read image to it with the Y axis inverted
  m_buffer = new unsigned char[m_stride * m_height];
  for (int y = 0; y < m_height; y++)
  {
    // we need to save in BGRA order so XOR Swap RGBA -> BGRA
    unsigned char* swap_pixels = surface.data() + (m_height - y - 1) * m_stride;
    for (int x = 0; x < m_width; x++, swap_pixels += 4)
    {
      std::swap(swap_pixels[0], swap_pixels[2]);
    }

    memcpy(m_buffer + y * m_stride, surface.data() + (m_height - y - 1) * m_stride, m_stride);
  }

  return m_buffer != nullptr;
}
