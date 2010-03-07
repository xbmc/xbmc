/*
* XBMC Media Center
* Video Filter Classes
* Copyright (c) 2007 d4rk
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "system.h"
#include "VideoFilterShader.h"
#include "utils/log.h"
#include "ConvolutionKernels.h"

#include <string>
#include <math.h>

#ifdef HAS_GL

using namespace Shaders;
using namespace std;

//////////////////////////////////////////////////////////////////////
// BaseVideoFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

BaseVideoFilterShader::BaseVideoFilterShader()
{
  m_width = 1;
  m_height = 1;
  m_hStepX = 0;
  m_hStepY = 0;
  m_stepX = 0;
  m_stepY = 0;
  m_sourceTexUnit = 0;
  m_hSourceTex = 0;

  m_stretch = 0.0f;

  string shaderv = 
    "uniform float stepx;"
    "uniform float stepy;"
    "void main()"
    "{"
    "gl_TexCoord[0].xy = gl_MultiTexCoord0.xy - vec2(stepx * 0.5, stepy * 0.5);"
    "gl_Position = ftransform();"
    "gl_FrontColor = gl_Color;"
    "}";
  VertexShader()->SetSource(shaderv);
}

ConvolutionFilterShader::ConvolutionFilterShader(ESCALINGMETHOD method, bool stretch)
{
  m_method = method;
  m_kernelTex1 = 0;

  string shadername;
  string defines;

  m_floattex = glewIsSupported("GL_ARB_texture_float");

  if (m_method == VS_SCALINGMETHOD_CUBIC ||
      m_method == VS_SCALINGMETHOD_LANCZOS2 ||
      m_method == VS_SCALINGMETHOD_LANCZOS3_FAST)
  {
    shadername = "convolution-4x4.glsl";
    if (m_floattex)
      m_internalformat = GL_RGBA16F_ARB;
    else
      m_internalformat = GL_RGBA;
  }
  else if (m_method == VS_SCALINGMETHOD_LANCZOS3)
  {
    shadername = "convolution-6x6.glsl";
    if (m_floattex)
      m_internalformat = GL_RGB16F_ARB;
    else
      m_internalformat = GL_RGB;
  }

  if (m_floattex)
    defines = "#define HAS_FLOAT_TEXTURE 1\n";
  else
    defines = "#define HAS_FLOAT_TEXTURE 0\n";

  //don't compile in stretch support when it's not needed
  if (stretch)
    defines += "#define XBMC_STRETCH 1\n";
  else
    defines += "#define XBMC_STRETCH 0\n";

  CLog::Log(LOGDEBUG, "GL: ConvolutionFilterShader: using %s defines: %s", shadername.c_str(), defines.c_str());
  PixelShader()->LoadSource(shadername, defines);
}

void ConvolutionFilterShader::OnCompiledAndLinked()
{
  // obtain shader attribute handles on successfull compilation
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStepX     = glGetUniformLocation(ProgramHandle(), "stepx");
  m_hStepY     = glGetUniformLocation(ProgramHandle(), "stepy");
  m_hKernTex   = glGetUniformLocation(ProgramHandle(), "kernelTex");
  m_hStretch   = glGetUniformLocation(ProgramHandle(), "m_stretch");

  CConvolutionKernel kernel(m_method, 256);

  if (m_kernelTex1)
  {
    glDeleteTextures(1, &m_kernelTex1);
    m_kernelTex1 = 0;
  }

  glGenTextures(1, &m_kernelTex1);

  if ((m_kernelTex1<=0))
  {
    CLog::Log(LOGERROR, "GL: ConvolutionFilterShader: Error creating kernel texture");
    return;
  }

  glActiveTexture(GL_TEXTURE2);

  //if float textures are supported, we can load the kernel as a 1d float texture
  //if not, we load it as a 2d texture with 2 rows, where row 0 contains the high byte
  //and row 1 contains the low byte, which can be converted in the shader
  if (m_floattex)
  {
    glBindTexture(GL_TEXTURE_1D, m_kernelTex1);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, m_internalformat, kernel.GetSize(), 0, GL_RGBA, GL_FLOAT, kernel.GetFloatPixels());
  }
  else
  {
    glBindTexture(GL_TEXTURE_2D, m_kernelTex1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, m_internalformat, kernel.GetSize(), 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, kernel.GetIntFractPixels());
  }

  glActiveTexture(GL_TEXTURE0);

  VerifyGLState();
}

bool ConvolutionFilterShader::OnEnabled()
{
  // set shader attributes once enabled
  glActiveTexture(GL_TEXTURE2);

  if (m_floattex)
    glBindTexture(GL_TEXTURE_1D, m_kernelTex1);
  else
    glBindTexture(GL_TEXTURE_2D, m_kernelTex1);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1i(m_hKernTex, 2);
  glUniform1f(m_hStepX, m_stepX);
  glUniform1f(m_hStepY, m_stepY);
  glUniform1f(m_hStretch, m_stretch);
  VerifyGLState();
  return true;
}

void ConvolutionFilterShader::Free()
{
  if (m_kernelTex1)
    glDeleteTextures(1, &m_kernelTex1);
  m_kernelTex1 = 0;
  BaseVideoFilterShader::Free();
}

StretchFilterShader::StretchFilterShader()
{
  PixelShader()->LoadSource("stretch.glsl");
}

void StretchFilterShader::OnCompiledAndLinked()
{
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStretch   = glGetUniformLocation(ProgramHandle(), "m_stretch");
}

bool StretchFilterShader::OnEnabled()
{
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1f(m_hStretch, m_stretch);
  VerifyGLState();
  return true;
}

#endif
