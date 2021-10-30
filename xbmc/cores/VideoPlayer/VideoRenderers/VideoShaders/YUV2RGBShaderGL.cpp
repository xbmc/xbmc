/*
 *  Copyright (c) 2007 d4rk
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "YUV2RGBShaderGL.h"

#include "../RenderFlags.h"
#include "ConvolutionKernels.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <sstream>
#include <string>
#include <utility>

using namespace Shaders::GL;

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(bool rect,
                                             EShaderFormat format,
                                             bool stretch,
                                             AVColorPrimaries dstPrimaries,
                                             AVColorPrimaries srcPrimaries,
                                             bool toneMap,
                                             ETONEMAPMETHOD toneMapMethod,
                                             std::shared_ptr<GLSLOutput> output)
{
  m_width = 1;
  m_height = 1;
  m_field = 0;
  m_format = format;
  m_black = 0.0f;
  m_contrast = 1.0f;
  m_stretch = 0.0f;

  // get defines from the output stage if used
  m_glslOutput = std::move(output);
  if (m_glslOutput)
  {
    m_defines += m_glslOutput->GetDefines();
  }

  if (rect)
    m_defines += "#define XBMC_texture_rectangle 1\n";
  else
    m_defines += "#define XBMC_texture_rectangle 0\n";

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_GLRectangleHack)
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
  else
    CLog::Log(LOGERROR, "GL: BaseYUV2RGBGLSLShader - unsupported format {}", m_format);

  if (dstPrimaries != srcPrimaries)
  {
    m_colorConversion = true;
    m_defines += "#define XBMC_COL_CONVERSION\n";
  }

  if (toneMap)
  {
    m_toneMapping = true;
    m_toneMappingMethod = toneMapMethod;
    m_defines += "#define XBMC_TONE_MAPPING\n";
    if (toneMapMethod == VS_TONEMAPMETHOD_REINHARD)
      m_defines += "#define KODI_TONE_MAPPING_REINHARD\n";
    else if (toneMapMethod == VS_TONEMAPMETHOD_ACES)
      m_defines += "#define KODI_TONE_MAPPING_ACES\n";
    else if (toneMapMethod == VS_TONEMAPMETHOD_HABLE)
      m_defines += "#define KODI_TONE_MAPPING_HABLE\n";
  }

  VertexShader()->LoadSource("gl_yuv2rgb_vertex.glsl", m_defines);

  CLog::Log(LOGDEBUG, "GL: BaseYUV2RGBGLSLShader: defines:\n{}", m_defines);

  m_convMatrix.SetDestinationColorPrimaries(dstPrimaries).SetSourceColorPrimaries(srcPrimaries);
}

BaseYUV2RGBGLSLShader::~BaseYUV2RGBGLSLShader()
{
  Free();
  m_glslOutput.reset();
}

void BaseYUV2RGBGLSLShader::OnCompiledAndLinked()
{
  m_hYTex = gl::GetUniformLocation(ProgramHandle(), "m_sampY");
  m_hUTex = gl::GetUniformLocation(ProgramHandle(), "m_sampU");
  m_hVTex = gl::GetUniformLocation(ProgramHandle(), "m_sampV");
  m_hYuvMat = gl::GetUniformLocation(ProgramHandle(), "m_yuvmat");
  m_hStretch = gl::GetUniformLocation(ProgramHandle(), "m_stretch");
  m_hStep = gl::GetUniformLocation(ProgramHandle(), "m_step");
  m_hVertex = gl::GetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hYcoord = gl::GetAttribLocation(ProgramHandle(), "m_attrcordY");
  m_hUcoord = gl::GetAttribLocation(ProgramHandle(), "m_attrcordU");
  m_hVcoord = gl::GetAttribLocation(ProgramHandle(), "m_attrcordV");
  m_hProj = gl::GetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = gl::GetUniformLocation(ProgramHandle(), "m_model");
  m_hAlpha = gl::GetUniformLocation(ProgramHandle(), "m_alpha");
  m_hPrimMat = gl::GetUniformLocation(ProgramHandle(), "m_primMat");
  m_hGammaSrc = gl::GetUniformLocation(ProgramHandle(), "m_gammaSrc");
  m_hGammaDstInv = gl::GetUniformLocation(ProgramHandle(), "m_gammaDstInv");
  m_hCoefsDst = gl::GetUniformLocation(ProgramHandle(), "m_coefsDst");
  m_hToneP1 = gl::GetUniformLocation(ProgramHandle(), "m_toneP1");
  m_hLuminance = gl::GetUniformLocation(ProgramHandle(), "m_luminance");
  VerifyGLState();

  if (m_glslOutput)
    m_glslOutput->OnCompiledAndLinked(ProgramHandle());
}

bool BaseYUV2RGBGLSLShader::OnEnabled()
{
  // set shader attributes once enabled
  gl::Uniform1i(m_hYTex, 0);
  gl::Uniform1i(m_hUTex, 1);
  gl::Uniform1i(m_hVTex, 2);
  gl::Uniform1f(m_hStretch, m_stretch);
  gl::Uniform2f(m_hStep, 1.0 / m_width, 1.0 / m_height);

  m_convMatrix.SetDestinationContrast(m_contrast)
      .SetDestinationBlack(m_black)
      .SetDestinationLimitedRange(!m_convertFullRange);

  Matrix4 yuvMat = m_convMatrix.GetYuvMat();
  gl::UniformMatrix4fv(m_hYuvMat, 1, GL_FALSE, reinterpret_cast<GLfloat*>(yuvMat.ToRaw()));
  gl::UniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  gl::UniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  gl::Uniform1f(m_hAlpha, m_alpha);

  if (m_colorConversion)
  {
    Matrix3 primMat = m_convMatrix.GetPrimMat();
    gl::UniformMatrix3fv(m_hPrimMat, 1, GL_FALSE, reinterpret_cast<GLfloat*>(primMat.ToRaw()));
    gl::Uniform1f(m_hGammaSrc, m_convMatrix.GetGammaSrc());
    gl::Uniform1f(m_hGammaDstInv, 1 / m_convMatrix.GetGammaDst());
  }

  if (m_toneMapping)
  {
    if (m_toneMappingMethod == VS_TONEMAPMETHOD_REINHARD)
    {
      float param = 0.7;
      if (m_hasLightMetadata)
        param = log10(100) / log10(m_lightMetadata.MaxCLL);
      else if (m_hasDisplayMetadata && m_displayMetadata.has_luminance)
        param = log10(100) / log10(m_displayMetadata.max_luminance.num/m_displayMetadata.max_luminance.den);

      // Sanity check
      if (param < 0.1f || param > 5.0f)
        param = 0.7f;

      param *= m_toneMappingParam;

      Matrix3x1 coefs = m_convMatrix.GetRGBYuvCoefs(AVColorSpace::AVCOL_SPC_BT709);
      gl::Uniform3f(m_hCoefsDst, coefs[0], coefs[1], coefs[2]);
      gl::Uniform1f(m_hToneP1, param);
    }
    else if (m_toneMappingMethod == VS_TONEMAPMETHOD_ACES)
    {
      gl::Uniform1f(m_hLuminance, GetLuminanceValue());
      gl::Uniform1f(m_hToneP1, m_toneMappingParam);
    }
    else if (m_toneMappingMethod == VS_TONEMAPMETHOD_HABLE)
    {
      float lumin = GetLuminanceValue();
      float param = (10000.0f / lumin) * (2.0f / m_toneMappingParam);
      gl::Uniform1f(m_hLuminance, lumin);
      gl::Uniform1f(m_hToneP1, param);
    }
  }

  VerifyGLState();
  if (m_glslOutput)
    m_glslOutput->OnEnabled();
  return true;
}

void BaseYUV2RGBGLSLShader::OnDisabled()
{
  if (m_glslOutput)
    m_glslOutput->OnDisabled();
}

void BaseYUV2RGBGLSLShader::Free()
{
  if (m_glslOutput)
    m_glslOutput->Free();
}

void BaseYUV2RGBGLSLShader::SetColParams(AVColorSpace colSpace, int bits, bool limited,
                                        int textureBits)
{
  if (colSpace == AVCOL_SPC_UNSPECIFIED)
  {
    if (m_width > 1024 || m_height >= 600)
      colSpace = AVCOL_SPC_BT709;
    else
      colSpace = AVCOL_SPC_BT470BG;
  }
  m_convMatrix.SetSourceColorSpace(colSpace)
      .SetSourceBitDepth(bits)
      .SetSourceLimitedRange(limited)
      .SetSourceTextureBitDepth(textureBits);
}

void BaseYUV2RGBGLSLShader::SetDisplayMetadata(bool hasDisplayMetadata, AVMasteringDisplayMetadata displayMetadata,
                                               bool hasLightMetadata, AVContentLightMetadata lightMetadata)
{
  m_hasDisplayMetadata = hasDisplayMetadata;
  m_displayMetadata = displayMetadata;
  m_hasLightMetadata = hasLightMetadata;
  m_lightMetadata = lightMetadata;
}


void BaseYUV2RGBGLSLShader::SetToneMapParam(ETONEMAPMETHOD method, float param)
{
  m_toneMappingMethod = method;
  m_toneMappingParam = param;
}

float BaseYUV2RGBGLSLShader::GetLuminanceValue() const //Maybe move this to linuxrenderer?! same as in baserenderer
{
  float lum1 = 400.0f; // default for bad quality HDR-PQ sources (with no metadata)
  float lum2 = lum1;
  float lum3 = lum1;

  if (m_hasLightMetadata)
  {
    uint16_t lum = m_displayMetadata.max_luminance.num / m_displayMetadata.max_luminance.den;
    if (m_lightMetadata.MaxCLL >= lum)
    {
      lum1 = static_cast<float>(lum);
      lum2 = static_cast<float>(m_lightMetadata.MaxCLL);
    }
    else
    {
      lum1 = static_cast<float>(m_lightMetadata.MaxCLL);
      lum2 = static_cast<float>(lum);
    }
    lum3 = static_cast<float>(m_lightMetadata.MaxFALL);
    lum1 = (lum1 * 0.5f) + (lum2 * 0.2f) + (lum3 * 0.3f);
  }
  else if (m_hasDisplayMetadata && m_displayMetadata.has_luminance &&
           m_displayMetadata.max_luminance.num)
  {
    uint16_t lum = m_displayMetadata.max_luminance.num / m_displayMetadata.max_luminance.den;
    lum1 = static_cast<float>(lum);
  }

  return lum1;
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(bool rect,
                                                   EShaderFormat format,
                                                   bool stretch,
                                                   AVColorPrimaries dstPrimaries,
                                                   AVColorPrimaries srcPrimaries,
                                                   bool toneMap,
                                                   ETONEMAPMETHOD toneMapMethod,
                                                   std::shared_ptr<GLSLOutput> output)
  : BaseYUV2RGBGLSLShader(rect,
                          format,
                          stretch,
                          dstPrimaries,
                          srcPrimaries,
                          toneMap,
                          toneMapMethod,
                          std::move(output))
{
  PixelShader()->LoadSource("gl_yuv2rgb_basic.glsl", m_defines);
  PixelShader()->AppendSource("gl_output.glsl");

  PixelShader()->InsertSource("gl_tonemap.glsl", "vec4 process()");
}

//------------------------------------------------------------------------------
// YUV2RGBFilterShader4
//------------------------------------------------------------------------------

YUV2RGBFilterShader4::YUV2RGBFilterShader4(bool rect,
                                           EShaderFormat format,
                                           bool stretch,
                                           AVColorPrimaries dstPrimaries,
                                           AVColorPrimaries srcPrimaries,
                                           bool toneMap,
                                           ETONEMAPMETHOD toneMapMethod,
                                           ESCALINGMETHOD method,
                                           std::shared_ptr<GLSLOutput> output)
  : BaseYUV2RGBGLSLShader(rect,
                          format,
                          stretch,
                          dstPrimaries,
                          srcPrimaries,
                          toneMap,
                          toneMapMethod,
                          std::move(output))
{
  m_scaling = method;
  PixelShader()->LoadSource("gl_yuv2rgb_filter4.glsl", m_defines);
  PixelShader()->AppendSource("gl_output.glsl");

  PixelShader()->InsertSource("gl_tonemap.glsl", "vec4 process()");
}

YUV2RGBFilterShader4::~YUV2RGBFilterShader4()
{
  if (m_kernelTex)
    gl::DeleteTextures(1, &m_kernelTex);
  m_kernelTex = 0;
}

void YUV2RGBFilterShader4::OnCompiledAndLinked()
{
  BaseYUV2RGBGLSLShader::OnCompiledAndLinked();
  m_hKernTex = gl::GetUniformLocation(ProgramHandle(), "m_kernelTex");

  if (m_scaling != VS_SCALINGMETHOD_LANCZOS3_FAST && m_scaling != VS_SCALINGMETHOD_SPLINE36_FAST)
  {
    CLog::Log(LOGERROR, "GL: BaseYUV2RGBGLSLShader4 - unsupported scaling {} will fallback",
              m_scaling);
    m_scaling = VS_SCALINGMETHOD_LANCZOS3_FAST;
  }

  CConvolutionKernel kernel(m_scaling, 256);

  if (m_kernelTex)
  {
    gl::DeleteTextures(1, &m_kernelTex);
    m_kernelTex = 0;
  }
  gl::GenTextures(1, &m_kernelTex);

  //make a kernel texture on GL_TEXTURE2 and set clamping and interpolation
  //TEXTARGET is set to GL_TEXTURE_1D or GL_TEXTURE_2D
  gl::ActiveTexture(GL_TEXTURE3);
  gl::BindTexture(GL_TEXTURE_1D, m_kernelTex);
  gl::TexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  GLvoid* data = (GLvoid*)kernel.GetFloatPixels();
  gl::TexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, kernel.GetSize(), 0, GL_RGBA, GL_FLOAT, data);
  gl::ActiveTexture(GL_TEXTURE0);
  VerifyGLState();
}

bool YUV2RGBFilterShader4::OnEnabled()
{
  gl::ActiveTexture(GL_TEXTURE3);
  gl::BindTexture(GL_TEXTURE_1D, m_kernelTex);
  gl::Uniform1i(m_hKernTex, 3);

  return BaseYUV2RGBGLSLShader::OnEnabled();
}
