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


#include "system.h"

#if HAS_GLES == 2

#include "GUIShader.h"
#include "MatrixGLES.h"
#include "utils/log.h"

CGUIShader::CGUIShader() : CGLSLShaderProgram("guishader_vert.glsl", "guishader_frag.glsl")
{
  // Initialise values
  m_hTex0   = NULL;
  m_hTex1   = NULL;
  m_hMethod = NULL;
  m_hProj   = NULL;
  m_hModel  = NULL;
  m_hPos    = NULL;
  m_hCol    = NULL;
  m_hCord0  = NULL;
  m_hCord1  = NULL;

  m_method = SM_DEFAULT;
  m_proj   = NULL;
  m_model  = NULL;
}

void CGUIShader::OnCompiledAndLinked()
{
  // This is called after CompileAndLink()

  // Variables passed directly to the Fragment shader
  m_hTex0   = glGetUniformLocation(ProgramHandle(), "m_samp0");
  m_hTex1   = glGetUniformLocation(ProgramHandle(), "m_samp1");
  m_hMethod = glGetUniformLocation(ProgramHandle(), "m_method");
  // Variables passed directly to the Vertex shader
  m_hProj   = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel  = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hPos    = glGetAttribLocation(ProgramHandle(),  "m_attrpos");
  m_hCol    = glGetAttribLocation(ProgramHandle(),  "m_attrcol");
  m_hCord0  = glGetAttribLocation(ProgramHandle(),  "m_attrcord0");
  m_hCord1  = glGetAttribLocation(ProgramHandle(),  "m_attrcord1");
}

bool CGUIShader::OnEnabled()
{
  // This is called after glUseProgram()

  glUniform1i(m_hMethod, (int)m_method);
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, g_matrices.GetMatrix(MM_PROJECTION));
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, g_matrices.GetMatrix(MM_MODELVIEW));

  if (m_method == SM_TEXTURE)
  {
    glUniform1i(m_hTex0, 0);
  }
  else if (m_method == SM_MULTI)
  {
    glUniform1i(m_hTex0, 0);
    glUniform1i(m_hTex1, 1);
  }

  return true;
}

void CGUIShader::Free()
{
  // Do Cleanup here
  CGLSLShaderProgram::Free();
}

#endif
