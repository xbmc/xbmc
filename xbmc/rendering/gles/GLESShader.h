/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/Shader.h"

#include <string>

class CGLESShader : public Shaders::CGLSLShaderProgram
{
public:
  CGLESShader(const char* shader, const std::string& prefix);
  CGLESShader(const char* vshader, const char* fshader, const std::string& prefix);
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;
  void Free();

  GLint GetPosLoc() const { return m_hPos;   }
  GLint GetColLoc() const { return m_hCol;   }
  GLint GetCord0Loc() const { return m_hCord0; }
  GLint GetCord1Loc() const { return m_hCord1; }
  GLint GetUniColLoc() const { return m_hUniCol; }
  GLint GetCoord0MatrixLoc() const { return m_hCoord0Matrix; }
  GLint GetFieldLoc() const { return m_hField; }
  GLint GetStepLoc() const { return m_hStep; }
  GLint GetContrastLoc() const { return m_hContrast; }
  GLint GetBrightnessLoc() const { return m_hBrightness; }
  GLint GetModelLoc() const { return m_hModel; }
  bool HardwareClipIsPossible() const { return m_clipPossible; }
  GLfloat GetClipXFactor() const { return m_clipXFactor; }
  GLfloat GetClipXOffset() const { return m_clipXOffset; }
  GLfloat GetClipYFactor() const { return m_clipYFactor; }
  GLfloat GetClipYOffset() const { return m_clipYOffset; }

protected:
  GLint m_hTex0 = 0;
  GLint m_hTex1 = 0;
  GLint m_hUniCol = 0;
  GLint m_hProj = 0;
  GLint m_hModel = 0;
  GLint m_hPos = 0;
  GLint m_hCol = 0;
  GLint m_hCord0 = 0;
  GLint m_hCord1 = 0;
  GLint m_hCoord0Matrix = 0;
  GLint m_hField = 0;
  GLint m_hStep = 0;
  GLint m_hContrast = 0;
  GLint m_hBrightness = 0;

  const GLfloat *m_proj;
  const GLfloat *m_model;

  bool m_clipPossible;
  GLfloat m_clipXFactor;
  GLfloat m_clipXOffset;
  GLfloat m_clipYFactor;
  GLfloat m_clipYOffset;

  GLfloat m_sdrPeak;
};
