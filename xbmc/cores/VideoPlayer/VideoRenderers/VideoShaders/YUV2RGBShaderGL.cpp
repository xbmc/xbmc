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
#include "YUV2RGBShaderGL.h"
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

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(bool rect, unsigned flags, EShaderFormat format, bool stretch,
                                             GLSLOutput *output)
{
  m_width      = 1;
  m_height     = 1;
  m_field      = 0;
  m_flags      = flags;
  m_format     = format;

  m_black      = 0.0f;
  m_contrast   = 1.0f;

  m_stretch = 0.0f;

  // shader attribute handles
  m_hYTex    = -1;
  m_hUTex    = -1;
  m_hVTex    = -1;
  m_hStretch = -1;
  m_hStep    = -1;

  // get defines from the output stage if used
  m_glslOutput = output;
  if (m_glslOutput) {
    m_defines += m_glslOutput->GetDefines();
  }

  if(rect)
    m_defines += "#define XBMC_texture_rectangle 1\n";
  else
    m_defines += "#define XBMC_texture_rectangle 0\n";

  if(g_advancedSettings.m_GLRectangleHack)
    m_defines += "#define XBMC_texture_rectangle_hack 1\n";
  else
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

  VertexShader()->LoadSource("yuv2rgb_vertex.glsl", m_defines);

  CLog::Log(LOGDEBUG, "GL: BaseYUV2RGBGLSLShader: defines:\n%s", m_defines.c_str());
}

BaseYUV2RGBGLSLShader::~BaseYUV2RGBGLSLShader()
{
  delete m_glslOutput;
}

void BaseYUV2RGBGLSLShader::OnCompiledAndLinked()
{
  m_hYTex    = glGetUniformLocation(ProgramHandle(), "m_sampY");
  m_hUTex    = glGetUniformLocation(ProgramHandle(), "m_sampU");
  m_hVTex    = glGetUniformLocation(ProgramHandle(), "m_sampV");
  m_hMatrix  = glGetUniformLocation(ProgramHandle(), "m_yuvmat");
  m_hStretch = glGetUniformLocation(ProgramHandle(), "m_stretch");
  m_hStep    = glGetUniformLocation(ProgramHandle(), "m_step");
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
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////
BaseYUV2RGBARBShader::BaseYUV2RGBARBShader(unsigned flags, EShaderFormat format)
{
  m_width         = 1;
  m_height        = 1;
  m_field         = 0;
  m_flags         = flags;
  m_format        = format;

  // shader attribute handles
  m_hYTex  = -1;
  m_hUTex  = -1;
  m_hVTex  = -1;
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(bool rect, unsigned flags, EShaderFormat format, bool stretch,
                                                   GLSLOutput *output)
  : BaseYUV2RGBGLSLShader(rect, flags, format, stretch, output)
{
  PixelShader()->LoadSource("yuv2rgb_basic.glsl", m_defines);
  PixelShader()->AppendSource("output.glsl");
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader(bool rect, unsigned flags, EShaderFormat format)
  : BaseYUV2RGBGLSLShader(rect, flags, format, false)
{
  m_hStepX = -1;
  m_hStepY = -1;
  m_hField = -1;
  PixelShader()->LoadSource("yuv2rgb_bob.glsl", m_defines);
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

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShaderARB - YUV2RGB with no deinterlacing
//////////////////////////////////////////////////////////////////////
YUV2RGBProgressiveShaderARB::YUV2RGBProgressiveShaderARB(bool rect, unsigned flags, EShaderFormat format)
  : BaseYUV2RGBARBShader(flags, format)
{
  m_black      = 0.0f;
  m_contrast   = 1.0f;

  std::string shaderfile;

  if (m_format == SHADER_YUY2)
  {
    if(rect)
      shaderfile = "yuv2rgb_basic_rect_YUY2.arb";
    else
      shaderfile = "yuv2rgb_basic_2d_YUY2.arb";
  }
  else if (m_format == SHADER_UYVY)
  {
    if(rect)
      shaderfile = "yuv2rgb_basic_rect_UYVY.arb";
    else
      shaderfile = "yuv2rgb_basic_2d_UYVY.arb";
  }
  else
  {
    if(rect)
      shaderfile = "yuv2rgb_basic_rect.arb";
    else
      shaderfile = "yuv2rgb_basic_2d.arb";
  }

  CLog::Log(LOGDEBUG, "GL: YUV2RGBProgressiveShaderARB: loading %s", shaderfile.c_str());

  PixelShader()->LoadSource(shaderfile);
}

void YUV2RGBProgressiveShaderARB::OnCompiledAndLinked()
{
}

bool YUV2RGBProgressiveShaderARB::OnEnabled()
{
  GLfloat matrix[4][4];
  CalculateYUVMatrixGL(matrix, m_flags, m_format, m_black, m_contrast, !m_convertFullRange);

  for(int i=0;i<4;i++)
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, i
                               , matrix[0][i]
                               , matrix[1][i]
                               , matrix[2][i]
                               , matrix[3][i]);

  glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 4,
                               1.0 / m_width, 1.0 / m_height,
                               m_width, m_height);
  return true;
}
