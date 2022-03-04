/*
 *  Copyright (c) 2007 d4rk
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoFilterShaderGL.h"

#include "ConvolutionKernels.h"
#include "ServiceBroker.h"
#include "rendering/RenderSystem.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <math.h>
#include <string>

#define TEXTARGET GL_TEXTURE_1D

using namespace Shaders::GL;

//////////////////////////////////////////////////////////////////////
// BaseVideoFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

BaseVideoFilterShader::BaseVideoFilterShader()
{
  m_width = 1;
  m_height = 1;
  m_stepX = 0;
  m_stepY = 0;
  m_stretch = 0.0f;

  VertexShader()->LoadSource("gl_videofilter_vertex.glsl");

  PixelShader()->LoadSource("gl_videofilter_frag.glsl");
}

BaseVideoFilterShader::~BaseVideoFilterShader()
{
  Free();
}

//////////////////////////////////////////////////////////////////////
// ConvolutionFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

ConvolutionFilterShader::ConvolutionFilterShader(ESCALINGMETHOD method, bool stretch, GLSLOutput *output)
{
  m_method = method;

  std::string shadername;
  std::string defines;

  m_floattex = CServiceBroker::GetRenderSystem()->IsExtSupported("GL_ARB_texture_float");

  if (m_method == VS_SCALINGMETHOD_CUBIC_B_SPLINE ||
      m_method == VS_SCALINGMETHOD_CUBIC_MITCHELL ||
      m_method == VS_SCALINGMETHOD_CUBIC_CATMULL ||
      m_method == VS_SCALINGMETHOD_CUBIC_0_075 ||
      m_method == VS_SCALINGMETHOD_CUBIC_0_1 ||
      m_method == VS_SCALINGMETHOD_LANCZOS2 ||
      m_method == VS_SCALINGMETHOD_SPLINE36_FAST ||
      m_method == VS_SCALINGMETHOD_LANCZOS3_FAST)
  {
    shadername = "gl_convolution-4x4.glsl";

    if (m_floattex)
      m_internalformat = GL_RGBA16F;
    else
      m_internalformat = GL_RGBA;
  }
  else if (m_method == VS_SCALINGMETHOD_SPLINE36 ||
           m_method == VS_SCALINGMETHOD_LANCZOS3)
  {
    shadername = "gl_convolution-6x6.glsl";

    if (m_floattex)
      m_internalformat = GL_RGB16F;
    else
      m_internalformat = GL_RGB;
  }

  if (m_floattex)
    defines = "#define HAS_FLOAT_TEXTURE\n";

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

  CLog::Log(LOGDEBUG, "GL: using scaling method: {}", m_method);
  CLog::Log(LOGDEBUG, "GL: using shader: {}", shadername);

  PixelShader()->LoadSource(shadername, defines);
  PixelShader()->AppendSource("gl_output.glsl");
}

ConvolutionFilterShader::~ConvolutionFilterShader()
{
  Free();
  delete m_glslOutput;
}

void ConvolutionFilterShader::OnCompiledAndLinked()
{
  // obtain shader attribute handles on successful compilation
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStepXY = glGetUniformLocation(ProgramHandle(), "stepxy");
  m_hKernTex = glGetUniformLocation(ProgramHandle(), "kernelTex");
  m_hStretch = glGetUniformLocation(ProgramHandle(), "m_stretch");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "m_alpha");
  m_hProj = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hVertex = glGetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hCoord = glGetAttribLocation(ProgramHandle(), "m_attrcord");

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

  glTexImage1D(TEXTARGET, 0, m_internalformat, kernel.GetSize(), 0, GL_RGBA, format, data);

  glActiveTexture(GL_TEXTURE0);

  VerifyGLState();

  if (m_glslOutput)
    m_glslOutput->OnCompiledAndLinked(ProgramHandle());
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
  glUniform1f(m_hAlpha, m_alpha);

  glUniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);

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

//////////////////////////////////////////////////////////////////////
// StretchFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

StretchFilterShader::StretchFilterShader()
{
  PixelShader()->LoadSource("gl_stretch.glsl");
}

void StretchFilterShader::OnCompiledAndLinked()
{
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStretch = glGetUniformLocation(ProgramHandle(), "m_stretch");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "m_alpha");
  m_hProj = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hVertex = glGetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hCoord = glGetAttribLocation(ProgramHandle(), "m_attrcord");
}

bool StretchFilterShader::OnEnabled()
{
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1f(m_hStretch, m_stretch);
  glUniform1f(m_hAlpha, m_alpha);
  glUniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  VerifyGLState();
  return true;
}

//////////////////////////////////////////////////////////////////////
// DefaultFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

void DefaultFilterShader::OnCompiledAndLinked()
{
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hProj = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hVertex = glGetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hCoord = glGetAttribLocation(ProgramHandle(), "m_attrcord");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "m_alpha");
}

bool DefaultFilterShader::OnEnabled()
{
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1f(m_hAlpha, m_alpha);
  glUniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  VerifyGLState();
  return true;
}
