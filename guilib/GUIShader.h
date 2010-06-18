/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#ifndef GUI_SHADER_H
#define GUI_SHADER_H

#pragma once

#include "Shader.h"

using namespace Shaders;

enum ESHADERMETHOD
{
  SM_DEFAULT = 0,
  SM_TEXTURE = 1,
  SM_MULTI   = 2,
  SM_FONTS   = 3,
  SM_TEXTURE_NOBLEND = 4,
  SM_ESHADERCOUNT
};

class CGUIShader : public CGLSLShaderProgram
{
public:
  CGUIShader( const char *shader = 0 );
  void OnCompiledAndLinked();
  bool OnEnabled();
  void Free();

  void Setup(ESHADERMETHOD method) { m_method = method; }

  GLint GetPosLoc()   { return m_hPos;   }
  GLint GetColLoc()   { return m_hCol;   }
  GLint GetCord0Loc() { return m_hCord0; }
  GLint GetCord1Loc() { return m_hCord1; }
  
protected:
  GLint m_hTex0;
  GLint m_hTex1;
  GLint m_hMethod;
  GLint m_hProj;
  GLint m_hModel;
  GLint m_hPos;
  GLint m_hCol;
  GLint m_hCord0;
  GLint m_hCord1;

  ESHADERMETHOD m_method;
  GLfloat *m_proj;
  GLfloat *m_model;
};

#endif // GUI_SHADER_H
