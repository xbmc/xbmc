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
#include "ConversionMatrix.h"
#include "ConvolutionKernels.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <sstream>
#include <string>
#include <utility>

using namespace Shaders;

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(bool rect, EShaderFormat format, bool stretch,
                                             AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries,
                                             bool toneMap,
                                             int toneMapMethod,
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
  else if (m_format == SHADER_YV12)
    m_defines += "#define XBMC_YV12\n";
  else
    CLog::Log(LOGERROR, "GL: BaseYUV2RGBGLSLShader - unsupported format %d", m_format);

  if (dstPrimaries != srcPrimaries)
  {
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

  CLog::Log(LOGDEBUG, "GL: BaseYUV2RGBGLSLShader: defines:\n%s", m_defines.c_str());

  m_pConvMatrix.reset(new CConvertMatrix());
  m_pConvMatrix->SetColPrimaries(dstPrimaries, srcPrimaries);
}

BaseYUV2RGBGLSLShader::~BaseYUV2RGBGLSLShader()
{
  Free();
  m_glslOutput.reset();
}

void BaseYUV2RGBGLSLShader::OnCompiledAndLinked()
{
  m_hYTex = glGetUniformLocation(ProgramHandle(), "m_sampY");
  m_hUTex = glGetUniformLocation(ProgramHandle(), "m_sampU");
  m_hVTex = glGetUniformLocation(ProgramHandle(), "m_sampV");
  m_hYuvMat = glGetUniformLocation(ProgramHandle(), "m_yuvmat");
  m_hStretch = glGetUniformLocation(ProgramHandle(), "m_stretch");
  m_hStep = glGetUniformLocation(ProgramHandle(), "m_step");
  m_hVertex = glGetAttribLocation(ProgramHandle(), "m_attrpos");
  m_hYcoord = glGetAttribLocation(ProgramHandle(), "m_attrcordY");
  m_hUcoord = glGetAttribLocation(ProgramHandle(), "m_attrcordU");
  m_hVcoord = glGetAttribLocation(ProgramHandle(), "m_attrcordV");
  m_hProj = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "m_alpha");
  m_hPrimMat = glGetUniformLocation(ProgramHandle(), "m_primMat");
  m_hGammaSrc = glGetUniformLocation(ProgramHandle(), "m_gammaSrc");
  m_hGammaDstInv = glGetUniformLocation(ProgramHandle(), "m_gammaDstInv");
  m_hCoefsDst = glGetUniformLocation(ProgramHandle(), "m_coefsDst");
  m_hToneP1 = glGetUniformLocation(ProgramHandle(), "m_toneP1");
  m_hLuminance = glGetUniformLocation(ProgramHandle(), "m_luminance");
  VerifyGLState();

  if (m_glslOutput)
    m_glslOutput->OnCompiledAndLinked(ProgramHandle());
}

bool BaseYUV2RGBGLSLShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, 0);
  glUniform1i(m_hUTex, 1);
  glUniform1i(m_hVTex, 2);
  glUniform1f(m_hStretch, m_stretch);
  glUniform2f(m_hStep, 1.0 / m_width, 1.0 / m_height);

  GLfloat yuvMat[4][4];
  m_pConvMatrix->SetParams(m_contrast, m_black, !m_convertFullRange);
  m_pConvMatrix->GetYuvMat(yuvMat);

  glUniformMatrix4fv(m_hYuvMat, 1, GL_FALSE, (GLfloat*)yuvMat);
  glUniformMatrix4fv(m_hProj, 1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  glUniform1f(m_hAlpha, m_alpha);

  GLfloat primMat[3][3];
  if (m_pConvMatrix->GetPrimMat(primMat))
  {
    glUniformMatrix3fv(m_hPrimMat, 1, GL_FALSE, (GLfloat*)primMat);
    glUniform1f(m_hGammaSrc, m_pConvMatrix->GetGammaSrc());
    glUniform1f(m_hGammaDstInv, 1/m_pConvMatrix->GetGammaDst());
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

      float coefs[3];
      CConvertMatrix::GetRGBYuvCoefs(AVColorSpace::AVCOL_SPC_BT709, coefs);
      glUniform3f(m_hCoefsDst, coefs[0], coefs[1], coefs[2]);
      glUniform1f(m_hToneP1, param);
    }
    else if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_ACES)
    {
      glUniform1f(m_hLuminance, GetLuminanceValue());
      glUniform1f(m_hToneP1, m_toneMappingParam);
    }
    else if (m_toneMapping && m_toneMappingMethod == VS_TONEMAPMETHOD_HABLE)
    {
      float lumin = GetLuminanceValue();
      float param = (10000.0f / lumin) * (2.0f / m_toneMappingParam);
      glUniform1f(m_hLuminance, lumin);
      glUniform1f(m_hToneP1, param);
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
  m_pConvMatrix->SetColParams(colSpace, bits, limited, textureBits);
}

void BaseYUV2RGBGLSLShader::SetDisplayMetadata(bool hasDisplayMetadata, AVMasteringDisplayMetadata displayMetadata,
                                               bool hasLightMetadata, AVContentLightMetadata lightMetadata)
{
  m_hasDisplayMetadata = hasDisplayMetadata;
  m_displayMetadata = displayMetadata;
  m_hasLightMetadata = hasLightMetadata;
  m_lightMetadata = lightMetadata;
}


void BaseYUV2RGBGLSLShader::SetToneMapParam(int method, float param)
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
                                                   int toneMapMethod,
                                                   std::shared_ptr<GLSLOutput> output)
  : BaseYUV2RGBGLSLShader(
        rect, format, stretch, dstPrimaries, srcPrimaries, toneMap, toneMapMethod, std::move(output))
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
                                           int toneMapMethod,
                                           ESCALINGMETHOD method,
                                           std::shared_ptr<GLSLOutput> output)
  : BaseYUV2RGBGLSLShader(
        rect, format, stretch, dstPrimaries, srcPrimaries, toneMap, toneMapMethod, std::move(output))
{
  m_scaling = method;
  PixelShader()->LoadSource("gl_yuv2rgb_filter4.glsl", m_defines);
  PixelShader()->AppendSource("gl_output.glsl");

  PixelShader()->InsertSource("gl_tonemap.glsl", "vec4 process()");
}

YUV2RGBFilterShader4::~YUV2RGBFilterShader4()
{
  if (m_kernelTex)
    glDeleteTextures(1, &m_kernelTex);
  m_kernelTex = 0;
}

void YUV2RGBFilterShader4::OnCompiledAndLinked()
{
  BaseYUV2RGBGLSLShader::OnCompiledAndLinked();
  m_hKernTex = glGetUniformLocation(ProgramHandle(), "m_kernelTex");

  if (m_scaling != VS_SCALINGMETHOD_LANCZOS3_FAST && m_scaling != VS_SCALINGMETHOD_SPLINE36_FAST)
  {
    CLog::Log(LOGERROR, "GL: BaseYUV2RGBGLSLShader4 - unsupported scaling %d will fallback", m_scaling);
    m_scaling = VS_SCALINGMETHOD_LANCZOS3_FAST;
  }

  CConvolutionKernel kernel(m_scaling, 256);

  if (m_kernelTex)
  {
    glDeleteTextures(1, &m_kernelTex);
    m_kernelTex = 0;
  }
  glGenTextures(1, &m_kernelTex);

  //make a kernel texture on GL_TEXTURE2 and set clamping and interpolation
  //TEXTARGET is set to GL_TEXTURE_1D or GL_TEXTURE_2D
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_1D, m_kernelTex);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  GLvoid* data = (GLvoid*)kernel.GetFloatPixels();
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, kernel.GetSize(), 0, GL_RGBA, GL_FLOAT, data);
  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();
}

bool YUV2RGBFilterShader4::OnEnabled()
{
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_1D, m_kernelTex);
  glUniform1i(m_hKernTex, 3);

  return BaseYUV2RGBGLSLShader::OnEnabled();
}
