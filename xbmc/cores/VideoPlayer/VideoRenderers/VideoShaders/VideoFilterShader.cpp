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

#if defined(HAS_GL) || HAS_GLES >= 2
#include <string>
#include <math.h>

#include "VideoFilterShader.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "ConvolutionKernels.h"
#include "windowing/WindowingFactory.h"

#if defined(HAS_GL)
  #define USE1DTEXTURE
  #define TEXTARGET GL_TEXTURE_1D
#else
  #define TEXTARGET GL_TEXTURE_2D
#endif

using namespace Shaders;

//////////////////////////////////////////////////////////////////////
// BaseVideoFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

BaseVideoFilterShader::BaseVideoFilterShader()
{
  m_width = 1;
  m_height = 1;
  m_hStepXY = 0;
  m_stepX = 0;
  m_stepY = 0;
  m_sourceTexUnit = 0;
  m_hSourceTex = 0;

  m_stretch = 0.0f;

  std::string shaderv =
    "varying vec2 cord;"
    "void main()"
    "{"
    "cord = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);"
    "gl_Position = ftransform();"
    "gl_FrontColor = gl_Color;"
    "}";
  VertexShader()->SetSource(shaderv);

  std::string shaderp =
    "uniform sampler2D img;"
    "varying vec2 cord;"
    "void main()"
    "{"
    "gl_FragColor.rgb = texture2D(img, cord).rgb;"
    "gl_FragColor.a = gl_Color.a;"
    "}";
  PixelShader()->SetSource(shaderp);
}

ConvolutionFilterShader::ConvolutionFilterShader(ESCALINGMETHOD method, bool stretch, GLSLOutput *output)
{
  m_method = method;
  m_kernelTex1 = 0;

  std::string shadername;
  std::string defines;

#if defined(HAS_GL)
  m_floattex = g_Windowing.IsExtSupported("GL_ARB_texture_float");
#elif HAS_GLES >= 2
  m_floattex = false;
#endif

  if (m_method == VS_SCALINGMETHOD_CUBIC ||
      m_method == VS_SCALINGMETHOD_LANCZOS2 ||
      m_method == VS_SCALINGMETHOD_SPLINE36_FAST ||
      m_method == VS_SCALINGMETHOD_LANCZOS3_FAST)
  {
    shadername = "convolution-4x4.glsl";
#if defined(HAS_GL)
    if (m_floattex)
      m_internalformat = GL_RGBA16F_ARB;
    else
#endif
      m_internalformat = GL_RGBA;
  }
  else if (m_method == VS_SCALINGMETHOD_SPLINE36 || 
           m_method == VS_SCALINGMETHOD_LANCZOS3)
  {
    shadername = "convolution-6x6.glsl";
#if defined(HAS_GL)
    if (m_floattex)
      m_internalformat = GL_RGB16F_ARB;
    else
#endif
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

  // get defines from the output stage if used
  m_glslOutput = output;
  if (m_glslOutput) {
    defines += m_glslOutput->GetDefines();
  }

  //tell shader if we're using a 1D texture
#ifdef USE1DTEXTURE
  defines += "#define USE1DTEXTURE 1\n";
#else
  defines += "#define USE1DTEXTURE 0\n";
#endif

  CLog::Log(LOGDEBUG, "GL: ConvolutionFilterShader: using %s defines:\n%s", shadername.c_str(), defines.c_str());
  PixelShader()->LoadSource(shadername, defines);
  PixelShader()->AppendSource("output.glsl");
}

ConvolutionFilterShader::~ConvolutionFilterShader()
{
  delete m_glslOutput;
}

void ConvolutionFilterShader::OnCompiledAndLinked()
{
  // obtain shader attribute handles on successful compilation
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStepXY    = glGetUniformLocation(ProgramHandle(), "stepxy");
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

  //make a kernel texture on GL_TEXTURE2 and set clamping and interpolation
  //TEXTARGET is set to GL_TEXTURE_1D or GL_TEXTURE_2D
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(TEXTARGET, m_kernelTex1);
  glTexParameteri(TEXTARGET, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(TEXTARGET, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(TEXTARGET, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTARGET, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  //if float textures are supported, we can load the kernel as a float texture
  //if not we load it as 8 bit unsigned which gets converted back to float in the shader
  GLenum  format;
  GLvoid* data;
  if (m_floattex)
  {
    format = GL_FLOAT;
    data   = (GLvoid*)kernel.GetFloatPixels();
  }
  else
  {
    format = GL_UNSIGNED_BYTE;
    data   = (GLvoid*)kernel.GetUint8Pixels();
  }

  //upload as 1D texture or as 2D texture with height of 1
#ifdef USE1DTEXTURE
  glTexImage1D(TEXTARGET, 0, m_internalformat, kernel.GetSize(), 0, GL_RGBA, format, data);
#else
  glTexImage2D(TEXTARGET, 0, m_internalformat, kernel.GetSize(), 1, 0, GL_RGBA, format, data);
#endif

  glActiveTexture(GL_TEXTURE0);

  VerifyGLState();

  if (m_glslOutput) m_glslOutput->OnCompiledAndLinked(ProgramHandle());
}

bool ConvolutionFilterShader::OnEnabled()
{
  // set shader attributes once enabled
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(TEXTARGET, m_kernelTex1);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1i(m_hKernTex, 2);
  glUniform2f(m_hStepXY, m_stepX, m_stepY);
  glUniform1f(m_hStretch, m_stretch);
  VerifyGLState();
  if (m_glslOutput) m_glslOutput->OnEnabled();
  return true;
}

void ConvolutionFilterShader::OnDisabled()
{
  if (m_glslOutput) m_glslOutput->OnDisabled();
}

void ConvolutionFilterShader::Free()
{
  if (m_kernelTex1)
    glDeleteTextures(1, &m_kernelTex1);
  m_kernelTex1 = 0;
  if (m_glslOutput) m_glslOutput->Free();
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

void DefaultFilterShader::OnCompiledAndLinked()
{
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
}

bool DefaultFilterShader::OnEnabled()
{
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  VerifyGLState();
  return true;
}

#endif
