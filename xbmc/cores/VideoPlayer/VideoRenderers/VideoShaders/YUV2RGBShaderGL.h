/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ConversionMatrix.h"
#include "GLSLOutput.h"
#include "ShaderFormats.h"
#include "cores/VideoSettings.h"
#include "guilib/Shader.h"
#include "utils/TransformMatrix.h"

#include <memory>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/mastering_display_metadata.h>
}

class CConvertMatrix;

namespace Shaders {
namespace GL
{

class BaseYUV2RGBGLSLShader : public CGLSLShaderProgram
{
public:
  BaseYUV2RGBGLSLShader(bool rect,
                        EShaderFormat format,
                        bool stretch,
                        AVColorPrimaries dst,
                        AVColorPrimaries src,
                        bool toneMap,
                        ETONEMAPMETHOD toneMapMethod,
                        std::shared_ptr<GLSLOutput> output);
  ~BaseYUV2RGBGLSLShader() override;

  void SetField(int field) { m_field  = field; }
  void SetWidth(int w) { m_width  = w; }
  void SetHeight(int h) { m_height = h; }

  void SetColParams(AVColorSpace colSpace, int bits, bool limited, int textureBits);
  void SetBlack(float black) { m_black = black; }
  void SetContrast(float contrast) { m_contrast = contrast; }
  void SetNonLinStretch(float stretch) { m_stretch = stretch; }
  void SetDisplayMetadata(bool hasDisplayMetadata,
                          const AVMasteringDisplayMetadata& displayMetadata,
                          bool hasLightMetadata,
                          AVContentLightMetadata lightMetadata);
  void SetToneMapParam(ETONEMAPMETHOD method, float param);

  void SetConvertFullColorRange(bool convertFullRange) { m_convertFullRange = convertFullRange; }

  GLint GetVertexLoc() { return m_hVertex; }
  GLint GetYcoordLoc() { return m_hYcoord; }
  GLint GetUcoordLoc() { return m_hUcoord; }
  GLint GetVcoordLoc() { return m_hVcoord; }

  void SetMatrices(const GLfloat *p, const GLfloat *m) { m_proj = p; m_model = m; }
  void SetAlpha(GLfloat alpha)  { m_alpha = alpha; }

protected:

  void OnCompiledAndLinked() override;
  bool OnEnabled() override;
  void OnDisabled() override;
  void Free();

  bool m_convertFullRange;
  EShaderFormat m_format;
  int m_width;
  int m_height;
  int m_field;
  bool m_hasDisplayMetadata = false;
  AVMasteringDisplayMetadata m_displayMetadata;
  bool m_hasLightMetadata = false;
  AVContentLightMetadata m_lightMetadata;
  bool m_toneMapping = false;
  ETONEMAPMETHOD m_toneMappingMethod = VS_TONEMAPMETHOD_REINHARD;
  float m_toneMappingParam = 1.0;

  bool m_colorConversion{false};

  float m_black;
  float m_contrast;
  float m_stretch;

  const GLfloat *m_proj = nullptr;
  const GLfloat *m_model = nullptr;
  GLfloat m_alpha = 1.0f;

  std::string m_defines;

  std::shared_ptr<Shaders::GLSLOutput> m_glslOutput;
  CConvertMatrix m_convMatrix;

  // pixel shader attribute handles
  GLint m_hYTex = -1;
  GLint m_hUTex = -1;
  GLint m_hVTex = -1;
  GLint m_hYuvMat = -1;
  GLint m_hStretch = -1;
  GLint m_hStep = -1;
  GLint m_hGammaSrc = -1;
  GLint m_hGammaDstInv = -1;
  GLint m_hPrimMat = -1;
  GLint m_hToneP1 = -1;
  GLint m_hCoefsDst = -1;
  GLint m_hLuminance = -1;

  // vertex shader attribute handles
  GLint m_hVertex = -1;
  GLint m_hYcoord = -1;
  GLint m_hUcoord = -1;
  GLint m_hVcoord = -1;
  GLint m_hProj = -1;
  GLint m_hModel = -1;
  GLint m_hAlpha = -1;
};

class YUV2RGBProgressiveShader : public BaseYUV2RGBGLSLShader
{
public:
  YUV2RGBProgressiveShader(bool rect,
                           EShaderFormat format,
                           bool stretch,
                           AVColorPrimaries dstPrimaries,
                           AVColorPrimaries srcPrimaries,
                           bool toneMap,
                           ETONEMAPMETHOD toneMapMethod,
                           std::shared_ptr<GLSLOutput> output);
};

class YUV2RGBFilterShader4 : public BaseYUV2RGBGLSLShader
{
public:
  YUV2RGBFilterShader4(bool rect,
                       EShaderFormat format,
                       bool stretch,
                       AVColorPrimaries dstPrimaries,
                       AVColorPrimaries srcPrimaries,
                       bool toneMap,
                       ETONEMAPMETHOD toneMapMethod,
                       ESCALINGMETHOD method,
                       std::shared_ptr<GLSLOutput> output);
  ~YUV2RGBFilterShader4() override;

protected:
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

  GLuint m_kernelTex = 0;
  GLint m_hKernTex = -1;
  ESCALINGMETHOD m_scaling = VS_SCALINGMETHOD_LANCZOS3_FAST;
};

} // namespace GL
} // end namespace

