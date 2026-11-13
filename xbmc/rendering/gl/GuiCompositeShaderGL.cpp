/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GuiCompositeShaderGL.h"

#include "utils/log.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

CGuiCompositeShaderGL::CGuiCompositeShaderGL(const std::string& prefix)
{
  VertexShader()->LoadSource("gl_gui_composite.vert", prefix);
  PixelShader()->LoadSource("gl_gui_composite.frag", prefix);
}

void CGuiCompositeShaderGL::OnCompiledAndLinked()
{
  m_hPos = glGetAttribLocation(ProgramHandle(), "a_pos");
  m_hTex = glGetAttribLocation(ProgramHandle(), "a_tex");
  m_hSamp = glGetUniformLocation(ProgramHandle(), "u_samp");
  m_hProj = glGetUniformLocation(ProgramHandle(), "u_proj");
  m_hSdrPeak = glGetUniformLocation(ProgramHandle(), "u_sdrPeak");
  m_hOotfGamma = glGetUniformLocation(ProgramHandle(), "u_ootfGamma");

  glUseProgram(ProgramHandle());
  glUniform1i(m_hSamp, 0);
  glUseProgram(0);
}

bool CGuiCompositeShaderGL::OnEnabled()
{
  if (m_proj)
    glUniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);

  glUniform1f(m_hSdrPeak, m_sdrPeak);
  glUniform1f(m_hOotfGamma, m_ootfGamma);
  return true;
}

bool CGuiCompositeShaderGL::SetColorTransfer(int colorTransfer)
{
  if (colorTransfer == AVCOL_TRC_SMPTE2084)
  {
    m_ootfGamma = 0.0f;
    return true;
  }

  if (colorTransfer == AVCOL_TRC_ARIB_STD_B67)
  {
    // BT.2100: gamma = 1.2 + 0.42 * log10(Lw/1000). For 1000-nit ref: 1.2.
    m_ootfGamma = 1.2f;
    return true;
  }

  CLog::Log(LOGERROR, "CGuiCompositeShaderGL::SetColorTransfer - unsupported transfer function {}",
            colorTransfer);
  return false;
}
