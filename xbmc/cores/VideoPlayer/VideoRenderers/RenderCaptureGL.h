/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderCapture.h"

#include "system_gl.h"

class CRenderCaptureGL : public CRenderCapture
{
public:
  CRenderCaptureGL() = default;
  ~CRenderCaptureGL() override;

  void BeginRender() override;
  void EndRender() override;
  void ReadOut() override;

  void* GetRenderBuffer() override;

private:
  void PboToBuffer();
  GLuint m_pbo{0};
  GLuint m_query{0};
  bool m_occlusionQuerySupported{false};
};
