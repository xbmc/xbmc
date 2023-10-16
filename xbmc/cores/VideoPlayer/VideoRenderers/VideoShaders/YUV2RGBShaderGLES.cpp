/*
 *  Copyright (c) 2007 d4rk
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "YUV2RGBShaderGLES.h"

#include "../RenderFlags.h"
#include "ToneMappers.h"
#include "settings/AdvancedSettings.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <sstream>
#include <string>

using namespace Shaders::GLES;

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(EShaderFormat format,
                                             AVColorPrimaries dstPrimaries,
                                             AVColorPrimaries srcPrimaries,
                                             bool toneMap,
                                             ETONEMAPMETHOD toneMapMethod)
{
  m_width = 1;
  m_height = 1;
  m_field = 0;
  m_format = format;

  m_black = 0.0f;
  m_contrast = 1.0f;

  m_convertFullRange = false;

  if (m_format == SHADER_YV12)
    m_defines += "#define XBMC_YV12\n";
  else if (m_format == SHADER_NV12)
    m_defines += "#define XBMC_NV12\n";
  else if (m_format == SHADER_NV12_RRG)
    m_defines += "#define XBMC_NV12_RRG\n";
  else
    CLog::Log(LOGERROR, "GLES: BaseYUV2RGBGLSLShader - unsupported format {}", m_format);

  if (dstPrimaries != srcPrimaries)
  {
    m_colorConversion = true;
    m_defines += "#define XBMC_COL_CONVERSION\n";
  }

  if (toneMap)
  {
    m_toneMapping = true;
    m_toneMappingMethod = toneMapMethod;
    if (toneMapMethod == VS_TONEMAPMETHOD_REINHARD)
      m_defines += "#define KODI_TONE_MAPPING_REINHARD\n";
    else if (toneMapMethod == VS_TONEMAPMETHOD_ACES)
      m_defines += "#define KODI_TONE_MAPPING_ACES\n";
    else if (toneMapMethod == VS_TONEMAPMETHOD_HABLE)
      m_defines += "#define KODI_TONE_MAPPING_HABLE\n";
  }

  VertexShader()->LoadSource("gles_yuv2rgb.vert", m_defines);

  CLog::Log(LOGDEBUG, "GLES: using shader format: {}", m_format);
  CLog::Log(LOGDEBUG, "GLES: using tonemap method: {}", m_toneMappingMethod);

  m_convMatrix.SetSourceColorPrimaries(srcPrimaries).SetDestinationColorPrimaries(dstPrimaries);
}

BaseYUV2RGBGLSLShader::~BaseYUV2RGBGLSLShader()
{
  Free();
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
  m_hYuvMat = glGetUniformLocation(ProgramHandle(), "m_yuvmat");
  m_hStep = glGetUniformLocation(ProgramHandle(), "m_step");
  m_hPrimMat = glGetUniformLocation(ProgramHandle(), "m_primMat");
  m_hGammaSrc = glGetUniformLocation(ProgramHandle(), "m_gammaSrc");
  m_hGammaDstInv = glGetUniformLocation(ProgramHandle(), "m_gammaDstInv");
  m_hCoefsDst = glGetUniformLocation(ProgramHandle(), "m_coefsDst");
  m_hToneP1 = glGetUniformLocation(ProgramHandle(), "m_toneP1");
  m_hLuminance = glGetUniformLocation(ProgramHandle(), "m_luminance");
  VerifyGLState();
}

bool BaseYUV2RGBGLSLShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, 0);
  glUniform1i(m_hUTex, 1);
  glUniform1i(m_hVTex, 2);
  glUniform2f(m_hStep, 1.0 / m_width, 1.0 / m_height);

  m_convMatrix.SetDestinationContrast(m_contrast)
      .SetDestinationBlack(m_black)
      .SetDestinationLimitedRange(!m_convertFullRange);

  Matrix4 yuvMat = m_convMatrix.GetYuvMat();
  glUniformMatrix4fv(m_hYuvMat, 1, GL_FALSE, yuvMat.ToRaw());
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  glUniform1f(m_hAlpha, m_alpha);

  if (m_colorConversion)
  {
    Matrix3 primMat = m_convMatrix.GetPrimMat();
    glUniformMatrix3fv(m_hPrimMat, 1, GL_FALSE, primMat.ToRaw());
    glUniform1f(m_hGammaSrc, m_convMatrix.GetGammaSrc());
    glUniform1f(m_hGammaDstInv, 1 / m_convMatrix.GetGammaDst());
  }

  if (m_toneMapping)
  {
    if (m_toneMappingMethod == VS_TONEMAPMETHOD_REINHARD)
    {
      float param = 0.7;

      if (m_hasLightMetadata)
      {
        param = log10(100) / log10(m_lightMetadata.MaxCLL);
      }
      else if (m_hasDisplayMetadata && m_displayMetadata.has_luminance)
      {
        param = log10(100) /
                log10(m_displayMetadata.max_luminance.num / m_displayMetadata.max_luminance.den);
      }

      // Sanity check
      if (param < 0.1f || param > 5.0f)
      {
        param = 0.7f;
      }

      param *= m_toneMappingParam;

      Matrix3x1 coefs = m_convMatrix.GetRGBYuvCoefs(AVColorSpace::AVCOL_SPC_BT709);
      glUniform3f(m_hCoefsDst, coefs[0], coefs[1], coefs[2]);
      glUniform1f(m_hToneP1, param);
    }
    else if (m_toneMappingMethod == VS_TONEMAPMETHOD_ACES)
    {
      const float lumin = CToneMappers::GetLuminanceValue(m_hasDisplayMetadata, m_displayMetadata,
                                                          m_hasLightMetadata, m_lightMetadata);
      glUniform1f(m_hLuminance, lumin);
      glUniform1f(m_hToneP1, m_toneMappingParam);
    }
    else if (m_toneMappingMethod == VS_TONEMAPMETHOD_HABLE)
    {
      const float lumin = CToneMappers::GetLuminanceValue(m_hasDisplayMetadata, m_displayMetadata,
                                                          m_hasLightMetadata, m_lightMetadata);
      const float param = (10000.0f / lumin) * (2.0f / m_toneMappingParam);
      glUniform1f(m_hLuminance, lumin);
      glUniform1f(m_hToneP1, param);
    }
  }

  VerifyGLState();

  return true;
}

void BaseYUV2RGBGLSLShader::OnDisabled()
{
}

void BaseYUV2RGBGLSLShader::Free()
{
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

void BaseYUV2RGBGLSLShader::SetDisplayMetadata(bool hasDisplayMetadata,
                                               const AVMasteringDisplayMetadata& displayMetadata,
                                               bool hasLightMetadata,
                                               AVContentLightMetadata lightMetadata)
{
  m_hasDisplayMetadata = hasDisplayMetadata;
  m_displayMetadata = displayMetadata;
  m_hasLightMetadata = hasLightMetadata;
  m_lightMetadata = lightMetadata;
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(EShaderFormat format,
                                                   AVColorPrimaries dstPrimaries,
                                                   AVColorPrimaries srcPrimaries,
                                                   bool toneMap,
                                                   ETONEMAPMETHOD toneMapMethod)
  : BaseYUV2RGBGLSLShader(format, dstPrimaries, srcPrimaries, toneMap, toneMapMethod)
{
  PixelShader()->LoadSource("gles_yuv2rgb_basic.frag", m_defines);
  PixelShader()->InsertSource("gles_tonemap.frag", "void main()");
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader(EShaderFormat format,
                                   AVColorPrimaries dstPrimaries,
                                   AVColorPrimaries srcPrimaries,
                                   bool toneMap,
                                   ETONEMAPMETHOD toneMapMethod)
  : BaseYUV2RGBGLSLShader(format, dstPrimaries, srcPrimaries, toneMap, toneMapMethod)
{
  PixelShader()->LoadSource("gles_yuv2rgb_bob.frag", m_defines);
  PixelShader()->InsertSource("gles_tonemap.frag", "void main()");
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
