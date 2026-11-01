/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GainMapShaderGLES.h"

#include "pictures/UltraHdr.h"
#include "rendering/MatrixGL.h"

CGainMapShaderGLES::CGainMapShaderGLES(const std::string& prefix)
{
  VertexShader()->LoadSource("gles_gainmap.vert", prefix);
  PixelShader()->LoadSource("gles_gainmap.frag", prefix);
}

void CGainMapShaderGLES::SetGainMap(const KODI::PICTURE::GainMapParams& params, float weight)
{
  m_min = params.min;
  m_max = params.max;
  m_gamma = params.gamma;
  m_offsetSdr = params.offsetSdr;
  m_offsetHdr = params.offsetHdr;
  m_weight = weight;
}

void CGainMapShaderGLES::SetGainMapScale(float x, float y)
{
  m_scaleX = x;
  m_scaleY = y;
}

void CGainMapShaderGLES::OnCompiledAndLinked()
{
  m_hPos = glGetAttribLocation(ProgramHandle(), "a_pos");
  m_hTex = glGetAttribLocation(ProgramHandle(), "a_tex");
  m_hProj = glGetUniformLocation(ProgramHandle(), "u_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "u_model");
  m_hPicture = glGetUniformLocation(ProgramHandle(), "u_picture");
  m_hGainMap = glGetUniformLocation(ProgramHandle(), "u_gainMap");
  m_hMin = glGetUniformLocation(ProgramHandle(), "u_min");
  m_hMax = glGetUniformLocation(ProgramHandle(), "u_max");
  m_hGamma = glGetUniformLocation(ProgramHandle(), "u_gamma");
  m_hOffsetSdr = glGetUniformLocation(ProgramHandle(), "u_offsetSdr");
  m_hOffsetHdr = glGetUniformLocation(ProgramHandle(), "u_offsetHdr");
  m_hWeight = glGetUniformLocation(ProgramHandle(), "u_weight");
  m_hScale = glGetUniformLocation(ProgramHandle(), "u_gainMapScale");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "u_alpha");

  glUseProgram(ProgramHandle());
  glUniform1i(m_hPicture, 0);
  glUniform1i(m_hGainMap, 1);
  glUseProgram(0);
}

bool CGainMapShaderGLES::OnEnabled()
{
  glUniformMatrix4fv(m_hProj, 1, GL_FALSE, glMatrixProject.Get());
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, glMatrixModview.Get());
  glUniform3fv(m_hMin, 1, m_min.data());
  glUniform3fv(m_hMax, 1, m_max.data());
  glUniform3fv(m_hGamma, 1, m_gamma.data());
  glUniform3fv(m_hOffsetSdr, 1, m_offsetSdr.data());
  glUniform3fv(m_hOffsetHdr, 1, m_offsetHdr.data());
  glUniform1f(m_hWeight, m_weight);
  glUniform2f(m_hScale, m_scaleX, m_scaleY);
  glUniform1f(m_hAlpha, m_alpha);
  return true;
}
