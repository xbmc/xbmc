/*
 *      Copyright (C) 2005-2017 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "guilib/Shader.h"

class CGLShader : public Shaders::CGLSLShaderProgram
{
public:
  CGLShader(const char *shader);
  CGLShader(const char *vshader, const char *fshader);
  void OnCompiledAndLinked();
  bool OnEnabled();
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

  GLfloat *m_proj = nullptr;
  GLfloat *m_model = nullptr;

  bool m_clipPossible = false;
  GLfloat m_clipXFactor;
  GLfloat m_clipXOffset;
  GLfloat m_clipYFactor;
  GLfloat m_clipYOffset;
};
