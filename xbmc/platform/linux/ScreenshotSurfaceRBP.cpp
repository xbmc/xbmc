/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenshotSurfaceRBP.h"

#include "utils/Screenshot.h"

#include "platform/linux/RBP.h"

void CScreenshotSurfaceRBP::Register()
{
  CScreenShot::Register(CScreenshotSurfaceRBP::CreateSurface);
}

std::unique_ptr<IScreenshotSurface> CScreenshotSurfaceRBP::CreateSurface()
{
  return std::unique_ptr<CScreenshotSurfaceRBP>(new CScreenshotSurfaceRBP());
}

bool CScreenshotSurfaceRBP::Capture()
{
  g_RBP.GetDisplaySize(m_width, m_height);
  m_buffer = g_RBP.CaptureDisplay(m_width, m_height, &m_stride, true, false);
  if (!m_buffer)
    return false;

  return true;
}
