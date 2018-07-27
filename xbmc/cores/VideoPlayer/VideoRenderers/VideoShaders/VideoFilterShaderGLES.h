/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_gl.h"


#include "guilib/Shader.h"
#include "cores/VideoSettings.h"

namespace Shaders {

  class BaseVideoFilterShader : public CGLSLShaderProgram
  {
  public:
    BaseVideoFilterShader();
    virtual void OnCompiledAndLinked();
    virtual bool OnEnabled();
    virtual void SetSourceTexture(GLint ytex) { m_sourceTexUnit = ytex; }
    virtual void SetWidth(int w) { m_width  = w; m_stepX = w>0?1.0f/w:0; }
    virtual void SetHeight(int h) { m_height = h; m_stepY = h>0?1.0f/h:0; }
    virtual bool GetTextureFilter(GLint& filter) { return false; }
    virtual GLint GetVertexLoc() { return m_hVertex; }
    virtual GLint GetcoordLoc() { return m_hcoord; }
    virtual void SetMatrices(const GLfloat *p, const GLfloat *m) { m_proj = p; m_model = m; }
    virtual void SetAlpha(GLfloat alpha)             { m_alpha = alpha; }

  protected:
    int m_width;
    int m_height;
    float m_stepX;
    float m_stepY;
    GLint m_sourceTexUnit;

    // shader attribute handles
    GLint m_hSourceTex;
    GLint m_hStepXY;

    GLint m_hVertex;
    GLint m_hcoord;
    GLint m_hProj;
    GLint m_hModel;
    GLint m_hAlpha;

    const GLfloat *m_proj;
    const GLfloat *m_model;
    GLfloat  m_alpha;
  };

  class ConvolutionFilterShader : public BaseVideoFilterShader
  {
  public:
    ConvolutionFilterShader(ESCALINGMETHOD method);
    ~ConvolutionFilterShader() override;
    void OnCompiledAndLinked() override;
    bool OnEnabled() override;
    void OnDisabled() override;
    void Free();

    bool GetTextureFilter(GLint& filter) override { filter = GL_NEAREST; return true; }

  protected:
    // kernel textures
    GLuint m_kernelTex1;

    // shader handles to kernel textures
    GLint m_hKernTex;

    ESCALINGMETHOD m_method;
    bool m_floattex; //if float textures are supported
    GLint m_internalformat;
  };

  class DefaultFilterShader : public BaseVideoFilterShader
  {
    public:
      void OnCompiledAndLinked() override;
      bool OnEnabled() override;
  };

} // end namespace

