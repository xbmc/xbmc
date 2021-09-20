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

class CRenderCaptureGLES : public CRenderCapture
{
public:
  CRenderCaptureGLES() = default;
  ~CRenderCaptureGLES() override;

  void BeginRender() override;
  void EndRender() override;
};
