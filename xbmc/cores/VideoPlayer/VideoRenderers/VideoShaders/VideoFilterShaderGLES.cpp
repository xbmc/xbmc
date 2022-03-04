/*
 *  Copyright (c) 2007 d4rk
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoFilterShaderGLES.h"

#include "ConvolutionKernels.h"
#include "ServiceBroker.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <math.h>
#include <string>

using namespace Shaders::GLES;

//////////////////////////////////////////////////////////////////////
// BaseVideoFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

BaseVideoFilterShader::BaseVideoFilterShader()
{
  m_width = 1;
  m_height = 1;
  m_stepX = 0;
  m_stepY = 0;

  m_proj = nullptr;
  m_model = nullptr;

  VertexShader()->LoadSource("gles_videofilter.vert");

  PixelShader()->LoadSource("gles_videofilter.frag");
}

void BaseVideoFilterShader::OnCompiledAndLinked()
{
  m_hVertex = glGetAttribLocation(ProgramHandle(),  "m_attrpos");
  m_hcoord = glGetAttribLocation(ProgramHandle(),  "m_attrcord");
  m_hAlpha  = glGetUniformLocation(ProgramHandle(), "m_alpha");
  m_hProj  = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
}

bool BaseVideoFilterShader::OnEnabled()
{
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  glUniform1f(m_hAlpha, m_alpha);
  return true;
}

ConvolutionFilterShader::ConvolutionFilterShader(ESCALINGMETHOD method)
{
  m_method = method;

  std::string shadername;
  std::string defines;

  if (CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_color_buffer_float"))
  {
    m_floattex = true;
  }
  else
  {
    m_floattex = false;
  }

  if (m_method == VS_SCALINGMETHOD_CUBIC_B_SPLINE ||
      m_method == VS_SCALINGMETHOD_CUBIC_MITCHELL ||
      m_method == VS_SCALINGMETHOD_CUBIC_CATMULL ||
      m_method == VS_SCALINGMETHOD_CUBIC_0_075 ||
      m_method == VS_SCALINGMETHOD_CUBIC_0_1 ||
      m_method == VS_SCALINGMETHOD_LANCZOS2 ||
      m_method == VS_SCALINGMETHOD_SPLINE36_FAST ||
      m_method == VS_SCALINGMETHOD_LANCZOS3_FAST)
  {
    shadername = "gles_convolution-4x4.frag";
  }
  else if (m_method == VS_SCALINGMETHOD_SPLINE36 ||
           m_method == VS_SCALINGMETHOD_LANCZOS3)
  {
    shadername = "gles_convolution-6x6.frag";
  }

  if (m_floattex)
  {
    m_internalformat = GL_RGBA16F_EXT;
    defines = "#define HAS_FLOAT_TEXTURE\n";
  }
  else
  {
    m_internalformat = GL_RGBA;
  }

  CLog::Log(LOGDEBUG, "GLES: using scaling method: {}", m_method);
  CLog::Log(LOGDEBUG, "GLES: using shader: {}", shadername);

  PixelShader()->LoadSource(shadername, defines);
}

ConvolutionFilterShader::~ConvolutionFilterShader()
{
  Free();
}

void ConvolutionFilterShader::OnCompiledAndLinked()
{
  BaseVideoFilterShader::OnCompiledAndLinked();

  // obtain shader attribute handles on successful compilation
  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
  m_hStepXY    = glGetUniformLocation(ProgramHandle(), "stepxy");
  m_hKernTex   = glGetUniformLocation(ProgramHandle(), "kernelTex");

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
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_kernelTex1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

  //upload as 2D texture with height of 1
  glTexImage2D(GL_TEXTURE_2D, 0, m_internalformat, kernel.GetSize(), 1, 0, GL_RGBA, format, data);

  glActiveTexture(GL_TEXTURE0);

  VerifyGLState();
}

bool ConvolutionFilterShader::OnEnabled()
{
  BaseVideoFilterShader::OnEnabled();

  // set shader attributes once enabled
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_kernelTex1);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  glUniform1i(m_hKernTex, 2);
  glUniform2f(m_hStepXY, m_stepX, m_stepY);
  VerifyGLState();

  return true;
}

void ConvolutionFilterShader::OnDisabled()
{
}

void ConvolutionFilterShader::Free()
{
  if (m_kernelTex1)
    glDeleteTextures(1, &m_kernelTex1);
  m_kernelTex1 = 0;
  BaseVideoFilterShader::Free();
}

void DefaultFilterShader::OnCompiledAndLinked()
{
  BaseVideoFilterShader::OnCompiledAndLinked();

  m_hSourceTex = glGetUniformLocation(ProgramHandle(), "img");
}

bool DefaultFilterShader::OnEnabled()
{
  BaseVideoFilterShader::OnEnabled();

  glUniform1i(m_hSourceTex, m_sourceTexUnit);
  VerifyGLState();
  return true;
}
