/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLSLOutput.h"
#include "cores/VideoSettings.h"
#include "guilib/Shader.h"

#include "system_gl.h"

namespace Shaders {

namespace GL
{

class BaseVideoFilterShader : public CGLSLShaderProgram
{
public:
  BaseVideoFilterShader();
  ~BaseVideoFilterShader() override;
  virtual bool GetTextureFilter(GLint& filter) { return false; }

  void SetSourceTexture(GLint ytex) { m_sourceTexUnit = ytex; }
  void SetWidth(int w)
  {
    m_width = w;
    m_stepX = w > 0 ? 1.0f / w : 0;
  }
  void SetHeight(int h)
  {
    m_height = h;
    m_stepY = h > 0 ? 1.0f / h : 0;
  }
  void SetNonLinStretch(float stretch) { m_stretch = stretch; }
  void SetAlpha(GLfloat alpha) { m_alpha = alpha; }

  GLint GetVertexLoc() { return m_hVertex; }
  GLint GetCoordLoc() { return m_hCoord; }

  void SetMatrices(const GLfloat* p, const GLfloat* m)
  {
    m_proj = p;
    m_model = m;
  }

protected:
  int m_width;
  int m_height;
  float m_stepX;
  float m_stepY;
  float m_stretch;
  GLfloat m_alpha;
  GLint m_sourceTexUnit = 0;
  const GLfloat* m_proj = nullptr;
  const GLfloat* m_model = nullptr;

  // shader attribute handles
  GLint m_hSourceTex = 0;
  GLint m_hStepXY = 0;
  GLint m_hStretch = -1;
  GLint m_hAlpha = -1;
  GLint m_hVertex = -1;
  GLint m_hCoord = -1;
  GLint m_hProj = -1;
  GLint m_hModel = -1;
  };

  class ConvolutionFilterShader : public BaseVideoFilterShader
  {
  public:
    ConvolutionFilterShader(ESCALINGMETHOD method, bool stretch, GLSLOutput *output=NULL);
    ~ConvolutionFilterShader() override;
    void OnCompiledAndLinked() override;
    bool OnEnabled() override;
    void OnDisabled() override;
    void Free();

    bool GetTextureFilter(GLint& filter) override { filter = GL_NEAREST; return true; }

  protected:
    // kernel textures
    GLuint m_kernelTex1 = 0;

    // shader handles to kernel textures
    GLint m_hKernTex;

    ESCALINGMETHOD m_method;
    bool m_floattex; //if float textures are supported
    GLint m_internalformat;

    GLSLOutput* m_glslOutput;
  };

  class StretchFilterShader : public BaseVideoFilterShader
  {
    public:
      StretchFilterShader();
      void OnCompiledAndLinked() override;
      bool OnEnabled() override;
  };

  class DefaultFilterShader : public BaseVideoFilterShader
  {
    public:
      void OnCompiledAndLinked() override;
      bool OnEnabled() override;
  };

  } // namespace GL
} // end namespace


