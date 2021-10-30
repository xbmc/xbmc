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
  m_hStepXY = 0;
  m_stepX = 0;
  m_stepY = 0;
  m_sourceTexUnit = 0;
  m_hSourceTex = 0;

  m_hVertex = -1;
  m_hcoord = -1;
  m_hProj = -1;
  m_hModel = -1;
  m_hAlpha = -1;

  m_proj = nullptr;
  m_model = nullptr;
  m_alpha = -1;

  VertexShader()->LoadSource("gles_videofilter.vert");

  PixelShader()->LoadSource("gles_videofilter.frag");
}

void BaseVideoFilterShader::OnCompiledAndLinked()
{
  m_hVertex = gl::GetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hcoord = gl::GetAttribLocation(ProgramHandle(), "m_attrcord");
  m_hAlpha = gl::GetUniformLocation(ProgramHandle(), "m_alpha");
  m_hProj = gl::GetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = gl::GetUniformLocation(ProgramHandle(), "m_model");
}

bool BaseVideoFilterShader::OnEnabled()
{
  gl::UniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  gl::UniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  gl::Uniform1f(m_hAlpha, m_alpha);
  return true;
}

ConvolutionFilterShader::ConvolutionFilterShader(ESCALINGMETHOD method)
{
  m_method = method;
  m_kernelTex1 = 0;
  m_hKernTex = -1;

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

  CLog::Log(LOGDEBUG, "GL: ConvolutionFilterShader: using {} defines:\n{}", shadername, defines);
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
  m_hSourceTex = gl::GetUniformLocation(ProgramHandle(), "img");
  m_hStepXY = gl::GetUniformLocation(ProgramHandle(), "stepxy");
  m_hKernTex = gl::GetUniformLocation(ProgramHandle(), "kernelTex");

  CConvolutionKernel kernel(m_method, 256);

  if (m_kernelTex1)
  {
    gl::DeleteTextures(1, &m_kernelTex1);
    m_kernelTex1 = 0;
  }

  gl::GenTextures(1, &m_kernelTex1);

  if ((m_kernelTex1<=0))
  {
    CLog::Log(LOGERROR, "GL: ConvolutionFilterShader: Error creating kernel texture");
    return;
  }

  //make a kernel texture on GL_TEXTURE2 and set clamping and interpolation
  gl::ActiveTexture(GL_TEXTURE2);
  gl::BindTexture(GL_TEXTURE_2D, m_kernelTex1);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
  gl::TexImage2D(GL_TEXTURE_2D, 0, m_internalformat, kernel.GetSize(), 1, 0, GL_RGBA, format, data);

  gl::ActiveTexture(GL_TEXTURE0);

  VerifyGLState();
}

bool ConvolutionFilterShader::OnEnabled()
{
  BaseVideoFilterShader::OnEnabled();

  // set shader attributes once enabled
  gl::ActiveTexture(GL_TEXTURE2);
  gl::BindTexture(GL_TEXTURE_2D, m_kernelTex1);

  gl::ActiveTexture(GL_TEXTURE0);
  gl::Uniform1i(m_hSourceTex, m_sourceTexUnit);
  gl::Uniform1i(m_hKernTex, 2);
  gl::Uniform2f(m_hStepXY, m_stepX, m_stepY);
  VerifyGLState();

  return true;
}

void ConvolutionFilterShader::OnDisabled()
{
}

void ConvolutionFilterShader::Free()
{
  if (m_kernelTex1)
    gl::DeleteTextures(1, &m_kernelTex1);
  m_kernelTex1 = 0;
  BaseVideoFilterShader::Free();
}

void DefaultFilterShader::OnCompiledAndLinked()
{
  BaseVideoFilterShader::OnCompiledAndLinked();

  m_hSourceTex = gl::GetUniformLocation(ProgramHandle(), "img");
}

bool DefaultFilterShader::OnEnabled()
{
  BaseVideoFilterShader::OnEnabled();

  gl::Uniform1i(m_hSourceTex, m_sourceTexUnit);
  VerifyGLState();
  return true;
}
