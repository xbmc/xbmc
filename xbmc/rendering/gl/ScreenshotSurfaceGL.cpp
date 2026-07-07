/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceGL.h"

#include "guilib/GUIWindowManager.h"
#include "utils/Screenshot.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <cstring>
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

bool CScreenshotSurfaceGL::Capture(const ScreenshotContext& ctx)
{
  std::unique_lock lock(ctx.winSystem.GetGfxContext());
  ctx.windowManager.Render();

  glReadBuffer(GL_BACK);

  // get current viewport
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  m_width = viewport[2] - viewport[0];
  m_height = viewport[3] - viewport[1];

  const int bitDepth = ctx.winSystem.GetOutputBitDepth();
  if (bitDepth > 8)
  {
    // desktop GL converts any framebuffer depth to 16-bit per channel on readback
    m_bitDepth = bitDepth;
    m_stride = m_width * 8; // 4 channels x 2 bytes
    std::vector<uint16_t> raw(m_width * m_height * 4);
    glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_SHORT,
                 static_cast<GLvoid*>(raw.data()));

    if (glGetError() != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "CScreenshotSurfaceGL: glReadPixels 16-bit failed");
      return false;
    }

    m_buffer = new unsigned char[m_stride * m_height];
    for (int y = 0; y < m_height; y++)
    {
      uint16_t* dst = reinterpret_cast<uint16_t*>(m_buffer + y * m_stride);
      std::memcpy(dst, raw.data() + (m_height - 1 - y) * m_width * 4, m_stride);
      for (int x = 0; x < m_width; x++)
        dst[x * 4 + 3] = 0xFFFF; // A
    }

    return m_buffer != nullptr;
  }

  m_stride = m_width * 4;
  std::vector<uint8_t> surface(m_stride * m_height);

  // read pixels from the backbuffer
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, static_cast<GLvoid*>(surface.data()));

  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CScreenshotSurfaceGL: glReadPixels 8-bit failed");
    return false;
  }

  // make a new buffer and copy the read image to it with the Y axis inverted
  m_buffer = new unsigned char[m_stride * m_height];
  for (int y = 0; y < m_height; y++)
    memcpy(m_buffer + y * m_stride, surface.data() + (m_height - y - 1) * m_stride, m_stride);

  return m_buffer != nullptr;
}
