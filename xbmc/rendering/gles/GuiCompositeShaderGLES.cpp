/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GuiCompositeShaderGLES.h"

#include "utils/log.h"

CGuiCompositeShaderGLES::CGuiCompositeShaderGLES()
{
  VertexShader()->LoadSource("gles_gui_composite.vert");
  PixelShader()->LoadSource("gles_gui_composite.frag");
}

void CGuiCompositeShaderGLES::OnCompiledAndLinked()
{
  m_hPos = glGetAttribLocation(ProgramHandle(), "a_pos");
  m_hTex = glGetAttribLocation(ProgramHandle(), "a_tex");
  m_hSamp = glGetUniformLocation(ProgramHandle(), "u_samp");
  m_hProj = glGetUniformLocation(ProgramHandle(), "u_proj");
  m_hSdrPeak = glGetUniformLocation(ProgramHandle(), "u_sdrPeak");

  glUseProgram(ProgramHandle());
  glUniform1i(m_hSamp, 0);
  glUseProgram(0);
}

bool CGuiCompositeShaderGLES::OnEnabled()
{
  if (m_proj)
    glUniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);

  glUniform1f(m_hSdrPeak, m_sdrPeak);
  return true;
}
