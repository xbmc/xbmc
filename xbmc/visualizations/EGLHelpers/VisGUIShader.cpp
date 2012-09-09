/*
*      Copyright (C) 2005-2012 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "VisGUIShader.h"

CVisGUIShader::CVisGUIShader(const char *vert, const char *frag ) : CVisGLSLShaderProgram(vert, frag)
{
  // Initialise values
  m_hTex0   = 0;
  m_hTex1   = 0;
  m_hProj   = 0;
  m_hModel  = 0;
  m_hPos    = 0;
  m_hCol    = 0;
  m_hCord0  = 0;
  m_hCord1  = 0;

  m_proj   = NULL;
  m_model  = NULL;
}

void CVisGUIShader::OnCompiledAndLinked()
{
  // This is called after CompileAndLink()

  // Variables passed directly to the Fragment shader
  m_hTex0   = glGetUniformLocation(ProgramHandle(), "m_samp0");
  m_hTex1   = glGetUniformLocation(ProgramHandle(), "m_samp1");
  // Variables passed directly to the Vertex shader
  m_hProj   = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel  = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hPos    = glGetAttribLocation(ProgramHandle(),  "m_attrpos");
  m_hCol    = glGetAttribLocation(ProgramHandle(),  "m_attrcol");
  m_hCord0  = glGetAttribLocation(ProgramHandle(),  "m_attrcord0");
  m_hCord1  = glGetAttribLocation(ProgramHandle(),  "m_attrcord1");

  // It's okay to do this only one time. Textures units never change.
  glUseProgram( ProgramHandle() );
  glUniform1i(m_hTex0, 0);
  glUniform1i(m_hTex1, 1);
  glUseProgram( 0 );
}

bool CVisGUIShader::OnEnabled()
{
  // This is called after glUseProgram()
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, GetMatrix(MM_PROJECTION));
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, GetMatrix(MM_MODELVIEW));

  return true;
}

void CVisGUIShader::Free()
{
  // Do Cleanup here
  CVisShaderProgram::Free();
}
