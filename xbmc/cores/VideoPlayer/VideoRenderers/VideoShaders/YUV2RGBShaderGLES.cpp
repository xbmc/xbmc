/*
 *  Copyright (c) 2007 d4rk
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "../RenderFlags.h"
#include "YUV2RGBShaderGLES.h"
#include "ConversionMatrix.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

#include <string>
#include <sstream>

using namespace Shaders;

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(EShaderFormat format, AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries)
{
  m_width = 1;
  m_height = 1;
  m_field = 0;
  m_format = format;

  m_black = 0.0f;
  m_contrast = 1.0f;

  m_convertFullRange = false;

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
    CLog::Log(LOGERROR, "GLES: BaseYUV2RGBGLSLShader - unsupported format %d", m_format);

  CLog::Log(LOGDEBUG, "GLES: BaseYUV2RGBGLSLShader - srcPrimaries {}", srcPrimaries);
  CLog::Log(LOGDEBUG, "GLES: BaseYUV2RGBGLSLShader - dstPrimaries {}", dstPrimaries);

  if (dstPrimaries != srcPrimaries)
  {
    m_defines += "#define XBMC_COL_CONVERSION\n";
  }

  VertexShader()->LoadSource("gles_yuv2rgb.vert", m_defines);

  CLog::Log(LOGDEBUG, "GLES: BaseYUV2RGBGLSLShader: defines:\n%s", m_defines.c_str());

  m_pConvMatrix.reset(new CConvertMatrix());
  m_pConvMatrix->SetColPrimaries(dstPrimaries, srcPrimaries);
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
  VerifyGLState();
}

bool BaseYUV2RGBGLSLShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, 0);
  glUniform1i(m_hUTex, 1);
  glUniform1i(m_hVTex, 2);
  glUniform2f(m_hStep, 1.0 / m_width, 1.0 / m_height);

  GLfloat yuvMat[4][4];
  // keep video levels
  m_pConvMatrix->SetParams(m_contrast, m_black, !m_convertFullRange);
  m_pConvMatrix->GetYuvMat(yuvMat);

  glUniformMatrix4fv(m_hYuvMat, 1, GL_FALSE, (GLfloat*)yuvMat);
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  glUniform1f(m_hAlpha, m_alpha);

  GLfloat primMat[3][3];
  if (m_pConvMatrix->GetPrimMat(primMat))
  {
    glUniformMatrix3fv(m_hPrimMat, 1, GL_FALSE, (GLfloat*)primMat);
    glUniform1f(m_hGammaSrc, m_pConvMatrix->GetGammaSrc());
    glUniform1f(m_hGammaDstInv, 1/m_pConvMatrix->GetGammaDst());
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
  m_pConvMatrix->SetColParams(colSpace, bits, limited, textureBits);
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(EShaderFormat format, AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries)
  : BaseYUV2RGBGLSLShader(format, dstPrimaries, srcPrimaries)
{
  PixelShader()->LoadSource("gles_yuv2rgb_basic.frag", m_defines);
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader(EShaderFormat format, AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries)
  : BaseYUV2RGBGLSLShader(format, dstPrimaries, srcPrimaries)
{
  m_hStepX = -1;
  m_hStepY = -1;
  m_hField = -1;

  PixelShader()->LoadSource("gles_yuv2rgb_bob.frag", m_defines);
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
