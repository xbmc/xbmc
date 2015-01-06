/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef GUI_SHADER_H
#define GUI_SHADER_H

#pragma once

#include "Shader.h"

class CGUIShader : public Shaders::CGLSLShaderProgram
{
public:
  CGUIShader( const char *shader = 0 );
  void OnCompiledAndLinked();
  bool OnEnabled();
  void Free();

  GLint GetPosLoc()   { return m_hPos;   }
  GLint GetColLoc()   { return m_hCol;   }
  GLint GetCord0Loc() { return m_hCord0; }
  GLint GetCord1Loc() { return m_hCord1; }
  GLint GetUniColLoc() { return m_hUniCol; }
  GLint GetCoord0MatrixLoc() { return m_hCoord0Matrix; }
  GLint GetFieldLoc() { return m_hField; }
  GLint GetStepLoc() { return m_hStep; }

protected:
  GLint m_hTex0;
  GLint m_hTex1;
  GLint m_hUniCol;
  GLint m_hProj;
  GLint m_hModel;
  GLint m_hPos;
  GLint m_hCol;
  GLint m_hCord0;
  GLint m_hCord1;
  GLint m_hCoord0Matrix;
  GLint m_hField;
  GLint m_hStep;

  GLfloat *m_proj;
  GLfloat *m_model;
};

#endif // GUI_SHADER_H
