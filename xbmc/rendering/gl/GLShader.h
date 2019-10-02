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

class CGLShader : public Shaders::CGLSLShaderProgram
{
public:
  CGLShader(const char *shader, std::string prefix);
  CGLShader(const char *vshader, const char *fshader, std::string prefix);
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;
  void Free();

  GLint GetPosLoc() {return m_hPos;}
  GLint GetColLoc() {return m_hCol;}
  GLint GetCord0Loc() {return m_hCord0;}
  GLint GetCord1Loc() {return m_hCord1;}
  GLint GetUniColLoc() {return m_hUniCol;}
  GLint GetModelLoc() {return m_hModel; }
  bool HardwareClipIsPossible() {return m_clipPossible; }
  GLfloat GetClipXFactor() {return m_clipXFactor; }
  GLfloat GetClipXOffset() {return m_clipXOffset; }
  GLfloat GetClipYFactor() {return m_clipYFactor; }
  GLfloat GetClipYOffset() {return m_clipYOffset; }

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

  const GLfloat *m_proj = nullptr;
  const GLfloat *m_model = nullptr;

  bool m_clipPossible = false;
  GLfloat m_clipXFactor;
  GLfloat m_clipXOffset;
  GLfloat m_clipYFactor;
  GLfloat m_clipYOffset;
};
