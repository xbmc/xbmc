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
  m_hStepXY = 0;
  m_stepX = 0;
  m_stepY = 0;
  m_sourceTexUnit = 0;
  m_hSourceTex = 0;
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
  m_kernelTex1 = 0;

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

  CLog::Log(LOGDEBUG, "GL: ConvolutionFilterShader: using {} defines:\n{}", shadername, defines);
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
  m_hSourceTex = gl::GetUniformLocation(ProgramHandle(), "img");
  m_hStepXY = gl::GetUniformLocation(ProgramHandle(), "stepxy");
  m_hKernTex = gl::GetUniformLocation(ProgramHandle(), "kernelTex");
  m_hStretch = gl::GetUniformLocation(ProgramHandle(), "m_stretch");
  m_hAlpha = gl::GetUniformLocation(ProgramHandle(), "m_alpha");
  m_hProj = gl::GetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = gl::GetUniformLocation(ProgramHandle(), "m_model");
  m_hVertex = gl::GetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hCoord = gl::GetAttribLocation(ProgramHandle(), "m_attrcord");

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
  //TEXTARGET is set to GL_TEXTURE_1D or GL_TEXTURE_2D
  gl::ActiveTexture(GL_TEXTURE2);
  gl::BindTexture(TEXTARGET, m_kernelTex1);
  gl::TexParameteri(TEXTARGET, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(TEXTARGET, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexParameteri(TEXTARGET, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(TEXTARGET, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

  gl::TexImage1D(TEXTARGET, 0, m_internalformat, kernel.GetSize(), 0, GL_RGBA, format, data);

  gl::ActiveTexture(GL_TEXTURE0);

  VerifyGLState();

  if (m_glslOutput)
    m_glslOutput->OnCompiledAndLinked(ProgramHandle());
}

bool ConvolutionFilterShader::OnEnabled()
{
  // set shader attributes once enabled
  gl::ActiveTexture(GL_TEXTURE2);
  gl::BindTexture(TEXTARGET, m_kernelTex1);

  gl::ActiveTexture(GL_TEXTURE0);
  gl::Uniform1i(m_hSourceTex, m_sourceTexUnit);
  gl::Uniform1i(m_hKernTex, 2);
  gl::Uniform2f(m_hStepXY, m_stepX, m_stepY);
  gl::Uniform1f(m_hStretch, m_stretch);
  gl::Uniform1f(m_hAlpha, m_alpha);

  gl::UniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  gl::UniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);

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
    gl::DeleteTextures(1, &m_kernelTex1);
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
  m_hSourceTex = gl::GetUniformLocation(ProgramHandle(), "img");
  m_hStretch = gl::GetUniformLocation(ProgramHandle(), "m_stretch");
  m_hAlpha = gl::GetUniformLocation(ProgramHandle(), "m_alpha");
  m_hProj = gl::GetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = gl::GetUniformLocation(ProgramHandle(), "m_model");
  m_hVertex = gl::GetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hCoord = gl::GetAttribLocation(ProgramHandle(), "m_attrcord");
}

bool StretchFilterShader::OnEnabled()
{
  gl::Uniform1i(m_hSourceTex, m_sourceTexUnit);
  gl::Uniform1f(m_hStretch, m_stretch);
  gl::Uniform1f(m_hAlpha, m_alpha);
  gl::UniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  gl::UniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  VerifyGLState();
  return true;
}

//////////////////////////////////////////////////////////////////////
// DefaultFilterShader - base class for video filter shaders
//////////////////////////////////////////////////////////////////////

void DefaultFilterShader::OnCompiledAndLinked()
{
  m_hSourceTex = gl::GetUniformLocation(ProgramHandle(), "img");
  m_hProj = gl::GetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = gl::GetUniformLocation(ProgramHandle(), "m_model");
  m_hVertex = gl::GetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hCoord = gl::GetAttribLocation(ProgramHandle(), "m_attrcord");
  m_hAlpha = gl::GetUniformLocation(ProgramHandle(), "m_alpha");
}

bool DefaultFilterShader::OnEnabled()
{
  gl::Uniform1i(m_hSourceTex, m_sourceTexUnit);
  gl::Uniform1f(m_hAlpha, m_alpha);
  gl::UniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  gl::UniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  VerifyGLState();
  return true;
}
