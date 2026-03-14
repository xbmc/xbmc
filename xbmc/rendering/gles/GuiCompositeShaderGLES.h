/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/Shader.h"

class CGuiCompositeShaderGLES : public Shaders::CGLSLShaderProgram
{
public:
  CGuiCompositeShaderGLES();

  void SetProjection(const GLfloat* proj) { m_proj = proj; }
  void SetSdrPeak(float peak) { m_sdrPeak = peak; }

  GLint GetPosLoc() { return m_hPos; }
  GLint GetTexLoc() { return m_hTex; }

protected:
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

private:
  const GLfloat* m_proj{nullptr};
  float m_sdrPeak{100.0f / 10000.0f};

  GLint m_hPos{-1};
  GLint m_hTex{-1};
  GLint m_hSamp{-1};
  GLint m_hProj{-1};
  GLint m_hSdrPeak{-1};
};
