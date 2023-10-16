/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ConversionMatrix.h"
#include "ShaderFormats.h"
#include "cores/VideoSettings.h"
#include "guilib/Shader.h"
#include "utils/TransformMatrix.h"

extern "C" {
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/pixfmt.h>
}

namespace Shaders {
namespace GLES
{

class BaseYUV2RGBGLSLShader : public CGLSLShaderProgram
{
  public:
    BaseYUV2RGBGLSLShader(EShaderFormat format,
                          AVColorPrimaries dst,
                          AVColorPrimaries src,
                          bool toneMap,
                          ETONEMAPMETHOD toneMapMethod);
    ~BaseYUV2RGBGLSLShader() override;
    void SetField(int field) { m_field = field; }
    void SetWidth(int w) { m_width = w; }
    void SetHeight(int h) { m_height = h; }

    void SetColParams(AVColorSpace colSpace, int bits, bool limited, int textureBits);
    void SetBlack(float black) { m_black = black; }
    void SetContrast(float contrast) { m_contrast = contrast; }
    void SetConvertFullColorRange(bool convertFullRange) { m_convertFullRange = convertFullRange; }
    void SetDisplayMetadata(bool hasDisplayMetadata,
                            const AVMasteringDisplayMetadata& displayMetadata,
                            bool hasLightMetadata,
                            AVContentLightMetadata lightMetadata);
    void SetToneMapParam(float param) { m_toneMappingParam = param; }

    GLint GetVertexLoc() { return m_hVertex; }
    GLint GetYcoordLoc() { return m_hYcoord; }
    GLint GetUcoordLoc() { return m_hUcoord; }
    GLint GetVcoordLoc() { return m_hVcoord; }

    void SetMatrices(const GLfloat *p, const GLfloat *m) { m_proj = p; m_model = m; }
    void SetAlpha(GLfloat alpha) { m_alpha = alpha; }

  protected:
    void OnCompiledAndLinked() override;
    bool OnEnabled() override;
    void OnDisabled() override;
    void Free();

    EShaderFormat m_format;
    int m_width;
    int m_height;
    int m_field;
    bool m_hasDisplayMetadata{false};
    AVMasteringDisplayMetadata m_displayMetadata;
    bool m_hasLightMetadata{false};
    AVContentLightMetadata m_lightMetadata;
    bool m_toneMapping{false};
    ETONEMAPMETHOD m_toneMappingMethod{VS_TONEMAPMETHOD_REINHARD};
    float m_toneMappingParam{1.0};

    bool m_colorConversion{false};

    float m_black;
    float m_contrast;

    std::string m_defines;

    CConvertMatrix m_convMatrix;

    // shader attribute handles
    GLint m_hYTex{-1};
    GLint m_hUTex{-1};
    GLint m_hVTex{-1};
    GLint m_hYuvMat{-1};
    GLint m_hStep{-1};
    GLint m_hGammaSrc{-1};
    GLint m_hGammaDstInv{-1};
    GLint m_hPrimMat{-1};
    GLint m_hToneP1{-1};
    GLint m_hCoefsDst{-1};
    GLint m_hLuminance = -1;

    GLint m_hVertex{-1};
    GLint m_hYcoord{-1};
    GLint m_hUcoord{-1};
    GLint m_hVcoord{-1};
    GLint m_hProj{-1};
    GLint m_hModel{-1};
    GLint m_hAlpha{-1};

    const GLfloat *m_proj{nullptr};
    const GLfloat *m_model{nullptr};
    GLfloat m_alpha{1.0f};

    bool m_convertFullRange;
  };

  class YUV2RGBProgressiveShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBProgressiveShader(EShaderFormat format,
                             AVColorPrimaries dstPrimaries,
                             AVColorPrimaries srcPrimaries,
                             bool toneMap,
                             ETONEMAPMETHOD toneMapMethod);
  };

  class YUV2RGBBobShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBBobShader(EShaderFormat format,
                     AVColorPrimaries dstPrimaries,
                     AVColorPrimaries srcPrimaries,
                     bool toneMap,
                     ETONEMAPMETHOD toneMapMethod);
    void OnCompiledAndLinked() override;
    bool OnEnabled() override;

    GLint m_hStepX = -1;
    GLint m_hStepY = -1;
    GLint m_hField = -1;
  };

  } // namespace GLES
} // end namespace
