/*
 *      Copyright (C) 2007-2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "guilib/TransformMatrix.h"
#include "ShaderFormats.h"

void CalculateYUVMatrix(TransformMatrix &matrix
                        , unsigned int  flags
                        , EShaderFormat format
                        , float         black
                        , float         contrast
                        , bool          limited);

#include "GLSLOutput.h"

#ifndef __GNUC__
#pragma warning( push )
#pragma warning( disable : 4250 )
#endif

#include "guilib/Shader.h"

namespace Shaders {

  class BaseYUV2RGBGLSLShader : public CGLSLShaderProgram
  {
  public:
    BaseYUV2RGBGLSLShader(bool rect, unsigned flags, EShaderFormat format, bool stretch, GLSLOutput *output=NULL);
   ~BaseYUV2RGBGLSLShader();
    void Free() override;

    void SetField(int field) { m_field  = field; }
    void SetWidth(int w) { m_width  = w; }
    void SetHeight(int h) { m_height = h; }

    void SetBlack(float black) { m_black    = black; }
    void SetContrast(float contrast) { m_contrast = contrast; }
    void SetNonLinStretch(float stretch) { m_stretch = stretch; }

    void SetConvertFullColorRange(bool convertFullRange) { m_convertFullRange = convertFullRange; }

    GLint GetVertexLoc() { return m_hVertex; }
    GLint GetYcoordLoc() { return m_hYcoord; }
    GLint GetUcoordLoc() { return m_hUcoord; }
    GLint GetVcoordLoc()  { return m_hVcoord; }

    void SetMatrices(GLfloat *p, GLfloat *m) { m_proj = p; m_model = m; }
    void SetAlpha(GLfloat alpha)  { m_alpha = alpha; }

  protected:
    void OnCompiledAndLinked() override;
    bool OnEnabled() override;
    void OnDisabled() override;

    bool m_convertFullRange;
    unsigned m_flags;
    EShaderFormat m_format;
    int m_width;
    int m_height;
    int m_field;

    float m_black;
    float m_contrast;
    float m_stretch;

    GLfloat *m_proj = nullptr;
    GLfloat *m_model = nullptr;
    GLfloat  m_alpha = 1.0f;

    std::string m_defines;

    Shaders::GLSLOutput *m_glslOutput;

    // pixel shader attribute handles
    GLint m_hYTex = -1;
    GLint m_hUTex = -1;
    GLint m_hVTex = -1;
    GLint m_hMatrix = -1;
    GLint m_hStretch = -1;
    GLint m_hStep = -1;

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
    YUV2RGBProgressiveShader(bool rect=false,
                             unsigned flags=0,
                             EShaderFormat format=SHADER_NONE,
                             bool stretch = false,
                             GLSLOutput *output=NULL);
  };

} // end namespace

#ifndef __GNUC__
#pragma warning( pop )
#endif
