/*
 *      Copyright (c) 2007 d4rk
 *      Copyright (C) 2007-2013 Team XBMC
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

#include "system.h"
#include "../RenderFlags.h"
#include "YUV2RGBShaderGLES.h"
#include "YUVMatrix.h"
#include "settings/AdvancedSettings.h"
#include "guilib/TransformMatrix.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

#include <string>
#include <sstream>

using namespace Shaders;

static void CalculateYUVMatrixGL(GLfloat      res[4][4]
                               , unsigned int flags
                               , EShaderFormat format
                               , float        black
                               , float        contrast
                               , bool         limited)
{
  TransformMatrix matrix;
  CalculateYUVMatrix(matrix, flags, format, black, contrast, limited);

  for(int row = 0; row < 3; row++)
    for(int col = 0; col < 4; col++)
      res[col][row] = matrix.m[row][col];

  res[0][3] = 0.0f;
  res[1][3] = 0.0f;
  res[2][3] = 0.0f;
  res[3][3] = 1.0f;
}

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(unsigned flags, EShaderFormat format, bool stretch,
                                             GLSLOutput *output)
{
  m_width = 1;
  m_height = 1;
  m_field = 0;
  m_flags = flags;
  m_format = format;

  m_black = 0.0f;
  m_contrast = 1.0f;

  m_stretch = 0.0f;

  // shader attribute handles
  m_hYTex = -1;
  m_hUTex = -1;
  m_hVTex = -1;
  m_hMatrix = -1;
  m_hStretch = -1;
  m_hStep = -1;

  m_hVertex = -1;
  m_hYcoord = -1;
  m_hUcoord = -1;
  m_hVcoord = -1;
  m_hProj = -1;
  m_hModel = -1;
  m_hAlpha = -1;

  m_proj = nullptr;
  m_model = nullptr;
  m_alpha = 1.0;

  // get defines from the output stage if used
  m_glslOutput = output;
  if (m_glslOutput)
    m_defines += m_glslOutput->GetDefines();

  // not supported on GLES 2.0
  m_defines += "#define XBMC_texture_rectangle 0\n";
  m_defines += "#define XBMC_texture_rectangle_hack 0\n";

  //don't compile in stretch support when it's not needed
  if (stretch)
    m_defines += "#define XBMC_STRETCH 1\n";
  else
    m_defines += "#define XBMC_STRETCH 0\n";

  if (m_format == SHADER_YV12 ||
      m_format == SHADER_YV12_9 ||
      m_format == SHADER_YV12_10 ||
      m_format == SHADER_YV12_12 ||
      m_format == SHADER_YV12_14 ||
      m_format == SHADER_YV12_16)
    m_defines += "#define XBMC_YV12\n";
  else if (m_format == SHADER_NV12)
    m_defines += "#define XBMC_NV12\n";
  else if (m_format == SHADER_YUY2)
    m_defines += "#define XBMC_YUY2\n";
  else if (m_format == SHADER_UYVY)
    m_defines += "#define XBMC_UYVY\n";
  else if (m_format == SHADER_NV12_RRG)
    m_defines += "#define XBMC_NV12_RRG\n";
  else if (m_format == SHADER_YV12)
    m_defines += "#define XBMC_YV12\n";
  else
    CLog::Log(LOGERROR, "GL: BaseYUV2RGBGLSLShader - unsupported format %d", m_format);

  VertexShader()->LoadSource("yuv2rgb_vertex_gles.glsl", m_defines);

  CLog::Log(LOGDEBUG, "GL: BaseYUV2RGBGLSLShader: defines:\n%s", m_defines.c_str());
}

BaseYUV2RGBGLSLShader::~BaseYUV2RGBGLSLShader()
{
  delete m_glslOutput;
}

void BaseYUV2RGBGLSLShader::OnCompiledAndLinked()
{
  m_hVertex = glGetAttribLocation(ProgramHandle(),  "m_attrpos");
  m_hYcoord = glGetAttribLocation(ProgramHandle(),  "m_attrcordY");
  m_hUcoord = glGetAttribLocation(ProgramHandle(),  "m_attrcordU");
  m_hVcoord = glGetAttribLocation(ProgramHandle(),  "m_attrcordV");
  m_hProj = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "m_alpha");
  m_hYTex = glGetUniformLocation(ProgramHandle(), "m_sampY");
  m_hUTex = glGetUniformLocation(ProgramHandle(), "m_sampU");
  m_hVTex = glGetUniformLocation(ProgramHandle(), "m_sampV");
  m_hMatrix = glGetUniformLocation(ProgramHandle(), "m_yuvmat");
  m_hStretch = glGetUniformLocation(ProgramHandle(), "m_stretch");
  m_hStep = glGetUniformLocation(ProgramHandle(), "m_step");
  VerifyGLState();

  if (m_glslOutput) m_glslOutput->OnCompiledAndLinked(ProgramHandle());
}

bool BaseYUV2RGBGLSLShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, 0);
  glUniform1i(m_hUTex, 1);
  glUniform1i(m_hVTex, 2);
  glUniform1f(m_hStretch, m_stretch);
  glUniform2f(m_hStep, 1.0 / m_width, 1.0 / m_height);

  GLfloat matrix[4][4];
  // keep video levels
  CalculateYUVMatrixGL(matrix, m_flags, m_format, m_black, m_contrast, !m_convertFullRange);

  glUniformMatrix4fv(m_hMatrix, 1, GL_FALSE, (GLfloat*)matrix);
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  glUniform1f(m_hAlpha, m_alpha);
  VerifyGLState();
  if (m_glslOutput) m_glslOutput->OnEnabled();
  return true;
}

void BaseYUV2RGBGLSLShader::OnDisabled()
{
  if (m_glslOutput) m_glslOutput->OnDisabled();
}

void BaseYUV2RGBGLSLShader::Free()
{
  if (m_glslOutput) m_glslOutput->Free();
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(unsigned flags, EShaderFormat format, bool stretch,
                                                   GLSLOutput *output)
  : BaseYUV2RGBGLSLShader(flags, format, stretch, output)
{
  PixelShader()->LoadSource("yuv2rgb_basic.glsl", m_defines);
  PixelShader()->AppendSource("output.glsl");
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader(unsigned flags, EShaderFormat format)
  : BaseYUV2RGBGLSLShader(flags, format, false)
{
  m_hStepX = -1;
  m_hStepY = -1;
  m_hField = -1;

  PixelShader()->LoadSource("yuv2rgb_bob_gles.glsl", m_defines);
}

void YUV2RGBBobShader::OnCompiledAndLinked()
{
  BaseYUV2RGBGLSLShader::OnCompiledAndLinked();
  m_hStepX = glGetUniformLocation(ProgramHandle(), "m_stepX");
  m_hStepY = glGetUniformLocation(ProgramHandle(), "m_stepY");
  m_hField = glGetUniformLocation(ProgramHandle(), "m_field");
  VerifyGLState();
}

bool YUV2RGBBobShader::OnEnabled()
{
  if(!BaseYUV2RGBGLSLShader::OnEnabled())
    return false;

  glUniform1i(m_hField, m_field);
  glUniform1f(m_hStepX, 1.0f / (float)m_width);
  glUniform1f(m_hStepY, 1.0f / (float)m_height);
  VerifyGLState();
  return true;
}
