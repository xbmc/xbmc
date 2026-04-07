/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GuiCompositeShaderGLES.h"

#include "utils/log.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <cmath>

namespace
{
// ST2084 (PQ) constants
constexpr float ST2084_m1 = 0.1593017578125f; // 2610/16384
constexpr float ST2084_m2 = 78.84375f; // 2523/4096 * 128
constexpr float ST2084_c1 = 0.8359375f; // 3424/4096
constexpr float ST2084_c2 = 18.8515625f; // 2413/4096 * 32
constexpr float ST2084_c3 = 18.6875f; // 2392/4096 * 32

float ForwardPQ(float L)
{
  float Lm1 = std::pow(L, ST2084_m1);
  return std::pow((ST2084_c1 + ST2084_c2 * Lm1) / (1.0f + ST2084_c3 * Lm1), ST2084_m2);
}

} // namespace

CGuiCompositeShaderGLES::CGuiCompositeShaderGLES()
{
  VertexShader()->LoadSource("gles_gui_composite.vert");
  PixelShader()->LoadSource("gles_gui_composite.frag");
}

CGuiCompositeShaderGLES::~CGuiCompositeShaderGLES()
{
  if (m_lutDegammaTexId)
    glDeleteTextures(1, &m_lutDegammaTexId);
  if (m_lutTFTexId)
    glDeleteTextures(1, &m_lutTFTexId);
}

void CGuiCompositeShaderGLES::OnCompiledAndLinked()
{
  m_hPos = glGetAttribLocation(ProgramHandle(), "a_pos");
  m_hTex = glGetAttribLocation(ProgramHandle(), "a_tex");
  m_hSamp = glGetUniformLocation(ProgramHandle(), "u_samp");
  m_hLutDegamma = glGetUniformLocation(ProgramHandle(), "u_lutDegamma");
  m_hLutTF = glGetUniformLocation(ProgramHandle(), "u_lutTF");
  m_hProj = glGetUniformLocation(ProgramHandle(), "u_proj");
  m_hOotfGamma = glGetUniformLocation(ProgramHandle(), "u_ootfGamma");
  glUseProgram(ProgramHandle());
  glUniform1i(m_hSamp, 0);
  glUniform1i(m_hLutDegamma, 1);
  glUniform1i(m_hLutTF, 2);
  glUseProgram(0);
}

bool CGuiCompositeShaderGLES::OnEnabled()
{
  if (m_proj)
    glUniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);

  glUniform1f(m_hOotfGamma, m_ootfGamma);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_lutDegammaTexId);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_lutTFTexId);
  glActiveTexture(GL_TEXTURE0);

  return true;
}

GLuint CGuiCompositeShaderGLES::CreateLUTTexture(const std::vector<float>& data)
{
  GLuint texId;
  glGenTextures(1, &texId);
  glBindTexture(GL_TEXTURE_2D, texId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, data.size(), 1, 0, GL_LUMINANCE, GL_FLOAT,
               data.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texId;
}

std::vector<float> CGuiCompositeShaderGLES::GenerateDegammaLUT()
{
  std::vector<float> lut(LUT_SIZE);
  for (int i = 0; i < LUT_SIZE; i++)
  {
    float x = static_cast<float>(i) / (LUT_SIZE - 1);
    lut[i] = std::pow(x, 2.2f);
  }
  return lut;
}

std::vector<float> CGuiCompositeShaderGLES::GeneratePQLUT(float sdrPeak)
{
  // PQ is display-referred (absolute luminance). sdrPeak is in PQ-normalized
  // units (nits / 10000), e.g. 203 nits = 0.0203. The LUT maps the full [0,1]
  // texture coordinate range to ForwardPQ([0, sdrPeak]), giving full LUT
  // resolution across the actual SDR luminance range.
  std::vector<float> lut(LUT_SIZE);
  for (int i = 0; i < LUT_SIZE; i++)
  {
    float L = static_cast<float>(i) / (LUT_SIZE - 1) * sdrPeak;
    lut[i] = ForwardPQ(L);
  }
  return lut;
}

bool CGuiCompositeShaderGLES::CreateLUTs(int colorTransfer)
{
  if (m_lutDegammaTexId)
    glDeleteTextures(1, &m_lutDegammaTexId);
  if (m_lutTFTexId)
    glDeleteTextures(1, &m_lutTFTexId);

  m_lutDegammaTexId = CreateLUTTexture(GenerateDegammaLUT());
  if (!m_lutDegammaTexId)
  {
    CLog::Log(LOGERROR, "CGuiCompositeShaderGLES::CreateLUTs - failed to create degamma LUT");
    return false;
  }

  if (colorTransfer == AVCOL_TRC_SMPTE2084)
  {
    m_lutTFTexId = CreateLUTTexture(GeneratePQLUT(m_sdrPeak));
    if (!m_lutTFTexId)
    {
      CLog::Log(LOGERROR, "CGuiCompositeShaderGLES::CreateLUTs - failed to create PQ LUT");
      return false;
    }
    m_ootfGamma = 0.0f;
    CLog::Log(LOGDEBUG, "CGuiCompositeShaderGLES::CreateLUTs - created PQ LUT ({} entries)",
              LUT_SIZE);
  }
  else if (colorTransfer == AVCOL_TRC_ARIB_STD_B67)
  {
    // HLG: no TF LUT needed, shader computes OETF + inverse OOTF directly.
    // BT.2100: gamma = 1.2 + 0.42 * log10(Lw/1000). For 1000-nit ref: 1.2.
    m_ootfGamma = 1.2f;
    CLog::Log(LOGDEBUG, "CGuiCompositeShaderGLES::CreateLUTs - HLG mode (gamma {})", m_ootfGamma);
  }
  else
  {
    CLog::Log(LOGERROR, "CGuiCompositeShaderGLES::CreateLUTs - unsupported transfer function {}",
              colorTransfer);
    return false;
  }
  return true;
}
