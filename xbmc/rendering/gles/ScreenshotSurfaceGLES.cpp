/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceGLES.h"

#include "rendering/capture/CaptureReadback.h"
#include "utils/Screenshot.h"
#include "windowing/WinSystem.h"

#include <memory>

#include "system_gl.h"

void CScreenshotSurfaceGLES::Register()
{
  CScreenShot::Register(CScreenshotSurfaceGLES::CreateSurface);
}

std::unique_ptr<IScreenshotSurface> CScreenshotSurfaceGLES::CreateSurface()
{
  return std::make_unique<CScreenshotSurfaceGLES>();
}

bool CScreenshotSurfaceGLES::Read(const ScreenshotContext& ctx)
{
  using namespace KODI::RENDERING::CAPTURE;

  // get current viewport: x, y, width, height
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  // window rows are bottom-up: flip to top-down. The shared readback negotiates
  // the deep type and delivers 8-bit when the driver offers nothing deeper.
  ReadbackBuffer buffer;
  if (!ReadFramebufferRegion(viewport[0], viewport[1], static_cast<unsigned int>(viewport[2]),
                             static_cast<unsigned int>(viewport[3]), ctx.winSystem.GetOutputBitDepth(),
                             true, buffer))
    return false;

  m_width = static_cast<int>(buffer.width);
  m_height = static_cast<int>(buffer.height);
  m_stride = static_cast<int>(buffer.stride);
  m_bitDepth = buffer.bitDepth;
  m_buffer = reinterpret_cast<unsigned char*>(buffer.pixels.release());
  return m_buffer != nullptr;
}
